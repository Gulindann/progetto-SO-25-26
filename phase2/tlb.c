#include "headers/tlb.h"
#include "headers/traps.h"

void tlbExceptionHandler(int excCode)
{
    passUpOrDie(PGFAULTEXCEPT);
}