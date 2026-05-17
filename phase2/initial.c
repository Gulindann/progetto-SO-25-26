#include "headers/initial.h"
#include "headers/scheduler.h"
#include "headers/exceptions.h"
#include "../phase1/headers/asl.h"
#include <uriscv/liburiscv.h>

// Phase 2 global variables
int processCount;
int softBlockCount;
struct list_head readyQueue;
pcb_t *currentProcess;
int deviceSemaphores[SEMDEVLEN];

extern void uTLB_RefillHandler();
extern void test();

int main()
{
    // Populate the Processor 0 Pass Up Vector
    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    // Initialize Phase 1 data structures
    initPcbs();
    initASL();

    // Initialize global state variables
    processCount = 0;
    softBlockCount = 0;
    mkEmptyProcQ(&readyQueue);
    currentProcess = NULL;

    // Initialize device semaphores to zero (for synchronization)
    for (int i = 0; i < SEMDEVLEN; i++)
    {
        deviceSemaphores[i] = 0;
    }

    // Load the system-wide Interval Timer (100ms)
    LDIT(PSECOND);

    // Instantiate the first process
    pcb_t *initProcess = allocPcb();
    if (initProcess != NULL)
    {
        insertProcQ(&readyQueue, initProcess);
        processCount++;

        // Setup initial processor state for the test process
        RAMTOP(initProcess->p_s.reg_sp);
        initProcess->p_s.mie = MIE_ALL;
        initProcess->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M; // Kernel mode
        initProcess->p_s.pc_epc = (memaddr)test;
    }

    // Start the scheduler
    scheduler();

    return 0;
}