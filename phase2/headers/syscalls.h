#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../../phase1/headers/pcb.h" /* pcb_t (per terminateProcessTree) */

void syscallExceptionHandler(int excCode);
void terminateProcessTree(pcb_t *p);

#endif /* SYSCALLS_H */