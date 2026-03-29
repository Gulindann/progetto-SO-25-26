#include "headers/exceptions.h"
#include "headers/initial.h"
#include "headers/scheduler.h"
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

void trapExceptionHandler(int excCode)
{
}

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
            PANIC();
            
        }
    }
    else
    {
        // User mode: simulare Program Trap
        return;
    }
}

void CreateProcess(state_t *caller_state)
{
    pcb_t *INIT = allocPcb();
    if (INIT == NULL)
    {
        caller_state->reg_a0 = -1; // Ritorna -1 se fallisce
    }
    else
    {

        INIT->p_s = *(state_t *)caller_state->reg_a1;
        INIT->p_prio = caller_state->reg_a2;
        INIT->p_supportStruct = (support_t *)caller_state->reg_a3;

        insertProcQ(&READY_Q, INIT);
        insertChild(CURRENT_P, INIT);
        PROC_C++;

        caller_state->reg_a0 = INIT->p_pid; // Ritorna il PID in a0
    }

    // Incremento il PC per non rifare la syscall
    caller_state->pc_epc += 4;
    LDST(caller_state); // Torna al processo chiamante
}

// --- Funzioni di supporto per TerminateProcess ---

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

            // Se l'indirizzo del semaforo è dentro l'array dei device, decrementa SBLOCK_C
            if (sem >= &SEM_DEV_Q[0] && sem <= &SEM_DEV_Q[SEMDEVLEN - 1]) // Controllo se id del mio semaforo si trova tra i semafori device a livello di memoria
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
        // Se non trovo il processo, proseguo semplicemente
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
        // Ho ucciso un altro processo
        caller_state->pc_epc += 4; // Incremento il PC
        LDST(caller_state);
    }
}

void Passeren(state_t *caller_state){
    int *sem_addr = caller_state->reg_a1; // Indirizzo semaforo
    (*sem_addr)--;

    if(*sem_addr >= 0){
        caller_state->pc_epc += 4;
        LDST(caller_state);
    }else{
        // Semaforo va sotto 0 quindi si blocca
        caller_state->pc_epc += 4;
        insertBlocked(sem_addr, CURRENT_P);

        // Aggiorno p_time
        cpu_t timenow;
        STCK(timenow);

        CURRENT_P->p_time = CURRENT_P->p_time + (timenow - p_start);

        // Salvo nuovo stato
        CURRENT_P->p_s = *caller_state;
        
        scheduler();
    }


}

void Verhogen(state_t *caller_state){
    int *sem_addr = caller_state->reg_a1;
    (*sem_addr)++;

    // Sveglio il processo
    if(*sem_addr >= 0){
        pcb_t *p = removeBlocked(sem_addr); // Rimuovo dalla coda dei bloccati

    insertProcQ(&READY_Q, p); // Inserisco nella readyQ
    }

    caller_state->pc_epc += 4;
    LDST(caller_state);
}

void DoIo(state_t *caller_state){
    caller_state->pc_epc += 4;

    // Aggiorno p_time
    cpu_t timenow;
    STCK(timenow);

    CURRENT_P->p_time = CURRENT_P->p_time + (timenow - p_start);

    // Salvo nuovo stato
    CURRENT_P->p_s = *caller_state;


    scheduler();

}

void GetCPUTime(state_t *caller_state){
    cpu_t timenow;
    STCK(timenow);

    caller_state->reg_a0 = CURRENT_P->p_time + (timenow - p_start);

    // Sempre aumentare PC e far ripartire processo se non bloccante
    caller_state->pc_epc += 4;
    LDST(caller_state);
}


void WaitForClock(state_t *caller_state){

}


void GetSupportData(state_t *caller_state){

}


void GetProcessId(state_t *caller_state){
    int pid = CURRENT_P->p_pid;
    if(caller_state->reg_a1 != 0){
        if(CURRENT_P->p_parent == NULL)
            pid = 0;
        else
            pid = CURRENT_P->p_parent->p_pid;
        
    }

    caller_state->reg_a0 = pid;

    // Sempre aumentare PC e far ripartire processo se non bloccante
    caller_state->pc_epc += 4;
    LDST(caller_state);
}


void Yield(state_t *caller_state){

}