#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "initial.h"
#include "uriscv/cpu.h"
#include "uriscv/liburiscv.h"
#include "../../headers/const.h"
#include "utils.h"
#include "scheduler.h"

void interruptHandler(unsigned int cause_reg);

#endif /* EXCEPTIONS_H */