/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

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

int floppy_dir()
{
    int ret;
    asm volatile( "int $0x7F" : "=a"(ret): "a"(8) );
    return ret;
}

void printLine(char* message, unsigned int line, unsigned char attribute)
{
    asm volatile( "int $0x7F" : : "a"(9), "b"(message), "c"(line), "d"(attribute) );
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

void showInfo(signed char val)
{
    char* line1 = "  _______                _______      <>_<>                                     "    ;
    char* line2 = " (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                 "   ;
    char* line3 = "`-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                                "  ;

    int i;
    char temp1,temp2,temp3;



    switch(val)
    {
        case 1:
            temp1 = line1[79];
            temp2 = line2[79];
            temp3 = line3[79];

            for(i=79;i>0;--i)
            {
                line1[i] = line1[i-1];
                line2[i] = line2[i-1];
                line3[i] = line3[i-1];
            }
            line1[0] = temp1;
            line2[0] = temp2;
            line3[0] = temp3;
            printLine(line1,46,0xE);
            printLine(line2,47,0xE);
            printLine(line3,48,0xE);
        break;

        default:
        break;
    }

}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
