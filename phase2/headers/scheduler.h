#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../../headers/types.h" /* cpu_t */
#include "initial.h"

extern cpu_t p_start;

void scheduler(void);

#endif /* SCHEDULER_H */