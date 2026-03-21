#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
#include "../../headers/const.h"

void exceptionHandler();

void interruptHandler();        /* CAUSE_IS_INT == true */
void tlbExceptionHandler();     /* Exception codes 24-28 */
void syscallExceptionHandler(); /* Exception codes 8 e 11 */
void trapExceptionHandler();    /* Exception codes 0-7, 9, 10, 12-23 */

#endif /* EXCEPTIONS_H */