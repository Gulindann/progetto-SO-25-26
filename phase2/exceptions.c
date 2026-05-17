#include "headers/exceptions.h"
#include "headers/syscalls.h"
#include "headers/tlb.h"
#include "headers/traps.h"
#include "headers/interrupts.h"
#include "../headers/const.h"
#include <uriscv/cpu.h>
#include <uriscv/liburiscv.h>

void exceptionHandler()
{
    // Read the cause register
    unsigned int causeReg = getCAUSE();

    // Pass control to interrupt handler if it's an interrupt
    if (CAUSE_IS_INT(causeReg))
    {
        interruptHandler(causeReg);

        // System should not reach here
        PANIC();
    }

    // Extract the exception code
    unsigned int excCode = causeReg & CAUSE_EXCCODE_MASK;

    // Dispatch the exception to the appropriate handler
    switch (excCode)
    {
    case SYSEXCEPTION: // 8
    case 11:
        syscallExceptionHandler(excCode);
        break;
    case 24 ... 28:
        tlbExceptionHandler(excCode);
        break;
    default:
        trapExceptionHandler(excCode);
        break;
    }
}