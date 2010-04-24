/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "my_stdarg.h"


/// Syscalls

void puts(const char* pString)
{
    __asm__ volatile("int $0x7F" : : "a"(0), "b"(pString));
}

void putch(unsigned char val)
{
    __asm__ volatile("int $0x7F" : : "a"(1), "b"(val));
}

void settextcolor(unsigned int foreground, unsigned int background)
{
    __asm__ volatile("int $0x7F" : : "a"(2), "b"(foreground), "c"(background));
}

unsigned char getch()
{
    unsigned char ret;
    do
    {
        __asm__ volatile("int $0x7F" : "=a"(ret): "a"(6));
    }
    while (ret==0);
    return ret;
}

int floppy_dir()
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(7));
    return ret;
}

void printLine(const char* message, unsigned int line, unsigned char attribute)
{
    if (line <= 45) // User may only write in his own area (size is 45)
    {
        __asm__ volatile("int $0x7F" : : "a"(8), "b"(message), "c"(line), "d"(attribute));
    }
}

unsigned int getCurrentSeconds()
{
    unsigned int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(9));
    return ret;
}

unsigned int getCurrentMilliseconds()
{
    unsigned int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(10));
    return ret;
}

int floppy_format(char* volumeLabel)
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(11), "b"(volumeLabel));
    return ret;
}

int floppy_load(const char* name, const char* ext)
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(12), "b"(name), "c"(ext));
    return ret;
}

void exit()
{
    __asm__ volatile("int $0x7F" : : "a"(13));
}

void settaskflag(int i)
{
    __asm__ volatile("int $0x7F" : : "a"(14), "b"(i));
}

void beep(unsigned int frequency, unsigned int duration)
{
    __asm__ volatile("int $0x7F" : : "a"(15), "b"(frequency), "c"(duration));
}

int getUserTaskNumber()
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(16));
    return ret;
}

bool testch()
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(17));
    return ret!=0;
}

void clearScreen(unsigned char backgroundColor)
{
    __asm__ volatile("int $0x7F" : : "a"(18), "b"(backgroundColor));
}

void gotoxy(unsigned char x, unsigned char y)
{
    __asm__ volatile("int $0x7F" : : "a"(19), "b"(x), "c"(y));
}

void* grow_heap(unsigned increase)
{
    int ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(20), "b"(increase));
    return (void*)ret;
}

void setScrollField(uint8_t top, uint8_t bottom) {
    __asm__ volatile("int $0x7F" : : "a"(21), "b"(top), "c"(bottom));
}


/// user functions

void* memcpy(void* dest, const void* src, size_t count)
{
    const uint8_t* sp = (const uint8_t*)src;
    uint8_t* dp = (uint8_t*)dest;
    for (; count != 0; count--) *dp++ = *sp++;
    return dest;
}

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
                ftoa(va_arg(ap, double), buffer);
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

void sprintf (char *buffer, const char *args, ...)
{
    va_list ap;
    va_start (ap, args);
    int pos = 0;
    char m_buffer[32]; // Larger is not needed at the moment
    buffer[0] = '\0';

    for (; *args; args++)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        itoa(va_arg(ap, uint32_t), m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'f':
                        ftoa(va_arg(ap, double), m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'X':
                        i2hex(va_arg(ap, int32_t), m_buffer,8);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), m_buffer,4);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), m_buffer,2);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 's':
                        strcat(buffer, va_arg (ap, char*));
                        pos = strlen(buffer) - 1;
                        break;
                    case 'c':
                        buffer[pos] = (int8_t)va_arg(ap, int32_t);
                        break;
                    case '%':
                        buffer[pos] = '%';
                        break;
                    default:
                        --args;
                        --pos;
                        break;
                    }
                break;
            default:
                buffer[pos] = (*args);
                break;
        }
        pos++;
        buffer[pos] = '\0';
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

char* toupper(char* s)
{
    for (int i=0;i<strlen(s);i++)
    {
        s[i] = toUpper(s[i]);
    }
    return s;
}

