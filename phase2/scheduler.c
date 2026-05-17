#include "headers/scheduler.h"
#include "headers/initial.h"
#include <uriscv/liburiscv.h>

// Timer start value for CPU time accounting
cpu_t p_start;

void scheduler()
{
    while (1)
    {
        if (!emptyProcQ(&readyQueue))
        {
            // Dispatch the next process in the ready queue
            currentProcess = removeProcQ(&readyQueue);

            // Load the time slice (5ms) into the PLT
            setTIMER(TIMESLICE);

            STCK(p_start);
            LDST(&currentProcess->p_s);
        }
        else
        {
            // Empty ready queue
            if (processCount == 0)
            {
                HALT();
            }
            else if (processCount > 0 && softBlockCount == 0) // Deadlock
            {

                PANIC();
            }
            else if (processCount > 0 && softBlockCount > 0) // Wait for pending I/O interrupts
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
}