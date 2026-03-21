#include "headers/initial.h"

int PROC_C ;
int SBLOCK_C;
static struct list_head READY_Q;
pcb_t* CURRENT_P;
static semd_t SEM_DEV_Q[SEMDEVLEN]; // ALTRIMENTI 7 = 5 device + 1 pseudo-clock + 1 terminale

extern void test();

int main(){
    
    passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR;
    //passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refill_stackPtr=KERNELSTACK;
    //passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr=KERNELSTACK;

    initPcbs();
    initASL();

    PROC_C = 0;
    SBLOCK_C = 0;
    mkEmptyProcQ(&READY_Q);
    CURRENT_P = NULL;
    
    for(int i=0; i<SEMDEVLEN; i++){
        SEM_DEV_Q[i].s_key = NULL; 
        INIT_LIST_HEAD(&SEM_DEV_Q[i].s_procq); // Initialize process queue
        INIT_LIST_HEAD(&SEM_DEV_Q[i].s_link);  // Initialize list link       
    }

    LDIT(PSECOND);

    pcb_t* INIT = allocPcb(); // Setta tutto a 0 o NULL
    insertProcQ(&READY_Q, INIT);

    PROC_C++;
    RAMTOP(INIT->p_s.gpr[2]); 
    INIT->p_s.mie = MIE_ALL;
    INIT->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
    INIT->p_s.pc_epc = (memaddr) test;

    // Chiama Scheduler
    

}