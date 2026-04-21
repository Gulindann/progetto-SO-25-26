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
            trapExceptionHandler(excCode);
            break;
        }
    }
    else
    {
        // User mode: simulare Program Trap
        trapExceptionHandler(excCode);
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

        insertProcQ(&READY_Q, INIT);
        insertChild(CURRENT_P, INIT);
        PROC_C++;

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
    while (!emptyChild(p))
    {
        terminateProcessTree(removeChild(p));
    }

    if (p != CURRENT_P)
    {
        if (outProcQ(&READY_Q, p) == NULL)
        {
            int *sem = p->p_semAdd;
            outBlocked(p);

            if (sem >= &SEM_DEV_Q[0] && sem <= &SEM_DEV_Q[SEMDEVLEN - 1])
            {
                SBLOCK_C--;
            }
        }
    }

    freePcb(p);
    PROC_C--;
}

void TerminateProcess(state_t *caller_state)
{
    int target_pid = caller_state->reg_a1;
    pcb_t *target_pcb = NULL;

    if (target_pid == 0)
    {
        target_pcb = CURRENT_P;
    }
    else
    {
        pcb_t *root = CURRENT_P;
        while (root->p_parent != NULL)
        {
            root = root->p_parent;
        }
        target_pcb = findPcbByPid(root, target_pid);
    }

    if (target_pcb == NULL)
    {
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }

    outChild(target_pcb);
    terminateProcessTree(target_pcb);

    if (target_pcb == CURRENT_P)
    {
        scheduler();
    }
    else
    {
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }
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

        cpu_t timenow;
        STCK(timenow);
        CURRENT_P->p_time += (timenow - p_start);

        CURRENT_P->p_s = *caller_state;
        insertBlocked(sem_addr, CURRENT_P);
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
            insertProcQ(&READY_Q, p);
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

    int *sem_addr = &SEM_DEV_Q[devIndex];
    (*sem_addr)--;

    caller_state->pc_epc += 4;

    cpu_t timenow;
    STCK(timenow);
    CURRENT_P->p_time += (timenow - p_start);
    CURRENT_P->p_s = *caller_state;

    insertBlocked(sem_addr, CURRENT_P);
    SBLOCK_C++;

    unsigned int *physicalAddr = (unsigned int *)commandAddr;
    *physicalAddr = commandValue;

    scheduler();
}

void GetCPUTime(state_t *caller_state)
{
    cpu_t timenow;
    STCK(timenow);

    caller_state->reg_a0 = CURRENT_P->p_time + (timenow - p_start);

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void WaitForClock(state_t *caller_state)
{
    int *pseudoclock_sem = &SEM_DEV_Q[SEMDEVLEN - 1];
    (*pseudoclock_sem)--;

    caller_state->pc_epc += 4;

    cpu_t timenow;
    STCK(timenow);
    CURRENT_P->p_time += (timenow - p_start);

    CURRENT_P->p_s = *caller_state;

    insertBlocked(pseudoclock_sem, CURRENT_P);
    SBLOCK_C++;

    scheduler();
}

void GetSupportData(state_t *caller_state)
{
    caller_state->reg_a0 = (unsigned int)CURRENT_P->p_supportStruct;
    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void GetProcessId(state_t *caller_state)
{
    int pid = CURRENT_P->p_pid;
    if (caller_state->reg_a1 != 0)
    {
        if (CURRENT_P->p_parent == NULL)
            pid = 0;
        else
            pid = CURRENT_P->p_parent->p_pid;
    }

    caller_state->reg_a0 = pid;

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void Yield(state_t *caller_state)
{
    caller_state->pc_epc += 4;

    cpu_t timenow;
    STCK(timenow);
    CURRENT_P->p_time += (timenow - p_start);

    CURRENT_P->p_s = *caller_state;

    insertProcQ(&READY_Q, CURRENT_P);

    // list_add_tail(&READY_Q, CURRENT_P); // Vuole essere ultimo in coda anche se con max priority prendere containerof di currentp TODO

    scheduler();
}