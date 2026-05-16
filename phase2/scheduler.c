#include "headers/initial.h"
#include "uriscv/liburiscv.h"

cpu_t p_start;

void scheduler()
{
    while (1)

        if (!emptyProcQ(&readyQueue))
        {
            // Caricamento processo

            currentProcess = removeProcQ(&readyQueue);

            setTIMER(TIMESLICE);
            STCK(p_start);
            LDST(&currentProcess->p_s);
        }
        else
        {
            if (processCount == 0) // Nessun processo in esecuzione
            {
                HALT();
            }
            else if (processCount > 0 && softBlockCount == 0) // Deadlock
            {
                PANIC();
            }
            else if (processCount > 0 && softBlockCount > 0) // Attesa di device interrupt
            {
                currentProcess = NULL;

                setMIE(MIE_ALL & ~MIE_MTIE_MASK);
                unsigned int status = getSTATUS();
                status |= MSTATUS_MIE_MASK;
                setSTATUS(status);

                WAIT();
            }
        }
}
