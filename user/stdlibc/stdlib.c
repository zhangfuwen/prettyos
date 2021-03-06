#include "limits.h"
#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "ctype.h"
#include "signal.h"


extern void (**_atexit_funcs)(); // -> syscalls.c
extern void (**_at_quick_exit_funcs)(); // -> syscalls.c


void* userheapAlloc(size_t increase); // -> Syscall, Userlib
void exitProcess(void); // -> Syscall, Userlib


void abort()
{
    raise(SIGABRT);
}

void exit(int status)
{
    if(_atexit_funcs)
        for(size_t i = 0; _atexit_funcs[i]; i++)
            _atexit_funcs[i]();
    exitProcess();
}

void quick_exit(int status)
{
    if(_at_quick_exit_funcs)
        for(size_t i = 0; _at_quick_exit_funcs[i]; i++)
            _at_quick_exit_funcs[i]();
    exitProcess();
}

void _Exit(int status)
{
    exitProcess();
}

int atexit(void (*func)())
{
    static size_t num = 0;
    static size_t size = 0;
    if(num+1 >= size) {
        size += 5;
        _atexit_funcs = malloc(size*sizeof(*_atexit_funcs));
    }
    _atexit_funcs[num] = func;
    num++;
    _atexit_funcs[num] = 0;
    return(0);
}

int at_quick_exit(void (*func)())
{
    static size_t num = 0;
    static size_t size = 0;
    if(num+1 >= size) {
        size += 5;
        _at_quick_exit_funcs = malloc(size*sizeof(*_at_quick_exit_funcs));
    }
    _at_quick_exit_funcs[num] = func;
    num++;
    _at_quick_exit_funcs[num] = 0;
    return(0);
}

int abs(int n); // -> math.c
long labs(long n)
{
    return (n<0?-n:n);
}
long long llabs(long long n)
{
    return (n<0?-n:n);
}

div_t div(int numerator, int denominator)
{
    div_t d = {.quot = numerator/denominator, .rem = numerator%denominator};
    return (d);
}

ldiv_t ldiv(long numerator, long denominator)
{
    ldiv_t d = {.quot = numerator/denominator, .rem = numerator%denominator};
    return (d);
}

lldiv_t lldiv(long long numerator, long long denominator)
{
    lldiv_t d = {.quot = numerator/denominator, .rem = numerator%denominator};
    return (d);
}

int mblen(const char* pmb, size_t max); /// TODO
size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t max); /// TODO
int mbtowc(wchar_t* pwc, const char* pmb, size_t max); /// TODO
size_t wcstombs(char* mbstr, const wchar_t* wcstr, size_t max); /// TODO
int wctomb(char* pmb, wchar_t character); /// TODO

static uint32_t seed = 0;
void srand(int val)
{
    seed = val;
}
int rand()
{
    return (((seed = seed * 214013L + 2531011L) >> 16) & RAND_MAX);
}
double random(double lower, double upper)
{
    return (( (double) rand() / ((double)RAND_MAX / (upper - lower))) + lower );
}

void* malloc(size_t size)
{
    static char* cur = 0;
    static char* top = 0;

    // Align
    size = (size+15) & ~15;

    // Heap not set up?
    if (!cur)
    {
        uint32_t sizeToGrow = (size+4095) & ~4095;
        cur = userheapAlloc(sizeToGrow);
        if (!cur)
            return (0);
        top = cur + sizeToGrow;
    }
    // Not enough space on heap?
    else if (top - cur < size)
    {
        uint32_t sizeToGrow = (size+4095) & ~4095;
        if (!userheapAlloc(sizeToGrow))
            return (0);
        top += sizeToGrow;
    }

    void* ret = cur;
    cur += size;
   return (ret);
}

void* calloc(size_t num, size_t size)
{
    void* ptr = malloc(num*size);
    memset(ptr, 0, num*size);
    return (ptr);
}

void* realloc(void* ptr, size_t size); /// TODO (Impossible with placement)

void free(void* ptr)
{
    // We do placement allocation at the moment -> Nothing to do.
}

char* getenv(const char* name)
{
    return (0); // We do not support any environment variables at the moment
}

int system(const char* command); /// TODO
