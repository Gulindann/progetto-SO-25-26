#include "headers/utils.h"
#include "headers/initial.h"
#include "headers/scheduler.h"
#include <uriscv/liburiscv.h>

// Standard memory copy utility
// This is required because the compiler generates implicit calls to memcpy
// when performing assignments of large structures like state_t.
void *memcpy(void *dest, const void *src, unsigned int n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (unsigned int i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

// Update the accumulated CPU time for the current process
void updateCpuTime()
{
    // Time accounting is only performed if a process is currently running
    if (currentProcess != NULL)
    {
        cpu_t timeNow;
        // Capture current TOD clock value
        STCK(timeNow);

        // Add the duration of the last execution interval to the total p_time
        currentProcess->p_time += (timeNow - p_start);

        // Reset the interval start point for the next measurement
        p_start = timeNow;
    }
    else
    {
        // If the CPU was idle (WAIT state), just reset the reference timer
        STCK(p_start);
    }
}