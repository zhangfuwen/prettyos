/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "my_stdarg.h"

/// syscalls ///

void settextcolor(unsigned int foreground, unsigned int background)
{
    __asm__ volatile( "int $0x7F" : : "a"(2), "b"(foreground), "c"(background) );
}

void putch(unsigned char val)
{
    __asm__ volatile( "int $0x7F" : : "a"(1), "b"(val) );
}

void puts(const char* pString)
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
    while (ret==0);
    return ret;
}

int floppy_dir()
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(7) );
    return ret;
}

void printLine(const char* message, unsigned int line, unsigned char attribute)
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

int floppy_load(const char* name, const char* ext)
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

int getUserTaskNumber()
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(16) );
    return ret;
}

bool testch()
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(17) );
    return ret!=0;
}

void clearScreen(unsigned char backgroundColor)
{
    __asm__ volatile( "int $0x7F" : : "a"(18 ), "b"(backgroundColor) );
}

void gotoxy(unsigned char x, unsigned char y)
{
    __asm__ volatile( "int $0x7F" : : "a"(19), "b"(x), "c"(y)  );
}

void* grow_heap( unsigned increase )
{
    int ret;
    __asm__ volatile( "int $0x7F" : "=a"(ret): "a"(20), "b"(increase) );
    return (void*)ret;
}




/// ///////////////////////////// ///
///          user functions       ///
/// ///////////////////////////// ///

// printf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c
void printf (const char* args, ...)
{
    va_list ap;
    va_start (ap, args);
    char buffer[32]; // Larger is not needed at the moment

    for (; *args; args++)
    {
        switch (*args)
        {
        case '%':
            switch (*(++args))
            {
            case 'u':
                itoa(va_arg(ap, uint32_t), buffer);
                puts(buffer);
                break;
            case 'f':
                float2string(va_arg(ap, double), 10, buffer);
                puts(buffer);
                break;
            case 'i': case 'd':
                itoa(va_arg(ap, int32_t), buffer);
                puts(buffer);
                break;
            case 'X':
                i2hex(va_arg(ap, int32_t), buffer,8);
                puts(buffer);
                break;
            case 'x':
                i2hex(va_arg(ap, int32_t), buffer,4);
                puts(buffer);
                break;
            case 'y':
                i2hex(va_arg(ap, int32_t), buffer,2);
                puts(buffer);
                break;
            case 's':
                puts(va_arg (ap, char*));
                break;
            case 'c':
                putch((int8_t)va_arg(ap, int32_t));
                break;
            case '%':
                putch('%');
                break;
            default:
                --args;
                break;
            }
            break;
        default:
            putch(*args); //printf("%c",*(args+index));
            break;
        }
    }
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
    for (int i=0;i<strlen(s);i++)
    {
        s[i]=toUpper(s[i]);
    }
    return s;
}

char* tolower( char* s )
{
    for (int i=0;i<strlen(s);i++)
    {
        s[i]=toLower(s[i]);
    }
    return s;
}

