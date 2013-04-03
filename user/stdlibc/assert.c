#include "assert.h"
#include "stdio.h"
#include "stdlib.h"


void assertion(const char* file, unsigned int line, const char* desc)
{
    printf("ASSERTION FAILED (%s) at %s:%u\nPress any key to exit.\n", desc, file, line);
    getchar();
    exit(EXIT_FAILURE);
}
