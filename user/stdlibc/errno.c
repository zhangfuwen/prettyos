#include "errno.h"


int* _errno()
{
    static int error = 0;
    return (&error);
}