unsigned int strlen(const char* str)
{
    unsigned int retval;
    for (retval = 0; *str != '\0'; ++str)
    {
        ++retval;
    }
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

/// http://en.wikipedia.org/wiki/Strcpy
// Copy the NUL-terminated string src into dest, and return dest.
char* strcpy(char* dest, const char* src)
{
   char* save = dest;
   while ( (*dest++ = *src++) );
   return save;
}

char* strncpy(char* dest, const char* src, size_t n) // okay?
{
    if (n != 0)
    {
        char* d = dest;
        do
        {
            if ((*d++ = *src++) == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                   *d++ = 0;
                break;
            }
        }
        while (--n != 0);
     }
     return (dest);
}

/// http://en.wikipedia.org/wiki/Strcat
char* strcat(char* dest, const char* src)
{
    strcpy(dest + strlen(dest), src);
    return dest;
}

char* strchr(char* str, int character)
{
    for (;;str++)
    {
        // NOTE< the order here is important >
        if ( *str == character )
        {
            return str;
        }
        if ( *str == 0 )
        {
            return 0;
        }
    }
}


char* gets(char* s)
{
    int i=0,flag=0;
    char c;
    //settaskflag(0);
    do
    {
        c = getch();
        //settextcolor(i,0);///TEST
        if (c=='\b')  // Backspace
        {
           if (i>0)
           {
              putch(c);
              s[i-1]='\0';
              --i;
           }
           else
           {
               beep(50,20);
               if (flag==false)
               {
                   putch('\n');
                   flag=true;
               }
           }
        }
        else
        {
            s[i] = c;
            putch(c);
            flag=false;
            i++;
        }
    }
    while (c!=10); // Linefeed
    s[i]='\0';

/*
    settextcolor(15,0);
    int j;
    for (j=0;j<15;j++)
    {
        if (s[j]=='\b')
        {
            puts("backspace");
        }
        else if (s[j]=='\0')
        {
            puts("EOS");
            putch('\n');
            break;
        }
        else if (s[j]=='\n')
        {
            puts("NL");
        }
        else
        {
            putch(s[j]);
        }
        putch('\n');
    }
*/
    //settaskflag(1);
    return s;
}

/// http://en.wikipedia.org/wiki/Itoa
void reverse(char* s)
{
    char c;

    for (int i=0,j=strlen(s)-1; i<j; i++, j--)
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
    if ((sign = n) < 0)  // record sign
    {
        n = -n;         // make n positive
    }
    i=0;
    do /* generate digits in reverse order */
    {
        s[i++] = n % 10 + '0';  // get next digit
    }
    while ( (n /= 10) > 0 );     // delete it

    if (sign < 0)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

int atoi(const char* s)
{
    int num=0,flag=0;
    for (int i=0;i<=strlen(s);i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] -'0';
        }
        else if (s[0] == '-' && i==0)
        {
            flag =1;
        }
        else
        {
            break;
        }
    }
    if (flag == 1)
    {
        num = num * -1;
    }
    return num;
}

void float2string(float value, int decimal, char* valuestring) // float --> string
{
   int neg = 0;
   char tempstr[20];
   int i=0;
   int c;
   int val1, val2;
   char* tempstring;

   tempstring = valuestring;
   if (value < 0)
   {
     {neg = 1; value = -value;}
   }
   for (int j=0; j < decimal; ++j)
   {
     {value = value * 10;}
   }
   val1 = (value * 2);
   val2 = (val1 / 2) + (val1 % 2);
   while (val2 !=0)
   {
     if ((decimal > 0) && (i == decimal))
     {
       tempstr[i] = (char)(0x2E);
       ++i;
     }
     else
     {
       c = (val2 % 10);
       tempstr[i] = (char) (c + 0x30);
       val2 = val2 / 10;
       ++i;
     }
   }
   if (neg)
   {
     *tempstring = '-';
      ++tempstring;
   }
   i--;
   for (;i > -1;i--)
   {
     *tempstring = tempstr[i];
     ++tempstring;
   }
   *tempstring = '\0';
}

void i2hex(uint32_t val, char* dest, int32_t len)
{
    char* cp;
    char  x;
    uint32_t n;
    n = val;
    cp = &dest[len];
    while (cp > dest)
    {
        x = n & 0xF;
        n >>= 4;
        *--cp = x + ((x > 9) ? 'A' - 10 : '0');
    }
    dest[len]  ='h';
    dest[len+1]='\0';
}





void showInfo(signed char val)
{
    static char* line1 = "   _______                _______      <>_<>                                    "    ;
    static char* line2 = "  (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                "   ;
    static char* line3 = " `-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                               "  ;

    if (val==1)
    {
        char temp1 = line1[79];
        char temp2 = line2[79];
        char temp3 = line3[79];

        for (int i=79;i>0;--i)
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

void test()
{
    settextcolor(4,0);
    puts(">>> TEST <<<");
    settextcolor(15,0);
}


void* malloc( size_t size )
{
    static char* cur = 0;
    static char* top = 0;

    // Align
    size = (size+15) & ~15;

    // Heap not set up?
    if ( ! cur )
    {
        unsigned to_grow = (size+4095) & ~4095;
        cur = grow_heap( to_grow );
        if ( ! cur )
            return 0;
        top = cur + to_grow;
    }
    // Not enough space on heap?
    else if ( top - cur < size )
    {
        unsigned to_grow = (size+4095) & ~4095;
        if ( ! grow_heap( to_grow ) )
            return 0;
        top += to_grow;
    }

    void* ret = cur;
    cur += size;
    return ret;
}

void free( void* mem )
{
    // Nothing to do
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
