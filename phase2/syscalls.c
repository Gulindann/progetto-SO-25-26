#include "headers/syscalls.h"
#include "headers/initial.h"
#include "headers/scheduler.h"
#include "headers/traps.h"
#include "uriscv/liburiscv.h"

// Prototipi interni per mantenere l'ordine
void CreateProcess(state_t *caller_state);
void TerminateProcess(state_t *caller_state);
void Passeren(state_t *caller_state);
void Verhogen(state_t *caller_state);
void DoIO(state_t *caller_state);
void GetCPUTime(state_t *caller_state);
void WaitForClock(state_t *caller_state);
void GetSupportData(state_t *caller_state);
void GetProcessId(state_t *caller_state);
void Yield(state_t *caller_state);
pcb_t *findPcbByPid(pcb_t *root, int target_pid);
void terminateProcessTree(pcb_t *p);

void syscallExceptionHandler(int excCode)
{
    state_t *saved_exception_state = GET_EXCEPTION_STATE_PTR(0);
    unsigned int mode = saved_exception_state->status & MSTATUS_MPP_MASK;

    if (mode == MSTATUS_MPP_M)
    {
        int SYS_ID = saved_exception_state->reg_a0;
        switch (SYS_ID)
        {
        case CREATEPROCESS:
            CreateProcess(saved_exception_state);
            break;
        case TERMPROCESS:
            TerminateProcess(saved_exception_state);
            break;
        case PASSEREN:
            Passeren(saved_exception_state);
            break;
        case VERHOGEN:
            Verhogen(saved_exception_state);
            break;
        case DOIO:
            DoIO(saved_exception_state);
            break;
        case GETTIME:
            GetCPUTime(saved_exception_state);
            break;
        case CLOCKWAIT:
            WaitForClock(saved_exception_state);
            break;
        case GETSUPPORTPTR:
            GetSupportData(saved_exception_state);
            break;
        case GETPROCESSID:
            GetProcessId(saved_exception_state);
            break;
        case YIELD:
            Yield(saved_exception_state);
            break;
        default:
            // Syscall non riconosciuta: Pass Up or Die simulando una Trap
            passUpOrDie(GENERALEXCEPT);
            break;
        }
    }
    else
    {
        // User mode: simulare Program Trap
        passUpOrDie(GENERALEXCEPT);
    }
}

