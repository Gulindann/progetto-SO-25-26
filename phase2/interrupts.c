#include "headers/interrupts.h"
#include "headers/initial.h"
#include "headers/scheduler.h"
#include "headers/utils.h"
#include "../phase1/headers/asl.h"
#include "../headers/types.h"
#include <uriscv/cpu.h>
#include <uriscv/liburiscv.h>

state_t *saved_interrupt_state;

void handlePLT();
void handlePseudoClock();
void handleDevice(int line);

void interruptHandler(unsigned int causeReg)
{
    // save the current processor state
    saved_interrupt_state = GET_EXCEPTION_STATE_PTR(0);

    //  Map interrupt cause to specific line handlers
    unsigned int excCode = causeReg & CAUSE_EXCCODE_MASK;

    if (excCode == IL_CPUTIMER)
        handlePLT();
    else if (excCode == IL_TIMER)
        handlePseudoClock();
    else if (excCode == IL_DISK)
        handleDevice(3);
    else if (excCode == IL_FLASH)
        handleDevice(4);
    else if (excCode == IL_ETHERNET)
        handleDevice(5);
    else if (excCode == IL_PRINTER)
        handleDevice(6);
    else if (excCode == IL_TERMINAL)
        handleDevice(7);
    else
        PANIC();
}

void handlePLT()
{

    // Load PLT (5ms)

    setTIMER(TIMESLICE);
    updateCpuTime();

    if (currentProcess != NULL)
    {
        // Save current state and put process back in Ready Queue
        currentProcess->p_s = *saved_interrupt_state;
        insertProcQ(&readyQueue, currentProcess);
    }
    scheduler();
}

void handlePseudoClock()
{
    // Load Interval Timer (100ms)
    LDIT(PSECOND);

    // unlock all processes waiting in the pseudoclock semaphore
    int *clockSem = &deviceSemaphores[SEMDEVLEN - 1];
    while (*clockSem < 0)
    {
        (*clockSem)++;
        pcb_t *p = removeBlocked(clockSem);
        if (p != NULL)
        {
            insertProcQ(&readyQueue, p);
            softBlockCount--;
        }
    }

    updateCpuTime();

    if (currentProcess != NULL)
        LDST(saved_interrupt_state);
    else
        scheduler();
}

void handleDevice(int line)
{
    // Identify the device number
    unsigned int *bitmapAddr = (unsigned int *)(0x10000040 + ((line - 3) * 4));
    unsigned int bitmap = *bitmapAddr;
    int devNo = -1;

    // Scan the bitmap to find the highest priority interrupting device
    // The lower the interrupt line and device number, the higher the priority of the interrupt
    for (int i = 0; i < 8; i++)
    {
        if (bitmap & (1 << i))
        {
            devNo = i;
            break;
        }
    }

    if (devNo == -1)
    {
        if (currentProcess != NULL)
            LDST(saved_interrupt_state);
        else
            scheduler();
    }

    // Compute device register base address and semaphore index
    unsigned int devAddrBase = 0x10000054 + ((line - 3) * 0x80) + (devNo * 0x10);
    int semIndex = (line - 3) * 8 + devNo;
    unsigned int status;

    // Acknowledge and save status

    if (line == 7)
    { // Terminals
        termreg_t *term = (termreg_t *)devAddrBase;
        if ((term->transm_status & 0xFF) == 5)
        { // Successful transmission
            status = term->transm_status;
            term->transm_command = 1; // ACK
            semIndex += 8;            // Shift to transmit semaphores
        }
        else
        {
            status = term->recv_status;
            term->recv_command = 1; // ACK
        }
    }
    else
    {
        dtpreg_t *dev = (dtpreg_t *)devAddrBase;
        status = dev->status;
        dev->command = 1; // ACK
    }

    // V on waiting process

    int *semAddr = &deviceSemaphores[semIndex];
    (*semAddr)++;
    if (*semAddr <= 0)
    {
        pcb_t *p = removeBlocked(semAddr);
        if (p != NULL)
        {
            p->p_s.reg_a0 = status; // Return status in a0
            insertProcQ(&readyQueue, p);
            softBlockCount--;
        }
    }

    // Return control to current process or scheduler
    if (currentProcess != NULL)
    {
        updateCpuTime();
        LDST(saved_interrupt_state);
    }
    else
    {
        scheduler();
    }
}