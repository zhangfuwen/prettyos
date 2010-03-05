#include "os.h"

int32_t power(int32_t base, int32_t n)
{
    if (n == 0 || base == 1)
    {
        return 1;
    }
    int32_t p = 1;
    for (; n > 0; --n)
    {
        p *= base;
    }
    return p;
}

int32_t abs(int32_t i)
{
    return i < 0 ? -i : i;
}


