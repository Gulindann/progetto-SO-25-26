#include "headers/initial.h"
#include "headers/scheduler.h"
#include "uriscv/liburiscv.h"
#include "headers/exceptions.h"

int processCount;
int softBlockCount;
struct list_head readyQueue;
pcb_t *currentProcess;
int deviceSemaphores[SEMDEVLEN];

extern void uTLB_RefillHandler();
extern void test();

int main()
{
    // Popolamento del Pass Up Vector per il Processore 0
    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    // Inizializzazione PCB e semafori
    initPcbs();
    initASL();

    // Inizializzazione delle variabili globali
    processCount = 0;
    softBlockCount = 0;
    mkEmptyProcQ(&readyQueue);
    currentProcess = NULL;

    // Inizializzazione semafori dei device
    for (int i = 0; i < SEMDEVLEN; i++)
    {
        deviceSemaphores[i] = 0;
    }

    // Interval Timer
    LDIT(PSECOND);

    // Allocazione primo processo
    pcb_t *initProcess = allocPcb();
    if (initProcess != NULL)
    {
        insertProcQ(&readyQueue, initProcess);
        processCount++;

        RAMTOP(initProcess->p_s.reg_sp);
        initProcess->p_s.mie = MIE_ALL;
        initProcess->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
        initProcess->p_s.pc_epc = (memaddr)test;
    }

    scheduler();

    return 0;
}