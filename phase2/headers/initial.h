#ifndef INITIAL_H
#define INITIAL_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"

extern int PROC_C;       // Contatore dei processi attivi
extern int SBLOCK_C;     // Contatore dei processi bloccati (Soft Block Count)
extern pcb_t *CURRENT_P; // Puntatore al processo attualmente in esecuzione

extern struct list_head READY_Q;
extern semd_t SEM_DEV_Q[SEMDEVLEN];

#endif /* INITIAL_H */