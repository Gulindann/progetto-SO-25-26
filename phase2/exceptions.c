#include "headers/exceptions.h"
#include "headers/initial.h"
#include "uriscv/liburiscv.h"

void exceptionHandler()
{

    int ID = getCAUSE();

    // Controllo se é interrupt
    if (CAUSE_IS_INT())
        interruptHandler(ID);

    // Guardo il codice dell'eccezione, faccio lo switch e smisto sulle altre funzioni
    switch (ID)
    {
    case 8:
    case 11:
        syscallExceptionHandler(ID);
        break;

    case 24 ... 28:
        tlbExceptionHandler(ID);
        break;

    case 0 ... 7:
    case 9:
    case 10:
    case 12 ... 23:
        trapExceptionHandler(ID);
        break;

    default:
        break;
    }
}

void interruptHandler(int ID)
{
}

void tlbExceptionHandler(int ID)
{
}

void syscallExceptionHandler(int ID)
{
    // Devo controllare se il processo é in kernel mode e lui mi da la macro per farlo
    // Devo ricavare il valore del registro general purpose a0 e prendere l'id della system call che deve essere negativo
    // Poi un bello switch su quel valore da -1 a -10 mi pare e ogni caso esegue una funzione, quindi vanno create una decina di funzioni systemcall.
    // TODO: creare file systemcalls.c per rendere il tutto piu elegante

    // Sviscerare il processo e leggere a0
}

void trapExceptionHandler(int ID)
{
}
