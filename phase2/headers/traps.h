#ifndef TRAPS_H
#define TRAPS_H

#include "../../headers/const.h" /* GENERALEXCEPT, PGFAULTEXCEPT */

void passUpOrDie(int exceptionType);
void trapExceptionHandler(int excCode);

#endif /* TRAPS_H */