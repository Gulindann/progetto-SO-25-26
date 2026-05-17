#include "headers/interrupts.h"
#include "uriscv/liburiscv.h"

state_t *saved_interrupt_state;

void PLT();
void PseudoClock();
void DeviceInterrupt(int line);
void interruptHandler(unsigned int cause_reg)
{
    saved_interrupt_state = GET_EXCEPTION_STATE_PTR(0);

    // 1. Estraggo l'Exception Code come dicono le specifiche
    unsigned int excCode = cause_reg & CAUSE_EXCCODE_MASK;

    // 2. Mappo l'Exception Code alla funzione o alla Linea corretta (Tabella 1)
    if (excCode == IL_CPUTIMER) // Exc Code 7 -> Linea 1 (PLT)
        PLT();
    else if (excCode == IL_TIMER) // Exc Code 3 -> Linea 2 (Interval Timer)
        PseudoClock();
    else if (excCode == IL_DISK) // Exc Code 17 -> Linea 3 (Dischi)
        DeviceInterrupt(3);
    else if (excCode == IL_FLASH) // Exc Code 18 -> Linea 4 (Flash)
        DeviceInterrupt(4);
    else if (excCode == IL_ETHERNET) // Exc Code 19 -> Linea 5 (Rete)
        DeviceInterrupt(5);
    else if (excCode == IL_PRINTER) // Exc Code 20 -> Linea 6 (Stampanti)
        DeviceInterrupt(6);
    else if (excCode == IL_TERMINAL) // Exc Code 21 -> Linea 7 (Terminali)
        DeviceInterrupt(7);
    else
        HALT(); // Se scatta questo, abbiamo davvero un problema hardware
}
void PLT()
{

    setTIMER(TIMESLICE);

    if (currentProcess != NULL)
    {
        currentProcess->p_s = *saved_interrupt_state;

        cpu_t timenow;
        STCK(timenow);
        currentProcess->p_time += (timenow - p_start);

        insertProcQ(&readyQueue, currentProcess);
    }

    scheduler();
}

void PseudoClock()
{
    LDIT(PSECOND);

    int *pseudoclock_sem = &deviceSemaphores[SEMDEVLEN - 1];

    while (*pseudoclock_sem < 0)
    {
        (*pseudoclock_sem)++;
        pcb_t *p = removeBlocked(pseudoclock_sem);
        if (p != NULL)
        {
            insertProcQ(&readyQueue, p);
            softBlockCount--;
        }
    }

    if (currentProcess != NULL)
    {
        // ← AGGIUNGERE: addebita il tempo trascorso al processo corrente
        cpu_t timenow;
        STCK(timenow);
        currentProcess->p_time += (timenow - p_start);
        STCK(p_start); // resetta p_start per il prossimo intervallo

        LDST(saved_interrupt_state);
    }
    else
    {
        scheduler();
    }
}

void DeviceInterrupt(int line)
{
    // 1. Calcolo il device number (DevNo) leggendo la bitmap degli interrupt
    // La bitmap per la linea corrente si trova a 0x10000040 + ((line - 3) * 4)
    unsigned int *bitmap_addr = (unsigned int *)(0x10000040 + ((line - 3) * 4));
    unsigned int bitmap = *bitmap_addr;

    int DevNo = -1;
    for (int i = 0; i < 8; i++)
    {
        // Controllo quale bit è a 1 (da 0 a 7)
        if (bitmap & (1 << i))
        {
            DevNo = i;
            break; // Trovato il colpevole!
        }
    }

    // Sicurezza: se per qualche strano motivo non trovo device
    if (DevNo == -1)
    {
        // NON FARE RETURN! Torna tranquillamente a chi stavi eseguendo
        if (currentProcess != NULL)
            LDST(saved_interrupt_state);
        else
            scheduler();
    }

    // Calcolo l'indirizzo base dei registri fisici di questo device (formula delle specifiche)
    unsigned int devAddrBase = 0x10000054 + ((line - 3) * 0x80) + (DevNo * 0x10);

    unsigned int status;

    // Calcolo l'indice base per il semaforo (come hai fatto nella tua DoIO)
    int sem_index = (line - 3) * 8 + DevNo;

    // 2 & 3. Salvo lo status e faccio l'Acknowledge (ACK = scrivere 1 nel command)
    if (line == 7)
    {
        // È un Terminale! Dobbiamo capire se l'interrupt è dello schermo (Transmit) o tastiera (Receive)
        // Offset 8 = Transmit Status. Se lo status è 5 significa "Char Transmitted" (ha finito di stampare)
        unsigned int transm_status = *(unsigned int *)(devAddrBase + 8);

        if ((transm_status & 0xFF) == 5)
        {
            status = transm_status;                  // Salvo lo status
            *(unsigned int *)(devAddrBase + 12) = 1; // ACK sul Transmit Command (offset 12)
            sem_index += 8;                          // I semafori di Transmit sono spostati di 8
        }
        else
        {
            status = *(unsigned int *)(devAddrBase + 0); // Salvo il Receive Status (offset 0)
            *(unsigned int *)(devAddrBase + 4) = 1;      // ACK sul Receive Command (offset 4)
        }
    }
    else
    {
        // Altri device (Dischi, Stampanti, ecc.)
        status = *(unsigned int *)(devAddrBase + 0); // Lo status register è all'offset 0
        *(unsigned int *)(devAddrBase + 4) = 1;      // Il command register (ACK) è all'offset 4
    }

    // 4. Perform a V operation sul semaforo corretto
    int *sem_addr = &deviceSemaphores[sem_index];
    (*sem_addr)++;

    // Controlliamo se c'era un processo ad aspettare questo device
    if (*sem_addr <= 0)
    {
        pcb_t *p = removeBlocked(sem_addr);
        if (p != NULL)
        {
            // 5. Inserisco lo status dell'operazione nel registro a0 del processo svegliato
            p->p_s.reg_a0 = status;

            // 6. Inserisco il processo sbloccato nella Ready Queue
            insertProcQ(&readyQueue, p);
            softBlockCount--; // Tengo aggiornato il contatore dei bloccati per evitare i finti Deadlock
        }
    }
    // (Punto 8 della spec: se p è NULL o il semaforo era > 0, non facciamo nulla. Significa
    // che il processo che aspettava è stato terminato (TerminateProcess) nel frattempo).

    // 7. Ritorno del controllo (IDENTICO allo PseudoClock!)
    if (currentProcess != NULL)
    {
        // NON addebitiamo il tempo, NON lo mettiamo in coda. Ridiamo subito la CPU al
        // processo che stava girando, esattamente da dove l'interrupt lo aveva interrotto.
        cpu_t timenow;
        STCK(timenow);
        currentProcess->p_time += (timenow - p_start);
        STCK(p_start);

        LDST(saved_interrupt_state);
    }
    else
    {
        // Eravamo in WAIT() a causa dello scheduler. Ora che abbiamo appena svegliato
        // un processo (che voleva fare I/O), chiamiamo lo scheduler per farlo partire!
        scheduler();
    }
}