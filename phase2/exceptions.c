#include "headers/exceptions.h"
#include "headers/syscalls.h"
#include "headers/tlb.h"
#include "headers/traps.h"
#include "headers/interrupts.h" // Assumendo che tu abbia già questo header
#include "uriscv/liburiscv.h"
#include "../../headers/const.h"

void exceptionHandler()
{
    // 1. Leggo l'intero Cause Register
    unsigned int cause_reg = getCAUSE();

    // 2. Controllo se è un interrupt passando il registro alla macro
    if (CAUSE_IS_INT(cause_reg))
    {
        interruptHandler(cause_reg); // Hardware, timer o periferiche
        return;
    }

    // 3. Estraggo l'Exception Code "pulito" mascherando i bit del Cause Register
    unsigned int excCode = cause_reg & GETEXECCODE;

    // 4. Guardo il codice dell'eccezione, faccio lo switch e smisto sulle altre funzioni
    switch (excCode)
    {
    case 8:
    case 11:
        syscallExceptionHandler(excCode);
        break;

    case 24 ... 28:
        tlbExceptionHandler(excCode);
        break;

    case 0 ... 7:
    case 9:
    case 10:
    case 12 ... 23:
        trapExceptionHandler(excCode);
        break;

    default:
        PANIC();
        break;
    }
}