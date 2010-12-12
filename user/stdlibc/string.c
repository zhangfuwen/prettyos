#include "string.h"
#include "stdint.h"

void* memchr(const void* ptr, char value, size_t num); /// TODO
int memcmp(const void* ptr1, const void* ptr2, size_t num); /// TODO

void* memcpy(void* dest, const void* source, size_t num)
{
    const uint8_t* sp = (const uint8_t*)source;
    uint8_t* dp = (uint8_t*)dest;
    for (; num != 0; num--) *dp++ = *sp++;
    return dest;
}

void* memmove(void* dest, const void* source, size_t num)
{
    if(source == dest || num == 0) // nothing to do
    {
        return(dest);
    }

    // Check for out of memory
    const uintptr_t memMax = ~((uintptr_t)0) - (num - 1); // ~0 is the highest possible value of the variables type. (No underflow possible on substraction, because size < adress space)
    if((uintptr_t)source > memMax || (uintptr_t)dest > memMax)
    {
        return(dest);
    }

    const uint8_t* source8 = (uint8_t*)source;
    uint8_t* destination8 = (uint8_t*)dest;

    // Arrangement of the destination and source decides about the direction of copying
    if(source8 < destination8)
    {
        source8 += num - 1;
        destination8 += num - 1;
        while(num > 0)
        {
            *destination8 = *source8;
            --destination8;
            --source8;
            --num;
        }
    }
    else // In all other cases, it is ok to copy from the start to the end of source.
    {
        while(num > 0)
        {
            *destination8 = *source8;
            ++destination8;
            ++source8;
            --num;
        }
    }

    return(dest);
}

void* memset(void* dest, char value, size_t num)
{
    char* temp = (char*)dest;
    for (; num != 0; num--) *temp++ = value;
    return dest;
}



size_t strlen(const char* str)
{
    size_t retval;
    for (retval = 0; *str != '\0'; ++str)
        ++retval;
    return retval;
}

int strcmp(const char* s1, const char* s2)
{
    while ((*s1) && (*s1 == *s2))
    {
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    while ((*s1) && n > 0 && (*s1 == *s2))
    {
        ++s1;
        ++s2;
        --n;
    }
    return (*s1 - *s2);
}

char* strcpy(char* dest, const char* src)
{
   char* save = dest;
   while ((*dest++ = *src++));
   return save;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    size_t i = 0;
    for(; i < n && src[i] != 0; i++)
    {
        dest[i] = src[i];
    }
    for(; i < n; i++)
    {
        dest[i] = 0;
    }
    return(dest);
}

char* strcat(char* dest, const char* src)
{
    strcpy(dest + strlen(dest), src);
    return dest;
}

char* strncat(char* dest, const char* src, size_t n)
{
    strncpy(dest + strlen(dest), src, n);
    return dest;
}

char* strchr(char* str, char character)
{
    for (;;str++)
    {
        // the order here is important
        if (*str == character)
        {
            return str;
        }
        if (*str == 0) // end of string
        {
            return 0;
        }
    }
}

int strcoll(const char* str1, const char* str2); /// TODO
size_t strcspn(const char* str1, const char* str2); /// TODO
char* strerror(int errornum); /// TODO
char* strpbrk(const char* str1, const char* str2); /// TODO
char* strrchr(const char* str, char character); /// TODO
size_t strspn(const char* str1, const char* str2); /// TODO
char* strstr(const char* str1, const char* str2); /// TODO
char* strtok(char* str, const char* delimiters); /// TODO
size_t strxfrm(char* destination, const char* source, size_t num); /// TODO
