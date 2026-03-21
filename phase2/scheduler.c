#include "headers/initial.h"
#include "uriscv/liburiscv.h"

void scheduler()
{
    while (1)
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
        else // Caso predefinito, vado avanti con l'esecuzione
        {
            // Forse il processo é da rimettere in coda e pulire

            CURRENT_P = removeProcQ(&READY_Q); // Primo processo della Queue diventa il current process

            LDIT(TIMESLICE);
            LDST(&CURRENT_P->p_s);
        }
    }
}
