#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "ctype.h"


void* userheapAlloc(size_t increase); // -> Syscall, Userlib


void abort(); /// TODO
void exit(); // -> Syscall
int atexit(void (*func)()); /// TODO



int abs(int n); // -> math.c
long labs(long n)
{
	return(abs(n)); // HACK?
}



div_t div(int numerator, int denominator); /// TODO
ldiv_t ldiv(long numerator, long denominator); /// TODO



int atoi(const char* s)
{
    int32_t num = 0;
    bool sign = false;
    for (size_t i=0; i<=strlen(s); i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] -'0';
        }
        else if (s[0] == '-' && i==0)
        {
            sign = true;
        }
        else
        {
            break;
        }
    }
    if (sign)
    {
        num *= -1;
    }
    return num;
}

long atol(const char* s)
{
	return(atoi(s)); // HACK?
}

float atof(const char* s) // TODO: Should return double
{
    int32_t i = 0;
    int8_t sign = 1;
    while(s[i] == ' ' || s[i] == '+' || s[i] == '-')
    {
        if(s[i] == '-')
        {
            sign *= -1;
        }
        i++;
    }

    float val;
    for (val = 0.0; isdigit(s[i]); i++)
    {
        val = 10.0 * val + s[i] - '0';
    }
    if (s[i] == '.')
    {
        i++;
    }
    float pow;
    for (pow = 1.0; isdigit(s[i]); i++)
    {
        val = 10.0 * val + s[i] - '0';
        pow *= 10.0;
    }
    return(sign * val / pow);
}

double strtod(const char* str, char** endptr); /// TODO
long strtol(const char* str, char** endptr, int base); /// TODO
unsigned long strtoul(const char* str, char** endptr, int base); /// TODO



int mblen(const char* pmb, size_t max); /// TODO
size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t max); /// TODO
int mbtowc(wchar_t* pwc, const char* pmb, size_t max); /// TODO
size_t wcstombs(char* mbstr, const wchar_t* wcstr, size_t max); /// TODO
int wctomb(char* pmb, wchar_t character); /// TODO

void* bsearch(const void* key, const void* base, size_t num, size_t size, int (*comparator)(const void*, const void*)); /// TODO

void qsort(void* base, size_t num, size_t size, int (*comparator)(const void*, const void*)); /// TODO



static uint32_t seed = 0;
void srand(int val)
{
    seed = val;
}
int rand()
{
    return (((seed = seed * 214013L + 2531011L) >> 16) & 0x7FFF);
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
            return 0;
        top = cur + sizeToGrow;
    }
    // Not enough space on heap?
    else if (top - cur < size)
    {
        uint32_t sizeToGrow = (size+4095) & ~4095;
        if (!userheapAlloc(sizeToGrow))
            return 0;
        top += sizeToGrow;
    }

    void* ret = cur;
    cur += size;
    return ret;
}

void* calloc(size_t num, size_t size)
{
	void* ptr = malloc(num*size);
	memset(ptr, 0, num*size);
	return(ptr);
}

void* realloc(void* ptr, size_t size); /// TODO

void free(void* ptr)
{
	// We do placement allocation at the moment -> Nothing to do.
}



char* getenv(const char* name)
{
	return(0); // We do not support any environment variables at the moment
}
int system(const char* command); /// TODO
