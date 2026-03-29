#include "headers/initial.h"
#include "headers/scheduler.h"

int PROC_C;
int SBLOCK_C;
struct list_head READY_Q;
pcb_t *CURRENT_P;
int SEM_DEV_Q[SEMDEVLEN];

extern void test();

void uTLB_RefillHandler()
{
    int prid = getPRID();
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((state_t *)BIOSDATAPAGE);
}

int main()
{

    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
    // passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = KERNELSTACK;

    initPcbs();
    initASL();

    PROC_C = 0;
    SBLOCK_C = 0;
    mkEmptyProcQ(&READY_Q);
    CURRENT_P = NULL;

    for (int i = 0; i < SEMDEVLEN; i++)
    {
        SEM_DEV_Q[i] = 0; // I semafori sono interi inizializzati a 0
    }

    LDIT(PSECOND);

    pcb_t *INIT = allocPcb(); // Setta tutto a 0 o NULL
    insertProcQ(&READY_Q, INIT);

    PROC_C++;
    RAMTOP(INIT->p_s.reg_sp);
    INIT->p_s.mie = MIE_ALL;
    INIT->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
    INIT->p_s.pc_epc = (memaddr)test;

    scheduler();
}