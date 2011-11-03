/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "dma.h"
#include "util/util.h"


/* The DMA (Direct Memory Access) controller allows the FDC to send data to the DMA, which can put the data in memory.
   While the FDC can be programmed to not use DMA, it is not very well supported on emulators or virtual machines.
   Because of this, the DMA is used for data transfers. */


const dma_channel_t dma_channel[4] = {{0, 0x87, 0x00, 0x01}, {1, 0x83, 0x02, 0x03}, {2, 0x81, 0x04, 0x05}, {3, 0x82, 0x06, 0x07}};


static void dma_action(void* address, uint16_t length, const dma_channel_t* channel, uint8_t mode)
{
    length--; // ISA DMA counts from 0

    outportb(0x0A,                 BIT(2) | channel->portNum); // Mask channel
    outportb(channel->pagePort,    (uintptr_t)address >> 16);  // Address: Bits 16-23 (External page register. Allows us to address up to 16 MiB)
    outportb(0x0C,                 0x00);                      // Reset flip-flop
    outportb(channel->addressPort, (uintptr_t)address);        // Address: Bits 0-7
    outportb(channel->addressPort, (uintptr_t)address >> 8);   // Address: Bits 8-15
    outportb(0x0C,                 0x00);                      // Reset flip-flop
    outportb(channel->counterPort, length);                    // Length: Lower byte
    outportb(channel->counterPort, length >> 8);               // Length: Higher byte
    outportb(0x0B,                 mode | channel->portNum);   // Mode
    outportb(0x0A,                 channel->portNum);          // Unmask channel
}

// Prepare the DMA for read transfer from hardware
void dma_read(void* dest, uint16_t length, const dma_channel_t* channel, DMA_TRANSFERMODE_t mode)
{
    dma_action(dest, length, channel, mode | DMA_WRITE);
}

// Prepare the DMA for write transfer to hardware
void dma_write(const void* source, uint16_t length, const dma_channel_t* channel, DMA_TRANSFERMODE_t mode)
{
    dma_action((void*)source, length, channel, mode | DMA_READ);
}


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
