/* Functions: (bsearch), qsort
 * Todo: bsearch, handle qsort() stack overflow.
 */

#include "stdlib.h"

void* bsearch(const void* key, const void* base, size_t nmemb, 
    size_t size, int (*compar)(const void *, const void *));

static void swap(void* a, void* b, size_t size)
{
    char buf;
    int ibuf;
    size_t is = (size) / sizeof(int);
    size_t s = (size) - is * sizeof(int);
    while (is--)
    {
        ibuf = *((int*)(a) + is);
        *((int*)(a) + is) = *((int*)(b) + is);
        *((int*)(b) + is) = ibuf;
    }
    while (s--)
    {
        buf = *((char*)(a) + s);
        *((char*)(a) + s) = *((char*)(b) + s);
        *((char*)(b) + s) = buf;
    }
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
    const int stack_size = 0x10000;
    void *stack[stack_size];
    void **pos = stack;
    void *l = base, *r = (char*)base + nmemb * size;
    do {
        if (l < r)
        {
            void *pivot = r;
            void *i = (char*) l - size;
            void *j = (char*) r + size;
            do {
                do {
                    i = (char*)i + size;
                } while (compar(i, pivot) < 0);
                do {
                    j = (char*)j - size;
                } while (compar(j, pivot) > 0);
                if (i < j)
                    swap(i, j, size);
            } while (i < j);
            if (pos != stack + stack_size)
            {
                *pos++ = r;
                r = (char*)l >= (char*)i - size ? l : (char*)i - size;
            }
        }
        else
        {
            if (pos != stack)
            {
                l = (char*)r + size;
                r = *(--pos);
            }
        } // what do we do, if (pos == stack + stack_size) ?
    } while (pos != stack && pos != stack + stack_size);
}
