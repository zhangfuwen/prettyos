/*
 *  license and disclaimer for the use of this source code as per statement below
 *  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
 */

#include "stdlib.h"

// TODO: bsearch, handle qsort() stack overflow.

void* bsearch(const void* key, const void* base, size_t nmemb, 
    size_t size, int (*compar)(const void *, const void *));

static void swap(void* a, void* b, size_t size)
{
    size_t is = (size) / sizeof(int);
    size_t s = (size) - is * sizeof(int);
    while (is--)
    {
        int ibuf = *((int*)(a) + is);
        *((int*)(a) + is) = *((int*)(b) + is);
        *((int*)(b) + is) = ibuf;
    }
    while (s--)
    {
        char buf = *((char*)(a) + s);
        *((char*)(a) + s) = *((char*)(b) + s);
        *((char*)(b) + s) = buf;
    }
}

void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void*, const void*))
{
    const int stack_size = 0x10000;
    void* stack[stack_size];
    void** pos = stack;
    void* l = base, *r = (char*)base + nmemb * size;
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

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
