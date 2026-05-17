#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"
#include "utils.h"

void syscallExceptionHandler(int excCode);

void terminateProcessTree(pcb_t *p);

#endif /* SYSCALLS_H */