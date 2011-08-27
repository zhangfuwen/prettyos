/*
 * Shows a DNS query.
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
            return 0;
        }
    }
    return 0;
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
            return 0;
        }
        if ((numbers = (int*)malloc(amount * sizeof(*numbers))))
        {
            printf("Random or Custom? (r/c): ");
            printf("\n");
            size_t i;
            if (getAnswer('c', 'r'))
            {
                for (i = 0; i < amount; ++i)
                {
                    printf("Number %i: ", i + 1);
                    numbers[i] = atoi(gets(buf));
                }
            }
            else
            {
                srand(getCurrentMilliseconds());
                for (i = 0; i < amount; ++i)
                    numbers[i] = rand();
            }
            printf("Sorting %i numbers..\n", amount);
            size_t t = getCurrentMilliseconds();
            qsort(numbers, amount, sizeof(*numbers), compare);
            is_sorted(numbers, amount);
            printf("Elapsed time: %i\n", getCurrentMilliseconds() - t);
            printf("Do you want to show the numbers? (y/n): ");
            printf("\n");
            if (getAnswer('y', 'n'))
            {
                for (i = 0; i < amount; ++i)
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
