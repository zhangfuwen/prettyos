#include "userlib.h"

/// syscalls ///

void settextcolor(unsigned int foreground, unsigned int background)
{
    asm volatile( "int $0x7F" : : "a"(2), "b"(foreground), "c"(background) );
}

void putch(unsigned char val)
{
    asm volatile( "int $0x7F" : : "a"(1), "b"(val) );
}

void puts(char* pString)
{
    asm volatile( "int $0x7F" : : "a"(0), "b"(pString) );
}

unsigned char getch()
{
    unsigned char ret;
    asm volatile( "int $0x7F" : "=a"(ret): "a"(7) );
    return ret;
}

void floppy_dir()
{
    asm volatile( "int $0x7F" : : "a"(8) );
}



/// user functions ///

// Compare two strings. Returns -1 if str1 < str2, 0 if they are equal or 1 otherwise.
int strcmp( const char* s1, const char* s2 )
{
    while ( ( *s1 ) && ( *s1 == *s2 ) )
    {
        ++s1; ++s2;
    }
    return ( *s1 - *s2 );
}
