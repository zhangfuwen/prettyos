/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"

// Syscalls
FS_ERROR execute(const char* path)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(0), "b"(path));
    return ret;
}

void exit()
{
    __asm__ volatile("int $0x7F" : : "a"(2));
}

void taskSleep(uint32_t duration)
{
    __asm__ volatile("int $0x7F" : : "a"(4), "b"(duration));
}

uint32_t getMyPID()
{
    uint32_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(6));
    return ret;
}

void* userheapAlloc(size_t increase)
{
    uintptr_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(10), "b"(increase));
    return (void*)ret;
}

file_t* fopen(const char* path, const char* mode)
{
    file_t* ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(15), "b"(path), "c"(mode));
    return ret;
}

char fgetc(file_t* file)
{
    char ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(16), "b"(file));
    return ret;
}

FS_ERROR fputc(char value, file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(17), "b"(value), "c"(file));
    return ret;
}

FS_ERROR fseek(file_t* file, size_t offset, SEEK_ORIGIN origin)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(18), "b"(file), "c"(offset), "d"(origin));
    return ret;
}

FS_ERROR fflush(file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(19), "b"(file));
    return ret;
}

FS_ERROR fclose(file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(21), "b"(file));
    return ret;
}

uint32_t getCurrentMilliseconds()
{
    uint32_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(40));
    return ret;
}

void systemControl(SYSTEM_CONTROL todo)
{
    __asm__ volatile("int $0x7F" : : "a"(50), "b"(todo));
}

void putch(char val)
{
    __asm__ volatile("int $0x7F" : : "a"(55), "b"(val));
}

void textColor(uint8_t color)
{
    __asm__ volatile("int $0x7F" : : "a"(56), "b"(color));
}

void setScrollField(uint8_t top, uint8_t bottom)
{
    __asm__ volatile("int $0x7F" : : "a"(57), "b"(top), "c"(bottom));
}

void setCursor(position_t pos)
{
    __asm__ volatile("int $0x7F" : : "a"(58), "b"(pos));
}

position_t getCursor()
{
    position_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(59));
    return ret;
}

void clearScreen(uint8_t backgroundColor)
{
    __asm__ volatile("int $0x7F" : : "a"(61), "b"(backgroundColor));
}

char getch()
{
    char ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(70));
    return ret;
}

bool keyPressed(VK key)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(71), "b"(key));
    return ret;
}

void beep(uint32_t frequency, uint32_t duration)
{
    __asm__ volatile("int $0x7F" : : "a"(80), "b"(frequency), "c"(duration));
}

 // deprecated
int32_t floppy_dir()
{
    int32_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(90));
    return ret;
}
void printLine(const char* message, uint32_t line, uint8_t attribute)
{
    if (line <= 45) // User may only write in his own area (size is 45)
    {
        __asm__ volatile("int $0x7F" : : "a"(91), "b"(message), "c"(line), "d"(attribute));
    }
}
int32_t floppy_format(char* volumeLabel)
{
    int32_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(92), "b"(volumeLabel));
    return ret;
}


// user functions
void iSetCursor(uint16_t x, uint16_t y)
{
	position_t temp;
	temp.x = x; temp.y = y;
	setCursor(temp);
}

uint32_t getCurrentSeconds()
{
    return(getCurrentMilliseconds()/1000);
}

void* memset(void* dest, int8_t val, size_t count)
{
    int8_t* temp = (int8_t*)dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count)
{
    const uint8_t* sp = (const uint8_t*)src;
    uint8_t* dp = (uint8_t*)dest;
    for (; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void* memmove(const void* source, void* destination, size_t size)
{
    // If source and destination point to the same memory area, coping something
    // is not necessary. It could be seen as a bug of the caller to pass the
    // same value for source and destination, but we decided to allow this and
    // just do nothing in this case.
    if(source == destination)
    {
        return(destination);
    }

    // If size is 0, just return. It is not a bug to call this function with size set to 0.
    if(size == 0)
    {
        return(destination);
    }

    // Check if either one of the memory regions is beyond the end of the
    // address space. We think that trying to copy from or to beyond the end of
    // the address space is a bug of the caller.
    // In future versions of this function, an exception will be thrown in this
    // case. For now, just return and do nothing.
    // The subtraction used to calculate the value of "max" cannot produce an
    // underflow because size can neither be greater than the maximum value of a
    // variable of type uintp or 0.
    const uintptr_t memMax = ~((uintptr_t)0) - (size - 1); // ~0 is the highest possible value of the variables type
    if((uintptr_t)source > memMax || (uintptr_t)destination > memMax)
    {
        return(destination);
    }

    const uint8_t* source8 = (uint8_t*)source;
    uint8_t* destination8 = (uint8_t*)destination;

    // The source overlaps with the destination and the destination is after the
    // source in memory. Coping from start to the end of source will overwrite
    // the last (size - (destination - source)) bytes of source with the first
    // ones. Therefore it is necessary to copy from the end to the start of
    // source and destination. Let us look at an example. Each letter or space
    // is one byte in memory.
    // |source     |
    // |      destination|
    // source starts at 0. destination at 6. Coping from start to end will
    // overwrite the last 5 bytes of the source.
    if(source8 < destination8)
    {
        source8 += size - 1;
        destination8 += size - 1;
        while(size > 0)
        {
            *destination8 = *source8;
            --destination8;
            --source8;
            --size;
        }
    }
    else // In all other cases, it is ok to copy from the start to the end of source.
    {
        while(size > 0)
        {
            *destination8 = *source8;
            ++destination8;
            ++source8;
            --size;
        }
    }
    return(destination);
}


void puts(const char* str)
{
    for (size_t i = 0; str[i] != 0; i++)
    {
        putch(str[i]);
    }
}
// printf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c
void printf(const char* args, ...) {
    va_list ap;
    va_start(ap, args);
    vprintf(args, ap);
    va_end(ap);
}
void vprintf(const char* args, va_list ap)
{
    char buffer[32]; // Larger is not needed at the moment

    for (; *args; args++)
    {
        switch (*args)
        {
        case '%':
            switch (*(++args))
            {
            case 'u':
                utoa(va_arg(ap, uint32_t), buffer);
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
            putch(*args);
            break;
        }
    }
}

void vsprintf(char *buffer, const char *args, va_list ap)
{
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
                        utoa(va_arg(ap, uint32_t), m_buffer);
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

void sprintf(char *buffer, const char *args, ...)
{
    va_list ap;
    va_start(ap, args);
    vsprintf(buffer, args, ap);
    va_end(ap);
}

void vsnprintf(char *buffer, size_t length, const char *args, va_list ap)
{
    char m_buffer[32]; // Larger is not needed at the moment
    memset(buffer, 0, length);

    for (size_t pos = 0; *args && pos < length; args++)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        utoa(va_arg(ap, uint32_t), m_buffer);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'f':
                        ftoa(va_arg(ap, double), m_buffer);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), m_buffer);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'X':
                        i2hex(va_arg(ap, int32_t), m_buffer, 8);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), m_buffer, 4);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), m_buffer, 2);
                        strncat(buffer, m_buffer, length - pos - 1);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 's':
                        strncat(buffer, va_arg (ap, char*), length - pos - 1);
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
    }
}

