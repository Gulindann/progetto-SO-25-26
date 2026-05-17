#include "headers/syscalls.h"
#include "headers/initial.h"
#include "headers/scheduler.h"
#include "headers/traps.h"
#include "headers/utils.h"
#include "../phase1/headers/asl.h"
#include "../headers/types.h"
#include <uriscv/liburiscv.h>

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
    // Retrieve the processor state saved at syscall entry
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
            passUpOrDie(GENERALEXCEPT);
            break;
        }
    }
    else
    {
        saved_exception_state->cause = PRIVINSTR;

        passUpOrDie(GENERALEXCEPT);
    }
}

void CreateProcess(state_t *caller_state)
{
    // Allocate a new PCB for the child process
    pcb_t *INIT = allocPcb();

    if (INIT == NULL)
    {
        // PCB allocation failed
        caller_state->reg_a0 = -1;
    }
    else
    {
        // Initialize process state
        INIT->p_s = *(state_t *)caller_state->reg_a1;
        INIT->p_prio = caller_state->reg_a2;
        INIT->p_supportStruct = (support_t *)caller_state->reg_a3;

        // Insert the new process into the ready queue
        insertProcQ(&readyQueue, INIT);

        // Link child to current process
        insertChild(currentProcess, INIT);

        processCount++;

        // Return the PID of the new process
        caller_state->reg_a0 = INIT->p_pid;
    }

    // Advance PC
    caller_state->pc_epc += 4;

    LDST(caller_state);
}

// Auxiliary function
pcb_t *findPcbByPid(pcb_t *root, int target_pid)
{
    // Check current node
    if (root->p_pid == target_pid)
        return root;

    pcb_t *child;

    // DFS of the process tree
    list_for_each_entry(child, &root->p_child, p_sib)
    {
        pcb_t *found = findPcbByPid(child, target_pid);

        if (found != NULL)
            return found;
    }

    return NULL;
}

// Auxiliary function
void terminateProcessTree(pcb_t *p)
{
    // Recursively terminate all descendants
    while (!emptyChild(p))
    {
        terminateProcessTree(removeChild(p));
    }

    // If the process is currently running, clear currentProcess
    if (p == currentProcess)
    {
        currentProcess = NULL;
    }
    else
    {
        // Try removing the process from the ready queue
        if (outProcQ(&readyQueue, p) == NULL)
        {
            // If not in ready queue, it must be blocked
            if (p->p_semAdd != NULL)
            {
                int *sem = p->p_semAdd;

                // Remove process from semaphore queue
                outBlocked(p);

                // Restore semaphore value
                (*sem)++;

                // If it was blocked on a device semaphore
                if (sem >= &deviceSemaphores[0] &&
                    sem <= &deviceSemaphores[SEMDEVLEN - 1])
                {
                    softBlockCount--;
                }
            }
        }
    }

    // Release PCB and update process count
    freePcb(p);

    processCount--;
}

void TerminateProcess(state_t *caller_state)
{
    int target_pid = caller_state->reg_a1;

    pcb_t *target_pcb = NULL;

    // PID 0 means terminate the caller itself
    if (target_pid == 0)
    {
        target_pcb = currentProcess;
    }
    else
    {
        pcb_t *root = currentProcess;

        // Move upward to reach the root of the process tree
        while (root->p_parent != NULL)
        {
            root = root->p_parent;
        }

        // Search the target process recursively
        target_pcb = findPcbByPid(root, target_pid);
    }

    // Ignore invalid PIDs
    if (target_pcb == NULL)
    {
        caller_state->pc_epc += 4;

        LDST(caller_state);

        return;
    }

    // Disconnect the target from its parent
    outChild(target_pcb);

    // Remove the entire subtree rooted at target_pcb
    terminateProcessTree(target_pcb);

    // If the caller terminated itself, invoke scheduler
    if (currentProcess == NULL)
    {
        scheduler();
    }
    else
    {
        // Otherwise continue execution normally
        caller_state->pc_epc += 4;

        LDST(caller_state);
    }
}

void Passeren(state_t *caller_state)
{
    // Semaphore address passed by the caller
    int *sem_addr = (int *)caller_state->reg_a1;

    // Perform P operation
    (*sem_addr)--;

    // Resource available, continue execution
    if (*sem_addr >= 0)
    {
        caller_state->pc_epc += 4;

        LDST(caller_state);
    }
    else
    {
        // Block the current process
        caller_state->pc_epc += 4;

        updateCpuTime();

        // Save processor state before blocking
        currentProcess->p_s = *caller_state;

        // Insert process into semaphore queue
        insertBlocked(sem_addr, currentProcess);

        currentProcess = NULL;

        scheduler();
    }
}

void Verhogen(state_t *caller_state)
{
    // Semaphore address passed by the caller
    int *sem_addr = (int *)caller_state->reg_a1;

    // Perform V operation
    (*sem_addr)++;

    // Wake up one blocked process if present
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

    // Compute the device index from physical address
    int devIndex = (commandAddr - START_DEVREG) / 0x10;

    // Terminals use different semaphores for read/write operations
    if (devIndex >= 32)
    {
        if ((commandAddr - START_DEVREG) % 0x10 == 0xC)
        {
            devIndex += 8;
        }
    }

    int *sem_addr = &deviceSemaphores[devIndex];

    // Block the process waiting for I/O completion
    (*sem_addr)--;

    caller_state->pc_epc += 4;

    updateCpuTime();

    // Save current state before blocking
    currentProcess->p_s = *caller_state;

    insertBlocked(sem_addr, currentProcess);

    softBlockCount++;

    // Send command to device register
    unsigned int *physicalAddr = (unsigned int *)commandAddr;

    *physicalAddr = commandValue;

    currentProcess = NULL;

    scheduler();
}

void GetCPUTime(state_t *caller_state)
{
    cpu_t timenow;

    // Read current TOD clock
    STCK(timenow);

    // Update accumulated CPU usage
    currentProcess->p_time += (timenow - p_start);

    // Return execution time scaled to microseconds
    caller_state->reg_a0 =
        (currentProcess->p_time) / (*((cpu_t *)TIMESCALEADDR));

    caller_state->pc_epc += 4;

    LDST(caller_state);
}

void WaitForClock(state_t *caller_state)
{
    // Last semaphore reserved for pseudo-clock synchronization
    int *pseudoclock_sem = &deviceSemaphores[SEMDEVLEN - 1];

    (*pseudoclock_sem)--;

    caller_state->pc_epc += 4;

    updateCpuTime();

    // Save current process state before blocking
    currentProcess->p_s = *caller_state;

    insertBlocked(pseudoclock_sem, currentProcess);

    softBlockCount++;

    currentProcess = NULL;

    scheduler();
}

void GetSupportData(state_t *caller_state)
{
    // Return pointer to the support structure of current process
    caller_state->reg_a0 =
        (unsigned int)currentProcess->p_supportStruct;

    caller_state->pc_epc += 4;

    LDST(caller_state);
}

void GetProcessId(state_t *caller_state)
{
    int pid = currentProcess->p_pid;

    // If requested, return parent PID
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
    // Advance PC
    caller_state->pc_epc += 4;

    updateCpuTime();

    // Save process state before rescheduling
    currentProcess->p_s = *caller_state;

    // Reinsert process at the end of the ready queue
    list_add_tail(&currentProcess->p_list, &readyQueue);

    currentProcess = NULL;

    scheduler();
}