#include "headers/interrupts.h"
#include "uriscv/liburiscv.h"

void PLT();
void PseudoClock();
void DeviceInterrupt(int line);

void interruptHandler(unsigned int cause_reg)
{
    unsigned int line = cause_reg & GETEXECCODE;

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

void PLT() {}
void PseudoClock() {}
void DeviceInterrupt(int line) {}