char* tolower(char* s)
{
    for (int i=0;i<strlen(s);i++)
    {
        s[i] = toLower(s[i]);
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
int strcmp(const char* s1, const char* s2)
{
    while ((*s1) && (*s1 == *s2))
    {
        ++s1; ++s2;
    }
    return (*s1 - *s2);
}

/// http://en.wikipedia.org/wiki/Strcpy
// Copy the NUL-terminated string src into dest, and return dest.
char* strcpy(char* dest, const char* src)
{
   char* save = dest;
   while ((*dest++ = *src++));
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
        // the order here is important
        if (*str == character)
        {
            return str;
        }
        if (*str == 0)
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
    int sign;
    if ((sign = n) < 0) // record sign
    {
        n = -n;         // make n positive
    }
    int i=0;
    do // generate digits in reverse order
    {
        s[i++] = n % 10 + '0'; // get next digit
    }
    while ((n /= 10) > 0);     // delete it

    if (sign < 0)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

int atoi(const char* s)
{
    int num=0, flag=0;
    for (int i=0; i<=strlen(s); i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] -'0';
        }
        else if (s[0] == '-' && i==0)
        {
            flag = 1;
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

float atof(const char* s)
{
    int32_t i = 0;
    int8_t sign = 1;
    while(s[i] == ' ' || s[i] == '+' || s[i] == '-')
    {
        if(s[i] == '-')
        {
            sign *= -1;
        }
        i++;
    }

    float val;
    for (val = 0.0; isdigit(s[i]); i++)
    {
        val = 10.0 * val + s[i] - '0';
    }
    if (s[i] == '.')
    {
        i++;
    }
    float pow;
    for (pow = 1.0; isdigit(s[i]); i++)
    {
        val = 10.0 * val + s[i] - '0';
        pow *= 10.0;
    }
    return(sign * val / pow);
}

void ftoa(float f, char* buffer)
{
    char tmp[32];
    int32_t index = sizeof(tmp) - 1;
    if (f < 0.0)
    {
        *buffer = '-';
        ++buffer;
        f = -f;
    }

    int32_t i = f;
    while (i > 0)
    {
        tmp[index] = ('0' + (i % 10));
        i /= 10;
        --index;
    }
    memcpy((void*)buffer, (void*)&tmp[index + 1], sizeof(tmp) - 1 - index);
    buffer += sizeof(tmp) - 1 - index;
    *buffer = '.';
    ++buffer;

    *buffer++ = ((uint32_t)(f * 10.0) % 10) + '0';
    *buffer++ = ((uint32_t)(f * 100.0) % 10) + '0';
    *buffer++ = ((uint32_t)(f * 1000.0) % 10) + '0';
    *buffer++ = ((uint32_t)(f * 10000.0) % 10) + '0';
    *buffer++ = ((uint32_t)(f * 100000.0) % 10) + '0';
    *buffer++ = ((uint32_t)(f * 1000000.0) % 10) + '0';
    *buffer   = '\0';
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
    if (val==1)
    {
        static char* line1 = "   _______                _______      <>_<>                                    ";
        static char* line2 = "  (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                ";
        static char* line3 = " `-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                               ";
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
        printLine(line1,43,0xE);
        printLine(line2,44,0xE);
        printLine(line3,45,0xE);
    }
}


void* malloc(size_t size)
{
    static char* cur = 0;
    static char* top = 0;

    // Align
    size = (size+15) & ~15;

    // Heap not set up?
    if (! cur)
    {
        unsigned to_grow = (size+4095) & ~4095;
        cur = grow_heap(to_grow);
        if (! cur)
            return 0;
        top = cur + to_grow;
    }
    // Not enough space on heap?
    else if (top - cur < size)
    {
        unsigned to_grow = (size+4095) & ~4095;
        if (! grow_heap(to_grow))
            return 0;
        top += to_grow;
    }

    void* ret = cur;
    cur += size;
    return ret;
}

void free(void* mem)
{
    // Nothing to do
}


/// math functions

double cos(double x)
{
    double result;
    __asm__ volatile("fcos;" : "=t" (result) : "0" (x));
    return result;
}

double sin(double x)
{
    double result;
    __asm__ volatile("fsin;" : "=t" (result) : "0" (x));
    return result;
}

double tan(double x)
{
    double result;
    __asm__ volatile("fptan; fstp %%st(0)": "=t" (result) : "0" (x));
    return result;
}

double acos(double x)
{
    if (x < -1 || x > 1)
        return NAN;

    return(pi / 2 - asin(x));
}

double asin(double x)
{
    if (x < -1 || x > 1)
        return NAN;

    return(2 * atan(x / (1 + sqrt(1 - (x * x)))));
}

double atan(double x)
{
    double result;
    __asm__ volatile("fld1; fpatan" : "=t" (result) : "0" (x));
    return result;
}

double atan2(double x, double y)
{
    double result;
    __asm__ volatile("fpatan" : "=t" (result) : "0" (y), "u" (x));
    return result;
}

double sqrt(double x)
{
    if (x <  0.0)
        return NAN;

    double result;
    __asm__ volatile("fsqrt" : "=t" (result) : "0" (x));
    return result;
}

double fabs(double x)
{
    double result;
    __asm__ volatile("fabs" : "=t" (result) : "0" (x));
    return result;
}


/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
