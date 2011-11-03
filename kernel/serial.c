/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://wiki.osdev.org/Serial_ports

#include "serial.h"
#include "video/console.h"
#include "util/util.h"


static uint8_t  serialPorts;
static uint16_t IOports[4]; // Contains the ports used to access


void serial_init()
{
    textColor(LIGHT_GRAY);
    printf("\n   => Serial ports:");
    serialPorts = (((*(uint16_t*)0x410)>>9)&0x7); // Read from BIOS Data Area (BDA)

    IOports[0]  = *((uint16_t*)0x400);
    IOports[1]  = *((uint16_t*)0x402);
    IOports[2]  = *((uint16_t*)0x404);
    IOports[3]  = *((uint16_t*)0x406);

    for (uint8_t i = 0; i < serialPorts; i++)
    {
        outportb(IOports[i] + 1, 0x00); // Disable all interrupts
        outportb(IOports[i] + 3, 0x80); // Enable DLAB (set baud rate divisor)
        //outportb(IOports[i] + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud (DO NOT TRY 0!!!)
        outportb(IOports[i] + 0, 0x01); // Set divisor to 1 (lo byte)
        outportb(IOports[i] + 1, 0x00); //                  (hi byte)
        outportb(IOports[i] + 3, 0x03); // 8 bits, no parity, one stop bit
        outportb(IOports[i] + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
        outportb(IOports[i] + 4, 0x0B); // IRQs enabled, RTS/DSR set
        textColor(LIGHT_GRAY);
        printf("\n     => COM %d:", i+1);
        printf("\n       => IO-port: ");
        textColor(TEXT);
        printf("%xh",IOports[i]);
    }

    printf("\n\n");
}

bool serial_received(uint8_t com)
{
    if (com <= serialPorts)
    {
        return inportb(IOports[com-1] + 5) & 1;
    }
    return false;
}

char serial_read(uint8_t com)
{
    if (com <= serialPorts)
    {
        while (serial_received(com) == false);
        return inportb(IOports[com-1]);
    }
    return (0);
}

bool serial_isTransmitEmpty(uint8_t com)
{
    if (com <= serialPorts)
        return inportb(IOports[com-1] + 5) & 0x20;
    return (true);
}

void serial_write(uint8_t com, char a)
{
    if (com <= serialPorts)
    {
        while (serial_isTransmitEmpty(com) == 0);
        outportb(IOports[com-1], a);
    }
}

#ifdef _SERIAL_LOG_
void serial_log(uint8_t com, const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    size_t length = strlen(msg) + 100;
    char array[length]; // HACK: Should be large enough.
    vsnprintf(array, length, msg, ap);
    for(size_t i = 0; i < length && array[i] != 0; i++)
    {
        serial_write(com, array[i]);
    }
}
#endif


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
