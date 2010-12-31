#ifndef STDLIB_H
#define STDLIB_H

#include "stddef.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1
#define MB_CUR_MAX
#define RAND_MAX


typedef struct {
    int quot, rem;
} div_t;

typedef struct {
    long quot, rem;
} ldiv_t;


#ifdef _cplusplus
extern "C" {
#endif

void abort();
void exit();
int atexit(void (*func)());

int abs(int n);
long labs(long n);

div_t div(int numerator, int denominator);
ldiv_t ldiv(long numerator, long denominator);

int atoi(const char* s);
long atol(const char* s);
float atof(const char* s); // TODO: Should return double
double strtod(const char* str, char** endptr);
long strtol(const char* str, char** endptr, int base);
unsigned long strtoul(const char* str, char** endptr, int base);

int mblen(const char* pmb, size_t max);
size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t max);
int mbtowc(wchar_t* pwc, const char* pmb, size_t max);
size_t wcstombs(char* mbstr, const wchar_t* wcstr, size_t max);
int wctomb(char* pmb, wchar_t character);

void* bsearch(const void* key, const void* base, size_t num, size_t size, int (*comparator)(const void*, const void*));

void qsort(void* base, size_t num, size_t size, int (*comparator)(const void*, const void*));

void srand(int val);
int rand();

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

char* getenv(const char* name);
int system(const char* command);

#ifdef _cplusplus
}
#endif

#endif
