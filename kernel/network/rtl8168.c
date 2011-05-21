/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "rtl8168.h"
#include "util.h"
#include "timer.h"
#include "paging.h"
#include "kheap.h"
#include "video/console.h"


static RTL8168_networkAdapter_t* RTL; // HACK

static const uint16_t numOfDesc = 1024; // Can be up to 1024


void rtl8168_handler(registers_t* data)
{
    printf("IRQ: RTL8168");
    uint32_t intStatus = *(uint32_t*)(RTL->device->MMIO_base + RTL8168_INTRSTATUS);
    printf("\t\t Status: %X", intStatus);
}

void setupDescriptors()
{
    RTL->Rx_Descriptors = malloc(numOfDesc*sizeof(RTL8168_Desc), 256, "Rx Desc");
    RTL->Tx_Descriptors = malloc(numOfDesc*sizeof(RTL8168_Desc), 256, "Tx Desc");
    // rx_buffer_len is the size (in bytes) that is reserved for incoming packets
    unsigned int OWN = 0x80000000, EOR = 0x40000000; // bit offsets
    for(uint16_t i = 0; i < numOfDesc; i++)
    {
        if(i == (1024 - 1)) // Last descriptor? if so, set the EOR bit
            RTL->Rx_Descriptors[i].command = (OWN | EOR | (2048 & 0x3FFF));
        else
            RTL->Rx_Descriptors[i].command = (OWN | (2048 & 0x3FFF));
        RTL->Rx_Descriptors[i].low_buf = paging_getPhysAddr(RTL->RxBuffer); // This is where the packet data will go. TODO: We will need more buffers...
        RTL->Rx_Descriptors[i].high_buf = 0;
    }
    printf("\nDescriptors are set up.");
}

void install_RTL8168(network_adapter_t* device)
{
    printf("\nInstalling RTL8168");

    RTL = malloc(sizeof(RTL8168_networkAdapter_t), 4, "RTL8168");
    device->data = RTL;
    RTL->device = device;

    // Acquire memory
    printf("\nMMIO_base (phys): %X", device->MMIO_base);
    device->MMIO_base = paging_acquirePciMemory((uintptr_t)device->MMIO_base, 1);
    printf("\t\tMMIO_base (virt): %X", device->MMIO_base);

    // Reset card
    *((uint8_t*)(device->MMIO_base + RTL8168_CHIPCMD)) = RTL8168_CMD_RESET;

    for(uint8_t k = 0; ; k++) // wait for the reset of the "reset flag"
    {
        sleepMilliSeconds(10);
        if (!(*((volatile uint8_t*)(device->MMIO_base + RTL8168_CHIPCMD)) & RTL8168_CMD_RESET))
        {
            printf("\nwaiting successful (%d).\n", k);
            break;
        }
        if (k > 100)
        {
            printf("\nWaiting not successful! Finished by timeout.\n");
            break;
        }
    }

    // Get MAC
    for (uint8_t i = 0; i < 6; i++)
    {
        device->MAC_address[i] =  *(uint8_t*)(device->MMIO_base + RTL8168_IDR0 + i);
    }

    setupDescriptors();

    *(uint8_t*)(device->MMIO_base + RTL8168_CFG9346) = 0xC0; // Unlock config registers
    *(uint32_t*)(device->MMIO_base + RTL8168_RXCONFIG) = 0x0000E70F; // RxConfig = RXFTH: unlimited, MXDMA: unlimited, AAP: set (promisc. mode set)
    *(uint32_t*)(device->MMIO_base + RTL8168_TXCONFIG) = 0x03000700; // TxConfig = IFG: normal, MXDMA: unlimited
    *(uint16_t*)(device->MMIO_base + 0xDA) = 0x1FFF; // Max rx packet size
    *(uint8_t*)(device->MMIO_base + 0xEC) = 0x3B; // max tx packet size

    *(uint32_t*)(device->MMIO_base + RTL8168_TXADDR0) = paging_getPhysAddr(RTL->Tx_Descriptors); // Tell the NIC where the first Tx descriptor is
    *(uint32_t*)(device->MMIO_base + RTL8168_RXADDR0) = paging_getPhysAddr(RTL->Rx_Descriptors); // Tell the NIC where the first Rx descriptor is

    *(uint8_t*)(device->MMIO_base + RTL8168_CHIPCMD) = 0x0C; // Enable Rx/Tx in the Command register
    *(uint8_t*)(device->MMIO_base + RTL8168_CFG9346) = 0x00; // Lock config registers

    printf("\nRTL8168 configured");
}


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
