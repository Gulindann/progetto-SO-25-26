#include "headers/exceptions.h"
#include "headers/initial.h"
#include "uriscv/liburiscv.h"

void exceptionHandler()
{
    // 1. Leggo l'intero Cause Register [cite: 129]
    unsigned int cause_reg = getCAUSE();

    // 2. Controllo se è un interrupt passando il registro alla macro
    if (CAUSE_IS_INT(cause_reg))
    {
        interruptHandler(cause_reg); // Hardware, timer o periferiche
        return;
    }

    // 3. Estraggo l'Exception Code "pulito" mascherando i bit del Cause Register
    unsigned int excCode = cause_reg & GETEXECCODE;

    // 4. Guardo il codice dell'eccezione, faccio lo switch e smisto sulle altre funzioni
    switch (excCode) // Software
    {
    case 8:
    case 11:
        syscallExceptionHandler(excCode);
        break;

    case 24 ... 28:
        // Gestione TLB
        tlbExceptionHandler(excCode);
        break;

    case 0 ... 7:
    case 9:
    case 10:
    case 12 ... 23:
        // Gestione Program Traps
        trapExceptionHandler(excCode);
        break;

    default:
        // Una buona pratica nei SO è chiamare PANIC() se l'eccezione non è riconosciuta
        PANIC();
        break;
    }
}

void interruptHandler(int excCode)
{
}

void tlbExceptionHandler(int excCode)
{
}

void syscallExceptionHandler(int excCode)
{
    // Devo controllare se il processo é in kernel mode...

    state_t *saved_exception_state = GET_EXCEPTION_STATE_PTR(0);

    unsigned int mode = saved_exception_state->status & MSTATUS_MPP_MASK;

    if (mode == MSTATUS_MPP_M)
    {
        // Devo ricavare il valore del registro general purpose a0...
        // Switch su tutte le system call
        unsigned int SYS_ID = saved_exception_state->gpr[24];
        switch (SYS_ID)
        {
        case -1:
            CreateProcess(&saved_exception_state);
            break;
        case -2:
        case -3:
        case -4:
        case -5:
        case -6:
        default:
        }
    }
    else
    { // User mode. Hai violato la legge!
        return;
    }
}

void trapExceptionHandler(int excCode)
{
}

// Funzioni system call (poi in altro file)
void CreateProcess(state_t *caller_state)
{
    pcb_t *INIT = allocPcb();
    if (INIT == NULL) // Se non ci sono PCB liberi non puó venir creato un nuovo processo
    {
        caller_state->gpr[24] = -1;
        return;
    }
    else
    {
        INIT->p_s = *(state_t *)caller_state->gpr[25];              // a1
        INIT->p_prio = *(int *)caller_state->gpr[26];               // a2
        INIT->p_supportStruct = (support_t *)caller_state->gpr[27]; // a3
        // ID assegnato in allocPcb
        insertProcQ(&READY_Q, INIT);
        insertChild(&CURRENT_P->p_child, INIT);
        PROC_C++;
        INIT->p_time = 0;      // Giá settato a 0 in allocPcb
        INIT->p_semAdd = NULL; // Giá settato a NULL in allocPcb
    }
}

void TerminateProcess(state_t *caller_state)
{
    unsigned int PPID = *(int *)caller_state->gpr[25];
    if (PPID == 0)
    {
        outChild(CURRENT_P); // Stacco il processo da suo padre
        pcb_t *tmp = CURRENT_P;
        // Cosa per distruggere tutto
        SterminatorePazzoAssassinoKillerAssurdoTrumpDioMostroDiFirenzeJackLoSquartatoreDiPcb(tmp);
    }
    else
    {
        pcb_t *iter;
        pcb_t *tmp = NULL;

        // Trova processo
        list_for_each_entry(iter, &READY_Q, p_list)
        {
            if (iter->p_pid == PPID)
            {
                tmp = iter;
            }
        }
        if (tmp == NULL)
        {
            return;
        }

        // Cosa per distruggere tutto
        SterminatorePazzoAssassinoKillerAssurdoTrumpDioMostroDiFirenzeJackLoSquartatoreDiPcb(tmp);
    }
}

void SterminatorePazzoAssassinoKillerAssurdoTrumpDioMostroDiFirenzeJackLoSquartatoreDiPcb(pcb_t *p)
{
    // 1. Finché il processo ha figli, stacco il primo e lo stermino
    while (!emptyChild(p))
    {
        pcb_t *figlio = removeChild(p);
        SterminatorePazzoAssassinoKillerAssurdoTrumpDioMostroDiFirenzeJackLoSquartatoreDiPcb(figlio); // Ricorsione: scendo fino alla foglia
    }

    if (p == CURRENT_P)
    {
        // È il processo attualmente in esecuzione, non è in nessuna coda
    }
    else if (outProcQ(&READY_Q, p) != NULL)
    {
        // Era nella Ready Queue (outProcQ lo ha appena rimosso)
    }
    else
    {
        // Se non era né il CURRENT_P né nella Ready_Q, è per forza bloccato in un semaforo!
        int *sem = p->p_semAdd;
        outBlocked(p); // Rimuove dalla coda dei bloccati (ASL)

        // ATTENZIONE: Se era bloccato su un semaforo di Device o Timer,
        // devi ricordarti di decrementare SBLOCK_C
        if (sem >= &SEM_DEV_Q[0].s_key && sem <= &SEM_DEV_Q[SEMDEVLEN - 1].s_key)
        {
            SBLOCK_C--;
        }
    }

    // 3. Il colpo di grazia
    freePcb(p);
    PROC_C--; // Meno un processo nel sistema globale
}