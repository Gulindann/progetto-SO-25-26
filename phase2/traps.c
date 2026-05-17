#include "headers/traps.h"
#include "headers/initial.h"
#include "headers/syscalls.h"
#include "headers/scheduler.h"
#include "../headers/types.h"
#include <uriscv/liburiscv.h>

void passUpOrDie(int exceptionType)
{
    if (currentProcess->p_supportStruct == NULL)
    {
        // Die: terminate the current process and all its progeny
        outChild(currentProcess);
        terminateProcessTree(currentProcess);
        currentProcess = NULL;
        scheduler();
    }
    else
    {
        // Pass Up: copy the saved state to the support structure
        state_t *saved = GET_EXCEPTION_STATE_PTR(0);
        currentProcess->p_supportStruct->sup_exceptState[exceptionType] = *saved;

        // Load the context from the support structure via LDCXT
        context_t *ctx = &currentProcess->p_supportStruct->sup_exceptContext[exceptionType];
        LDCXT(ctx->stackPtr, ctx->status, ctx->pc);
    }
}

void trapExceptionHandler(int excCode)
{
    passUpOrDie(GENERALEXCEPT);
}