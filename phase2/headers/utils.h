#ifndef UTILS_H
#define UTILS_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"
#include "initial.h"
#include "scheduler.h"

void *memcpy(void *dest, const void *src, unsigned int n);

void updateCpuTime();

#endif