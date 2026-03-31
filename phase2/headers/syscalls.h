#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"

void syscallExceptionHandler(int excCode);

#endif /* SYSCALLS_H */