#include "headers/traps.h"
#include "headers/initial.h"
#include "headers/syscalls.h"
#include "uriscv/liburiscv.h"

void passUpOrDie(int exceptionType)
{
    if (CURRENT_P->p_supportStruct == NULL)
    {
        // "Die": termina il processo corrente e tutta la sua progenie
        // Riusa la logica di TerminateProcess con target = CURRENT_P
        outChild(CURRENT_P);
        terminateProcessTree(CURRENT_P);
        scheduler();
    }
    else
    {
        // "Pass Up": copia lo stato salvato nella Support Structure
        state_t *saved = GET_EXCEPTION_STATE_PTR(0);
        CURRENT_P->p_supportStruct->sup_exceptState[exceptionType] = *saved;

        // Passa il controllo al Support Level tramite LDCXT
        context_t *ctx = &CURRENT_P->p_supportStruct->sup_exceptContext[exceptionType];
        LDCXT(ctx->stackPtr, ctx->status, ctx->pc);
    }
}

void trapExceptionHandler(int excCode)
{
    passUpOrDie(GENERALEXCEPT);
}