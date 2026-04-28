#include "headers/tlb.h"
#include "headers/traps.h"
#include "uriscv/liburiscv.h"
#include "../headers/const.h"

void tlbExceptionHandler(int excCode)
{
    passUpOrDie(PGFAULTEXCEPT);
}