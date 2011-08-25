#include "string.h"
#include "stdint.h"

void* memchr(void* ptr, char value, size_t num)
{
    char* str = ptr;
    for (; num > 0; str++, num--)
    {
        if (*str == value)
        {
            return str;
        }
    }
    return(0);
}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
    if (num == 0) return(0);

    const uint8_t* s1 = ptr1;
    const uint8_t* s2 = ptr2;
    for (; num > 1 && *s1 == *s2; num--)
    {
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}

void* memcpy(void* dest, const void* source, size_t bytes)
{
    size_t dwords = bytes/4;
    bytes %= 4;
    __asm__ volatile("cld\n" "rep movsl" : : "S" (source), "D" (dest), "c" (dwords));
    __asm__ volatile("rep movsb" : : "c" (bytes));
    return(dest);
}

static void* memcpyr(void* dest, const void* src, size_t bytes)
{
    // Calculate starting addresses
    void* temp = dest+bytes-1;
    src += bytes-1;

    size_t dwords = bytes/4;
    bytes %= 4;

    __asm__ volatile("std\n" "rep movsb"         : : "S" (src), "D" (temp), "c" (bytes));
    __asm__ volatile("sub $3, %edi\n" "sub $3, %esi");
    __asm__ volatile("rep movsl\n" "cld" : : "c" (dwords));
    return(dest);
}

void* memmove(void* dest, const void* source, size_t num)
{
    if (source == dest || num == 0) // Copying is not necessary. Calling memmove with source==destination or size==0 is not a bug.
    {
        return(dest);
    }

    // Check for out of memory
    const uintptr_t memMax = ~((uintptr_t)0) - (num - 1); // ~0 is the highest possible value of the variables type. (No underflow possible on substraction, because size < adress space)
    if ((uintptr_t)source > memMax || (uintptr_t)dest > memMax)
    {
        return(dest);
    }

    // Arrangement of the destination and source decides about the direction of copying
    if (source < dest)
    {
        memcpyr(dest, source, num);
    }
    else // In all other cases, it is ok to copy from the start to the end of source.
    {
        memcpy(dest, source, num); // We assume, that memcpy does forward copy
    }
    return(dest);
}

void* memset(void* dest, char value, size_t bytes)
{
    size_t dwords = bytes/4; // Number of dwords (4 Byte blocks) to be written
    bytes %= 4;              // Remaining bytes
    uint32_t dval = (value<<24)|(value<<16)|(value<<8)|value; // Create dword from byte value
    __asm__ volatile("cld\n" "rep stosl" : : "D"(dest), "eax"(dval), "c" (dwords));
    __asm__ volatile("rep stosb" : : "al"(value), "c" (bytes));
    return dest;
}



size_t strlen(const char* str)
{
    size_t retval = 0;
    for (; *str != '\0'; ++str)
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
    if (n == 0) return(0);

    for (; *s1 && n > 1 && *s1 == *s2; n--)
    {
        ++s1;
        ++s2;
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
    for (; i < n && src[i] != 0; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
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

char* strrchr(const char* s, int c)
{
    const char* p = s + strlen(s);
    if (c == 0)
        return (char*)p;
    while (p != s)
    {
        if (*(--p) == c)
            return (char*)p;
    }
    return 0;
}

int strcoll(const char* str1, const char* str2); /// TODO
size_t strcspn(const char* str1, const char* str2); /// TODO
char* strerror(int errornum); /// TODO

char* strpbrk(const char* str, const char* delim)
{
    for(; *str != 0; str++)
        for(size_t i = 0; delim[i] != 0; i++)
            if(*str == delim[i])
                return((char*)str);

    return(0);
}

size_t strspn(const char* str1, const char* str2); /// TODO

char* strstr(const char* str1, const char* str2)
{
    const char* p1 = str1;
    const char* p2;
    while (*str1)
    {
        p2 = str2;
        while (*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }
        if (*p2 == 0)
        {
            return (char*)str1;
        }
        ++str1;
        p1 = str1;
    }
    return 0;
}

char* strtok(char* str, const char* delimiters); /// TODO
size_t strxfrm(char* destination, const char* source, size_t num); /// TODO
