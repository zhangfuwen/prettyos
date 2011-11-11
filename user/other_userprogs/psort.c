/*
 * License and disclaimer for the use of this source code as per statement below.
 * Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten.
 */

/*
 * Shows use of quicksort
 * Todo: -
 * Comments: -
 */

#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"
#include "userlib.h"

int getAnswer(char a, char b)
{
    int c;
    while ((c = getchar()) != EOF)
    {
        if (toupper(c) == toupper(a))
        {
            //while ((c = getchar()) != EOF && c != '\n');
            return 1;
        }
        if (toupper(c) == toupper(b))
        {
            //while ((c = getchar()) != EOF && c != '\n');
            return (0);
        }
    }
    return (0);
}

void is_sorted(int* a, size_t size)
{
    if (size)
    {
        int *l = a++;
        while (--size)
        {
            if (*l++ > *a++)
            {
                printf("unsorted!\n");
                return;
            }
        }
    }
    printf("sorted!\n");
}

int compare(const void* a, const void* b)
{
    return *(int*)a - *(int*)b;
}

int main()
{
    int* numbers = 0;
    while (1)
    {
        char buf[0x100];
        printf("How many numbers do you want to sort? (0 to exit): ");
        size_t amount = strtoul(gets(buf), 0, 10);
        if (!amount)
        {
            return (0);
        }
        if ((numbers = (int*)malloc(amount * sizeof(*numbers))))
        {
            printf("Random or Custom? (r/c): ");
            putchar('\n');
            size_t i;
            if (getAnswer('c', 'r'))
            {
                for (i = 0; i < amount; i++)
                {
                    printf("Number %i: ", i + 1);
                    numbers[i] = atoi(gets(buf));
                }
            }
            else
            {
                srand(getCurrentMilliseconds());
                for (i = 0; i < amount; i++)
                    numbers[i] = rand();
            }
            printf("Sorting %i numbers..\n", amount);
            size_t t = getCurrentMilliseconds();
            qsort(numbers, amount, sizeof(*numbers), compare);
            is_sorted(numbers, amount);
            printf("Elapsed time: %i\n", getCurrentMilliseconds() - t);
            printf("Do you want to show the numbers? (y/n): ");
            putchar('\n');
            if (getAnswer('y', 'n'))
            {
                for (i = 0; i < amount; i++)
                    printf("%i, ", numbers[i]);
            }
            free(numbers);
            printf("\n\n");
        }
        else
        {
            printf("Could not allocate enough memory.\n");
        }
    }
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
