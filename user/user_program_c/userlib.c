/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"

/// syscalls ///

void settextcolor(unsigned int foreground, unsigned int background)
{
    __asm__ volatile( "int $0x7F" : : "a"(2), "b"(foreground), "c"(background) );
}

void putch(unsigned char val)
{
    __asm__ volatile( "int $0x7F" : : "a"(1), "b"(val) );
}

void puts(char* pString)
{
    __asm__ volatile( "int $0x7F" : : "a"(0), "b"(pString) );
}

unsigned char getch()
{
    unsigned char ret;
    do
    {
        __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(6) );
    }
    while(ret==0);
    return ret;
}

int floppy_dir()
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(7) );
    return ret;
}

void printLine(char* message, unsigned int line, unsigned char attribute)
{
    __asm__ volatile( "int $0x7F" : : "a"(8), "b"(message), "c"(line), "d"(attribute) );
}

unsigned int getCurrentSeconds()
{
    unsigned int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(9) );
    return ret;
}

unsigned int getCurrentMilliseconds()
{
    unsigned int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(10) );
    return ret;
}

int floppy_format(char* volumeLabel)
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(11), "b"(volumeLabel) );
    return ret;
}

int floppy_load(char* name, char* ext)
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(12), "b"(name), "c"(ext)  );
    return ret;
}

void exit()
{
    __asm__ volatile( "int $0x7F" : : "a"(13) );
}

void settaskflag(int i)
{
    __asm__ volatile( "int $0x7F" : : "a"(14), "b"(i) );
}

void beep(unsigned int frequency, unsigned int duration)
{
    __asm__ volatile( "int $0x7F" : : "a"(15), "b"(frequency), "c"(duration)  );
}


/// ///////////////////////////// ///
///          user functions       ///
/// ///////////////////////////// ///

void test()
{
    settextcolor(4,0);
    puts(">>> TEST <<<");
    settextcolor(15,0);
}

char toLower(char c)
{
    return isupper(c) ? ('a' - 'A') + c : c;
}

char toUpper(char c)
{
    return islower(c) ? ('A' - 'a') + c : c;
}

char* toupper( char* s )
{
    int i;
    for(i=0;i<strlen(s);i++)
    {
        s[i]=toUpper(s[i]);
    }
    return s;
}

char* tolower( char* s )
{
    int i;
    for(i=0;i<strlen(s);i++)
    {
        s[i]=toLower(s[i]);
    }
    return s;
}

unsigned int strlen(const char* str)
{
    unsigned int retval;
    for(retval = 0; *str != '\0'; ++str)
        ++retval;
    return retval;
}

// Compare two strings. Returns -1 if str1 < str2, 0 if they are equal or 1 otherwise.
int strcmp( const char* s1, const char* s2 )
{
    while ( ( *s1 ) && ( *s1 == *s2 ) )
    {
        ++s1; ++s2;
    }
    return ( *s1 - *s2 );
}

char* gets(char* s) ///TODO: task switch has to be handled!
{
    int i=0;
    char c;

    ///TEST
    settaskflag(0);

    do
    {
        c = getch();
        putch(c);
        if(c==8)  // Backspace
        {
           if(i>0)
           {
              s[i]='\0';
           }
        }
        s[i] = c;
        i++;
    }
    while(c!=10); // Linefeed
    s[i]='\0';

    ///TEST
    settaskflag(1);

    return s;
}

/// http://en.wikipedia.org/wiki/Itoa
void reverse(char* s)
{
    int i, j;
    char c;

    for(i=0, j = strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/// http://en.wikipedia.org/wiki/Itoa
void itoa(int n, char* s)
{
    int i, sign;
    if((sign = n) < 0)  // record sign
    {
        n = -n;         // make n positive
    }
    i=0;
    do /* generate digits in reverse order */
    {
        s[i++] = n % 10 + '0';  // get next digit
    }
    while( (n /= 10) > 0 );     // delete it

    if(sign < 0)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

int atoi(char* s)
{
    int num=0,flag=0, i;
    for(i=0;i<=strlen(s);i++)
    {
        if(s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] -'0';
        }
        else if(s[0] == '-' && i==0)
        {
            flag =1;
        }
        else
        {
            break;
        }
    }
    if(flag == 1)
    {
        num = num * -1;
    }
    return num;
}











void showInfo(signed char val)
{
    static char* line1 = "   _______                _______      <>_<>                                    "    ;
    static char* line2 = "  (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                "   ;
    static char* line3 = " `-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                               "  ;

    int i;
    char temp1,temp2,temp3;

    if(val==1)
    {
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
