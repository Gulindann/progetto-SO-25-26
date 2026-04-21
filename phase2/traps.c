#include "headers/traps.h"
#include "uriscv/liburiscv.h"

void trapExceptionHandler(int excCode)
{
    // Se la CPU entra qui, significa che qualcuno ha chiamato PANIC() (ebreak)
    // oppure c'è stato un errore grave.
    // Fermiamo l'emulatore in modo PULITO per evitare che salti a 0x0!
    HALT();
}