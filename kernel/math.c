#include "os.h"

int32_t power(int32_t base,int32_t n)
{
    int32_t i,p;
    if (n == 0)
        return 1;
    p=1;
    for (i=1;i<=n;++i)
        p=p*base;
    return p;
}

int32_t abs(int32_t i)
{
  return i < 0 ? -i : i;
}


