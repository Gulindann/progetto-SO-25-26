#include "headers/tlb.h"
#include "uriscv/liburiscv.h"

void tlbExceptionHandler(int excCode)
{
    HALT();
}