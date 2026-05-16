#ifndef INITIAL_H
#define INITIAL_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"

extern int processCount;      // Contatore dei processi attivi
extern int softBlockCount;    // Contatore dei processi bloccati (Soft Block Count)
extern pcb_t *currentProcess; // Puntatore al processo attualmente in esecuzione

extern struct list_head readyQueue;
extern int deviceSemaphores[SEMDEVLEN];

#endif /* INITIAL_H */