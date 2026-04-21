#include "headers/exceptions.h"

void exceptionHandler()
{
    // 1. Leggo l'intero Cause Register
    unsigned int cause_reg = getCAUSE();

    if (CAUSE_IS_INT(cause_reg))
    {
        interruptHandler(cause_reg);

        // Se arrivi qui, interruptHandler ha fallito a fare la LDST.
        // Togli il return; e metti PANIC così capiamo se ci sfugge un interrupt!
        PANIC();
    }

    // 3. Estraggo l'Exception Code "pulito" mascherando i bit del Cause Register
    unsigned int excCode = cause_reg & CAUSE_EXCCODE_MASK;

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