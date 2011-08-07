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

// Syscalls
FS_ERROR execute(const char* path, size_t argc, char* argv[])
{
    FS_ERROR ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(0), "b"(path), "c"(argc), "d"(argv));
    return ret;
}

void exit()
{
    __asm__ volatile("int $0x7F" : : "a"(2));
}

bool wait(BLOCKERTYPE reason, void* data, uint32_t timeout)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(3), "b"(reason), "c"(data), "d"(timeout/10)); // HACK. Unbound it from system frequency. cf. scheduler.c
    return ret;
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

bool waitForEvent(uint32_t timeout)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(37), "b"(timeout));
    return ret;
}

void event_enable(bool b)
{
    enabledEvents = b;
    if(b)
    {
        __asm__ volatile("int $0x7F" : : "a"(38), "b"(b));
    }
}

EVENT_t event_poll(void* destination, size_t maxLength, EVENT_t filter)
{
    EVENT_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(39), "b"(destination), "c"(maxLength), "d"(filter));
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

void getCursor(position_t* pos)
{
    __asm__ volatile("int $0x7F" : : "a"(59), "b"(pos));
}

void clearScreen(uint8_t backgroundColor)
{
    __asm__ volatile("int $0x7F" : : "a"(61), "b"(backgroundColor));
}

char getchar()
{
    char ret = 0;
    EVENT_t ev = event_poll(&ret, 1, enabledEvents ? EVENT_TEXT_ENTERED : EVENT_NONE);

    while(ev != EVENT_TEXT_ENTERED)
    {
        if(ev == EVENT_NONE)
        {
            waitForEvent(0);
        }
        ev = event_poll(&ret, 1, enabledEvents ? EVENT_TEXT_ENTERED : EVENT_NONE);
    }
    return(ret);
}

bool keyPressed(KEY_t key)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(71), "b"(key));
    return ret;
}

void beep(uint32_t frequency, uint32_t duration)
{
    __asm__ volatile("int $0x7F" : : "a"(80), "b"(frequency), "c"(duration));
}

uint32_t tcp_connect(IP_t IP, uint16_t port)
{
    uint32_t ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(85), "b"(IP), "c"(port));
    return ret;
}

bool tcp_send(uint32_t ID, void* data, size_t length)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(86), "b"(ID), "c"(data), "d"(length));
    return ret;
}

bool tcp_close(uint32_t ID)
{
    bool ret;
    __asm__ volatile("int $0x7F" : "=a"(ret): "a"(87), "b"(ID));
    return ret;
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
    if (line <= 45) // User must not write outside of client area (size is 45)
    {
        __asm__ volatile("int $0x7F" : : "a"(91), "b"(message), "c"(line), "d"(attribute));
    }
}


// user functions
void sleep(uint32_t milliseconds)
{
    wait(BL_TIME, 0, milliseconds);
}

bool waitForTask(uint32_t pid, uint32_t timeout)
{
    return(wait(BL_TASK, (void*)pid, timeout));
}

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
    dest[len]='\0';
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
            break;
        }
        case 2:
        {
            // TODO: new info line
            break;
        }
    }
}


IP_t stringToIP(char* str)
{
    IP_t IP;
    for(uint8_t i_start = 0, i_end = 0, byte = 0; byte < 4; i_end++)
    {
        if(str[i_end] == 0)
        {
            IP.IP[byte] = atoi(str+i_start);
            break;
        }
        if(str[i_end] == '.')
        {
            str[i_end] = 0;
            IP.IP[byte] = atoi(str+i_start);
            i_start = i_end+1;
            byte++;
        }
    }
    return(IP);
}


IP_t resolveIP(const char* host)
{
    IP_t IP = {.IP = {82, 100, 220, 68}}; // www.henkessoft.de (web site of Dr. Erhard Henkes) 

    textColor(0x0F);
    printf("Resolving DNS...\n");
    uint32_t connection = tcp_connect(IP, 80);
    printf(" Connecting to DNS Server...", connection);

    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TCP_CONNECTED:
            {
                textColor(0x0A);
                printf("OK\n");
                textColor(0x0F);

                printf(" Sending DNS Request...");
                char pStr[200];
                strcpy(pStr,"GET /PrettyOS/resolve.php");
                strcat(pStr,"?hostname=");
                strcat(pStr,host);
                strcat(pStr," HTTP/1.0\r\nHost: ");
                strcat(pStr,"www.henkessoft.de");
                strcat(pStr,"\r\nConnection: close\r\n\r\n");

                tcp_send(connection, pStr, strlen(pStr));
                textColor(0x0A);
                printf("OK\n");
                textColor(0x0F);

                printf(" Waiting for answer...");
                break;
            }
            case EVENT_TCP_RECEIVED:
            {
                textColor(0x0A);
                printf("OK\n");
                textColor(0x0F);

                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;

                tcp_close(connection);

                printf(" Interpreting data...");

                char* deststring = strstr(data,"\r\n\r\n") + 4;

                IP_t resIP = stringToIP(deststring);

                textColor(0x0A);
                printf("OK\n");
                textColor(0x0F);

                printf("\nResolved IP is:\n", deststring);
                printf("%u.%u.%u.%u\n\n",resIP.IP[0],resIP.IP[1],resIP.IP[2],resIP.IP[3]);

                return resIP;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if(*key == KEY_ESC)
                {
                    tcp_close(connection);

                    IP_t noIP = {.iIP = 0};
                    return noIP;
                }
                break;
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(connection);

    IP_t noIP = {.iIP = 0};
    return noIP;
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