void snprintf(char *buffer, size_t length, const char *args, ...)
{
    va_list ap;
    va_start(ap, args);
    vsnprintf(buffer, length, args, ap);
    va_end(ap);
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
    for (size_t i = 0; i < strlen(s); i++)
    {
        s[i] = toUpper(s[i]);
    }
    return s;
}

char* tolower(char* s)
{
    for (size_t i = 0; i < strlen(s); i++)
    {
        s[i] = toLower(s[i]);
    }
    return s;
}

size_t strlen(const char* str)
{
    size_t retval;
    for (retval = 0; *str != '\0'; ++str)
    {
        ++retval;
    }
    return retval;
}

// Compare two strings. Returns -1 if str1 < str2, 0 if they are equal or 1 otherwise.
int32_t strcmp(const char* s1, const char* s2)
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

/// http://en.wikipedia.org/wiki/Strcat
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
        if (*str == 0)
        {
            return 0;
        }
    }
}


char* gets(char* s)
{
    int32_t i = 0;
    char c;
    do
    {
        c = getch();
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
           }
        }
        else
        {
            s[i] = c;
            putch(c);
            i++;
        }
    }
    while (c!=10); // Linefeed
    s[i]='\0';

    return s;
}

/// http://en.wikipedia.org/wiki/Itoa
void reverse(char* s)
{
    for (size_t i = 0, j = strlen(s)-1; i < j; i++, j--)
    {
        char c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/// http://en.wikipedia.org/wiki/Itoa
char* itoa(int32_t n, char* s)
{
    bool sign = n < 0;
    if (sign)   // record sign
    {
        n = -n; // make n positive
    }
    uint32_t i = 0;
    do // generate digits in reverse order
    {
        s[i++] = n % 10 + '0'; // get next digit
    }
    while ((n /= 10) > 0);     // delete it

    if (sign)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
    return(s);
}

char* utoa(uint32_t n, char* s)
{
    uint32_t i = 0;
    do // generate digits in reverse order
    {
        s[i++] = n % 10 + '0';  // get next digit
    }
    while ((n /= 10) > 0);     // delete it
    s[i] = '\0';
    reverse(s);
    return(s);
}

int32_t atoi(const char* s)
{
    int32_t num = 0;
    bool sign = false;
    for (size_t i=0; i<=strlen(s); i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            num = num * 10 + s[i] -'0';
        }
        else if (s[0] == '-' && i==0)
        {
            sign = true;
        }
        else
        {
            break;
        }
    }
    if (sign)
    {
        num *= -1;
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

void i2hex(uint32_t val, char* dest, uint32_t len)
{
    char* cp = &dest[len];
    while (cp > dest)
    {
        char x = val & 0xF;
        val >>= 4;
        *--cp = x + ((x > 9) ? 'A' - 10 : '0');
    }
    dest[len]  ='h';
    dest[len+1]='\0';
}


void showInfo(uint8_t val)
{
    if (val==1)
    {
        static char* line1 = "   _______                _______      <>_<>                                    ";
        static char* line2 = "  (_______) |_|_|_|_|_|_|| [] [] | .---|'\"`|---.                                ";
        static char* line3 = " `-oo---oo-'`-oo-----oo-'`-o---o-'`o\"O-OO-OO-O\"o'                               ";
        char temp1 = line1[79];
        char temp2 = line2[79];
        char temp3 = line3[79];

        for (uint8_t i=79; i>0; --i)
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
    if (!cur)
    {
        uint32_t sizeToGrow = (size+4095) & ~4095;
        cur = userheapAlloc(sizeToGrow);
        if (!cur)
            return 0;
        top = cur + sizeToGrow;
    }
    // Not enough space on heap?
    else if (top - cur < size)
    {
        uint32_t sizeToGrow = (size+4095) & ~4095;
        if (!userheapAlloc(sizeToGrow))
            return 0;
        top += sizeToGrow;
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

/// random generator
static uint32_t seed = 0;

void srand(uint32_t val)
{
    seed = val;
}

uint32_t rand()
{
    return (((seed = seed * 214013L + 2531011L) >> 16) & 0x7FFF);
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
