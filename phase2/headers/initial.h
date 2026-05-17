#ifndef INITIAL_H
#define INITIAL_H

#include "../../phase1/headers/pcb.h" /* pcb_t, list_head */
#include "../../headers/const.h"      /* SEMDEVLEN */

extern int processCount;
extern int softBlockCount;
extern pcb_t *currentProcess;
extern struct list_head readyQueue;
extern int deviceSemaphores[SEMDEVLEN];

#endif /* INITIAL_H */