void CreateProcess(state_t *caller_state)
{
    pcb_t *INIT = allocPcb();
    if (INIT == NULL)
    {
        caller_state->reg_a0 = -1;
    }
    else
    {
        INIT->p_s = *(state_t *)caller_state->reg_a1;
        INIT->p_prio = caller_state->reg_a2;
        INIT->p_supportStruct = (support_t *)caller_state->reg_a3;

        insertProcQ(&readyQueue, INIT);
        insertChild(currentProcess, INIT);
        processCount++;

        caller_state->reg_a0 = INIT->p_pid;
    }

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

pcb_t *findPcbByPid(pcb_t *root, int target_pid)
{
    if (root->p_pid == target_pid)
        return root;

    pcb_t *child;
    list_for_each_entry(child, &root->p_child, p_sib)
    {
        pcb_t *found = findPcbByPid(child, target_pid);
        if (found != NULL)
            return found;
    }
    return NULL;
}

void terminateProcessTree(pcb_t *p)
{
    // 1. Uccidiamo tutti i figli ricorsivamente
    while (!emptyChild(p))
    {
        terminateProcessTree(removeChild(p));
    }

    // 2. Rimuoviamo il processo dalle code in cui si trova
    if (p == currentProcess)
    {
        // Se stiamo uccidendo il processo in esecuzione, azzeriamo il puntatore!
        currentProcess = NULL;
    }
    else
    {
        // Se non è CURRENT_P, deve essere in READY_Q oppure Bloccato
        if (outProcQ(&readyQueue, p) == NULL)
        {
            // Non era in READY_Q, quindi è bloccato su un semaforo
            if (p->p_semAdd != NULL)
            {
                int *sem = p->p_semAdd;
                outBlocked(p); // Lo scolleghiamo dalla coda del semaforo

                (*sem)++;

                // Se era un semaforo dei device o del clock, aggiorniamo SBLOCK_C
                if (sem >= &deviceSemaphores[0] && sem <= &deviceSemaphores[SEMDEVLEN - 1])
                {
                    softBlockCount--;
                }
            }
        }
    }

    // 3. Lo rimettiamo tra i PCB liberi
    freePcb(p);
    processCount--;
}

void TerminateProcess(state_t *caller_state)
{
    int target_pid = caller_state->reg_a1;
    pcb_t *target_pcb = NULL;

    // 1. Troviamo il PCB bersaglio
    if (target_pid == 0)
    {
        target_pcb = currentProcess;
    }
    else
    {
        pcb_t *root = currentProcess;
        // Risaliamo fino al vero "root" dell'albero come da tua logica
        while (root->p_parent != NULL)
        {
            root = root->p_parent;
        }
        target_pcb = findPcbByPid(root, target_pid);
    }

    // Se non troviamo il processo, ignoriamo e andiamo avanti
    if (target_pcb == NULL)
    {
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }

    // 2. Lo sganciamo dall'albero genealogico genitore
    outChild(target_pcb);

    // 3. Uccidiamo l'intero albero radicato in target_pcb
    terminateProcessTree(target_pcb);

    // 4. Se il processo che stava chiamando la Syscall è morto (o perché si è
    //    suicidato, o perché ha ucciso un genitore), chiamiamo lo scheduler.
    if (currentProcess == NULL)
    {
        scheduler();
    }
    else
    {
        // Se ha ucciso qualcun altro ma lui è sopravvissuto, riprende l'esecuzione.
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }
}

int isDescendant(pcb_t *ancestor, pcb_t *p)
{
    if (p == NULL)
        return 0;
    if (p->p_parent == ancestor)
        return 1;
    return isDescendant(ancestor, p->p_parent);
}

void Passeren(state_t *caller_state)
{
    int *sem_addr = (int *)caller_state->reg_a1;
    (*sem_addr)--;

    if (*sem_addr >= 0)
    {
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }
    else
    {
        caller_state->pc_epc += 4;

        updateCpuTime();

        currentProcess->p_s = *caller_state;
        insertBlocked(sem_addr, currentProcess);
        currentProcess = NULL;
        scheduler();
    }
}

void Verhogen(state_t *caller_state)
{
    int *sem_addr = (int *)caller_state->reg_a1;
    (*sem_addr)++;

    if (*sem_addr <= 0)
    {
        pcb_t *p = removeBlocked(sem_addr);
        if (p != NULL)
        {
            insertProcQ(&readyQueue, p);
        }
    }

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void DoIO(state_t *caller_state)
{
    unsigned int commandAddr = caller_state->reg_a1;
    unsigned int commandValue = caller_state->reg_a2;

    int devIndex = (commandAddr - START_DEVREG) / 0x10;

    if (devIndex >= 32)
    {
        if ((commandAddr - START_DEVREG) % 0x10 == 0xC)
        {
            devIndex += 8;
        }
    }

    int *sem_addr = &deviceSemaphores[devIndex];
    (*sem_addr)--;

    caller_state->pc_epc += 4;

    updateCpuTime();

    currentProcess->p_s = *caller_state;

    insertBlocked(sem_addr, currentProcess);
    softBlockCount++;

    unsigned int *physicalAddr = (unsigned int *)commandAddr;
    *physicalAddr = commandValue;

    currentProcess = NULL;
    scheduler();
}

void GetCPUTime(state_t *caller_state)
{
    updateCpuTime();

    caller_state->reg_a0 = (currentProcess->p_time) / (*((cpu_t *)TIMESCALEADDR));

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void WaitForClock(state_t *caller_state)
{
    int *pseudoclock_sem = &deviceSemaphores[SEMDEVLEN - 1];
    (*pseudoclock_sem)--;

    caller_state->pc_epc += 4;

    updateCpuTime();

    currentProcess->p_s = *caller_state;

    insertBlocked(pseudoclock_sem, currentProcess);
    softBlockCount++;

    currentProcess = NULL;
    scheduler();
}

void GetSupportData(state_t *caller_state)
{
    caller_state->reg_a0 = (unsigned int)currentProcess->p_supportStruct;
    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void GetProcessId(state_t *caller_state)
{
    int pid = currentProcess->p_pid;
    if (caller_state->reg_a1 != 0)
    {
        if (currentProcess->p_parent == NULL)
            pid = 0;
        else
            pid = currentProcess->p_parent->p_pid;
    }

    caller_state->reg_a0 = pid;

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void Yield(state_t *caller_state)
{
    caller_state->pc_epc += 4;

    updateCpuTime();

    currentProcess->p_s = *caller_state;

    insertProcQ(&deviceSemaphores, currentProcess);

    // list_add_tail(&READY_Q, CURRENT_P); // Vuole essere ultimo in coda anche se con max priority prendere containerof di currentp TODO
    currentProcess = NULL;
    scheduler();
}