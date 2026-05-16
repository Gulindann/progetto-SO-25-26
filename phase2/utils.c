#include "headers/utils.h"

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

void updateCpuTime()
{
    cpu_t timenow;
    STCK(timenow);
    currentProcess->p_time += (timenow - p_start);

    p_start = timenow;
}
