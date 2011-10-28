/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"


bool enabledEvents = false;


void event_flush(EVENT_t filter)
{
    EVENT_t ev;
    do
    {
        ev = event_poll(0, 0, filter);
    } while(ev != EVENT_NONE);
}

void sleep(uint32_t milliseconds)
{
    wait(BL_TIME, 0, milliseconds);
}

bool waitForTask(uint32_t pid, uint32_t timeout)
{
    return (wait(BL_TASK, (void*)pid, timeout));
}

void iSetCursor(uint16_t x, uint16_t y)
{
    position_t temp;
    temp.x = x; temp.y = y;
    setCursor(temp);
}

char getchar()
{
    char ret = 0;
    EVENT_t ev = event_poll(&ret, 1, enabledEvents ? EVENT_TEXT_ENTERED : EVENT_NONE);

    while (ev != EVENT_TEXT_ENTERED)
    {
        if (ev == EVENT_NONE)
        {
            waitForEvent(0);
        }
        ev = event_poll(&ret, 1, enabledEvents ? EVENT_TEXT_ENTERED : EVENT_NONE);
    }
    return (ret);
}

uint32_t getCurrentSeconds()
{
    return (getCurrentMilliseconds()/1000);
}

void vsnprintf(char *buffer, size_t length, const char *args, va_list ap)
{
    char m_buffer[32]; // Larger is not needed at the moment

    for (size_t pos = 0; *args && pos < length; args++)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        utoa(va_arg(ap, uint32_t), m_buffer);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += strlen(m_buffer);
                        break;
                    case 'f':
                        ftoa(va_arg(ap, double), m_buffer);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += strlen(m_buffer);
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), m_buffer);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += strlen(m_buffer);
                        break;
                    case 'X':
                        i2hex(va_arg(ap, int32_t), m_buffer, 8);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += 8;
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), m_buffer, 4);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += 4;
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), m_buffer, 2);
                        strncpy(buffer+pos, m_buffer, length - pos);
                        pos += 2;
                        break;
                    case 's':
                    {
                        const char* string = va_arg(ap, const char*);
                        strncpy(buffer+pos, string, length - pos);
                        pos += strlen(string);
                        break;
                    }
                    case 'c':
                        buffer[pos] = (int8_t)va_arg(ap, int32_t);
                        buffer[pos+1] = 0;
                        pos++;
                        break;
                    case '%':
                        buffer[pos] = '%';
                        buffer[pos+1] = 0;
                        pos++;
                        break;
                    default:
                        --args;
                        break;
                    }
                break;
            default:
                buffer[pos] = (*args);
                pos++;
                break;
        }
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
    char* p = s;
    for (; *p; ++p)
        *p = toupper(*p);
    return s;
}

char* stolower(char* s)
{
    char* p = s;
    for (; *p; ++p)
        *p = tolower(*p);
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
    return (s);
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
    return (s);
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
    dest[len]='\0';
}

static void scrollInfoLine(char* line1, char* line2, char* line3)
{
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

void showInfo(uint8_t val)
{
    switch (val)
    {
        case 1:
        {
            static char* line1 = "   _______                __________      <>_<>                                 ";
            static char* line2 = "  (_______) |_|_|_|_|_|_|| [] [] [] | .---|'\"`|---.                             ";
            static char* line3 = " `-oo---oo-'`-oo-----oo-'`-oo----oo-'`o\"O-OO-OO-O\"o'                            ";
            scrollInfoLine(line1, line2, line3);
            break;
        }
        case 2:
        {
            static char* line1 = "       ___    ___    ___    ___                                                 ";
            static char* line2 = " ______\\  \\___\\  \\___\\  \\___\\  \\__________                                      ";
            static char* line3 = " \\ =  : : : : : : : : PrettyOS : : : : : /                                      ";
            scrollInfoLine(line1, line2, line3);
            break;
        }
    }
}


IP_t stringToIP(char* str)
{
    IP_t IP;
    for (uint8_t i_start = 0, i_end = 0, byte = 0; byte < 4; i_end++)
    {
        if (str[i_end] == 0)
        {
            IP.IP[byte] = atoi(str+i_start);
            break;
        }
        if (str[i_end] == '.')
        {
            str[i_end] = 0;
            IP.IP[byte] = atoi(str+i_start);
            i_start = i_end+1;
            byte++;
        }
    }
    return (IP);
}


/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
