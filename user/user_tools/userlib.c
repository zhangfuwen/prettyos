/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"

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

int fputc(char value, file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(17), "b"(value), "c"(file));
    return ret;
}

int fseek(file_t* file, size_t offset, SEEK_ORIGIN origin)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(18), "b"(file), "c"(offset), "d"(origin));
    return ret;
}

int fflush(file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(19), "b"(file));
    return ret;
}

int fclose(file_t* file)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(21), "b"(file));
    return ret;
}

FS_ERROR partition_format(const char* path, FS_t type, const char* name)
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(22), "b"(path), "c"(type), "d"(name));
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

int putchar(char val)
{
    __asm__ volatile("int $0x7F" : : "a"(55), "b"(val));
    return(0); // HACK
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

char getchar()
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

char* stoupper(char* s)
{
    for (size_t i = 0; i < strlen(s); i++)
    {
        s[i] = toupper(s[i]);
    }
    return s;
}

char* stolower(char* s)
{
    for (size_t i = 0; i < strlen(s); i++)
    {
        s[i] = tolower(s[i]);
    }
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
