/* Functions: atof, atoi, atol, strtod, strtol, strtoul
 * Todo: errno.h -> ERANGE, strtod() overflow -> HUGE_VAL?
 */

#include "ctype.h"
#include "limits.h"
#include "stdlib.h"
#include "stdint.h"

double atof(const char *nptr)
{
    return strtod(nptr, 0);
}

int atoi(const char* nptr)
{
    return (int)strtol(nptr, 0, 10);
}

long int atol(const char* nptr)
{
    return strtol(nptr, 0, 10);
}

double strtod(const char* nptr, char** endptr)
{
    double num = 0.0;
    int sign, point = 0, exp = 0;

    while (isspace(*nptr))
        ++nptr; // skip spaces

    sign = *nptr == '-' ? -1 : 1;
    if (*nptr == '+' || *nptr == '-')
        ++nptr;

    while (*nptr == '0')
        ++nptr;

    for (; *nptr != '\0'; ++nptr)
    {
        if (*nptr >= '0' && *nptr <= '9')
        {
            if (exp)
            {
                exp = exp * 10 + *nptr - '0';
            }
            else if (point)
            {
                num += (double)(*nptr - '0') / point;
                point *= 10;
            }
            else
            {
                num = num * 10 + *nptr - '0';
            }
        }
        else if (*nptr == '.')
        {
            if (!point)
            {
                point = 10;
            }
            else
            {
                break;
            }
        }
        else if (*nptr == 'E' || *nptr == 'e')
        {
            ++nptr;
            exp = *nptr == '-' ? -1 : 1;
            if (*nptr == '+' || *nptr == '-')
                ++nptr;
            if (*nptr >= '0' && *nptr <= '9')
                exp *= *nptr - '0';
        }
    }

    if (exp > 0)
    {
        while (exp--)
            num *= 10;
    }
    else if (exp < 0)
    {
        while (exp++)
            num /= 10;
    }

    if (endptr)
        *endptr = (char*)nptr;

    return num * sign;
}

long int strtol(const char* nptr, char** endptr, int base)
{
    long num = 0;
    int sign;

    if (base && (base < 2 || base > 36))
        return num;

    while (isspace(*nptr))
        ++nptr; // skip spaces

    sign = *nptr == '-' ? -1 : 1;
    if (*nptr == '-' || *nptr == '+')
        ++nptr;

    if (base == 0)
    {
        base = 10;
        if (*nptr == '0' && (*(++nptr) == 'X' || *nptr == 'x'))
        {
            ++nptr;
            base = 16;
        }
    }
    else if (base == 16)
    {
        if (*nptr == '0' && (*(++nptr) == 'X' || *nptr == 'x'))
            ++nptr;
    }

    while (*nptr == '0')
        ++nptr;

    for (; *nptr != '\0'; ++nptr)
    {
        if (*nptr - '0' < base && *nptr - '0' >= 0)
        {
            num = num * base + *nptr - '0';
            if (num <= 0)
            {
                num = LONG_MAX;
                break;
            }
        }
        else if ((*nptr | 0x20) - 'a' + 10 < base && 
            (*nptr | 0x20) - 'a' + 10 >= 0)
        { // 'x' | 0x20 =~= tolower('X')
            num = num * base + (*nptr | 0x20) - 'a' + 10;
            if (num <= 0)
            {
                num = LONG_MAX;
                break;
            }
        }
        else
        {
            break;
        }
    }

    for (; *nptr != '\0'; ++nptr)
    { // skip rest of integer constant
        if (!(*nptr - '0' < base && *nptr - '0' >= 0) && 
            !((*nptr | 0x20) - 'a' + 10 < base) && 
            !((*nptr | 0x20) - 'a' + 10 >= 0))
        {
            break;
        }
    }

    if (endptr)
        *endptr = (char*)nptr;

    return num * sign;
}

unsigned long int strtoul(const char* nptr, char** endptr, int base)
{
    unsigned long num = 0, buf;

    while (isspace(*nptr))
        ++nptr; // skip spaces

    if (base && (base < 2 || base > 36))
        return num;

    if (base == 0)
    {
        base = 10;
        if (*nptr == '0' && (*(++nptr) == 'X' || *nptr == 'x'))
        {
            ++nptr;
            base = 16;
        }
    }
    else if (base == 16)
    {
        if (*nptr == '0' && (*(++nptr) == 'X' || *nptr == 'x'))
            ++nptr;
    }

    while (*nptr == '0')
        ++nptr;

    for (; *nptr != '\0'; ++nptr)
    {
        buf = num;
        if (*nptr - '0' < base && *nptr - '0' >= 0)
        {
            num = num * base + *nptr - '0';
        }
        else if ((*nptr | 0x20) - 'a' + 10 < base && 
                 (*nptr | 0x20) - 'a' + 10 >= 0)
        { // 'x' | 0x20 =~= tolower('X')
            num = num * base + (*nptr | 0x20) - 'a' + 10;
        }
        else
        {
            break;
        }
        if (num <= buf)
        {
            num = ULONG_MAX;
            break;
        }
    }

    for (; *nptr != '\0'; ++nptr)
    { // skip rest of integer constant
        if (!(*nptr - '0' < base && *nptr - '0' >= 0) && 
            !((*nptr | 0x20) - 'a' + 10 < base) && 
            !((*nptr | 0x20) - 'a' + 10 >= 0))
        {
            break;
        }
    }

    if (endptr)
        *endptr = (char*)nptr;

    return num;
}
