#include "headers/initial.h"
#include "uriscv/liburiscv.h"

cpu_t p_start;

void scheduler()
{
    while (1)

        if (!emptyProcQ(&READY_Q))
        {
            CURRENT_P = removeProcQ(&READY_Q);

            LDIT(TIMESLICE);
            STCK(p_start);
            LDST(&CURRENT_P->p_s);
        }
        else
        {
            if (PROC_C == 0) // Nessun processo in esecuzione
            {
                // Termina
                HALT();
            }
            else if (PROC_C > 0 && SBLOCK_C == 0) // Deadlock
            {
                PANIC();
            }
            else if (PROC_C > 0 && SBLOCK_C > 0) // Attesa di device interrupt
            {
                // Ringraziamento speciale al carissimo e stimatissimo Dott. Rovelli
                setMIE(MIE_ALL & ~MIE_MTIE_MASK);
                unsigned int status = getSTATUS();
                status |= MSTATUS_MIE_MASK;
                setSTATUS(status);

                WAIT();
            }
        }
}
