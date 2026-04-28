#ifndef TRAPS_H
#define TRAPS_H

#include "initial.h"
#include "uriscv/liburiscv.h"
#include "../../headers/const.h"

void trapExceptionHandler(int excCode);
void passUpOrDie(int exceptionType);  // <-- aggiungere

#endif

