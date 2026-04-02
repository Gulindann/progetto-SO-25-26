#include "headers/interrupts.h"
#include "uriscv/liburiscv.h"

state_t *saved_interrupt_state;

void PLT();
void PseudoClock();
void DeviceInterrupt(int line);

void interruptHandler(unsigned int cause_reg)
{
    saved_interrupt_state = GET_EXCEPTION_STATE_PTR(0);

    unsigned int line = cause_reg & CAUSE_EXCCODE_MASK;

    if (line == 1)
    {
        PLT();
    }
    else if (line == 2)
    {
        PseudoClock();
    }
    else if (line >= 3 && line <= 7)
    {
        DeviceInterrupt(line);
    }
}

void PLT()
{

    LDIT(TIMESLICE);
    if (CURRENT_P != NULL)
    {
        CURRENT_P->p_s = *saved_interrupt_state;

        cpu_t timenow;
        STCK(timenow);
        CURRENT_P->p_time += (timenow - p_start);

        insertProcQ(&READY_Q, CURRENT_P);
    }

    scheduler();
}
void PseudoClock() {}
void DeviceInterrupt(int line) {}