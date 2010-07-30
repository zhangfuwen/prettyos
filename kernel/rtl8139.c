/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "irq.h"
#include "pci.h"
#include "paging.h"
#include "video/console.h"
#include "ipTcpStack.h"
#include "rtl8139.h"

uint32_t BaseAddressRTL8139_IO;
uint32_t BaseAddressRTL8139_MMIO;

// to set the WRAP bit, an 8K buffer must in fact be 8 KiB + 16 byte + 1.5 KiB
// Rx buffer + header + largest potentially overflowing packet
uint8_t network_buffer[8192+16+1536];  

void rtl8139_handler(registers_t* r)
{
    /// TODO: ring buffer, we get always the first received data!

    // read bytes 003Eh bis 003Fh, Interrupt Status Register
    uint16_t val = *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3E));

    char str[80];
    strcpy(str,"");

    if ((val & 0x1) == 0x1)
    {
        strcpy(str,"Receive OK,");
    }
    if ((val & 0x4) == 0x4)
    {
        strcpy(str,"Transfer OK,");
    }
    textColor(0x03);
    printf("\n--------------------------------------------------------------------------------");
    textColor(0x0E);
    printf("\nRTL8139 IRQ: %y, %s  ", val, str);
    textColor(0x03);

    // reset interrupts by writing 1 to the bits of offset 003Eh bis 003Fh, Interrupt Status Register
    *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3E)) = val;

    strcat(str,"   Receiving Buffer content:\n");
    printf(str);

    int32_t length = network_buffer[3]*0x100 + network_buffer[2]; // Little Endian
    if (length>300)
        length = 300;
    int32_t ethernetType = network_buffer[16]*0x100 + network_buffer[17]; // Big Endian

    // output receiving buffer
    textColor(0x0D);
    printf("Flags: ");
    textColor(0x03);
    for (int8_t i = 0; i < 2; i++)
    {
        printf("%y ",network_buffer[i]);
    }

    // textColor(0x0D); printf("\tLength: "); textColor(0x03);
    // for (i=2;i<4;i++) {printf("%y ",network_buffer[i]);}
    uint32_t packet_length = (network_buffer[3] << 8) | network_buffer[2];

    textColor(0x0D); printf("\tLength: ");
    textColor(0x03);  printf("%d", packet_length);

    textColor(0x0D); printf("\nMAC Receiver: "); textColor(0x03);
    for (int8_t i = 4; i < 10; i++)
    {
        printf("%y ", network_buffer[i]);
    }

    textColor(0x0D); printf("MAC Transmitter: "); textColor(0x03);
    for (int8_t i = 10; i < 16; i++)
    {
        printf("%y ", network_buffer[i]);
    }

    textColor(0x0D);
    printf("\nEthernet: ");
    textColor(0x03);
    if (ethernetType <= 0x05DC)
    {
        printf("type 1, ");
    }
    else
    {
        printf("type 2, ");
    }

    textColor(0x0D);
    if (ethernetType <= 0x05DC)
    {
        printf("Length: ");
    }
    else
    {
        printf("Type: ");
    }
    textColor(0x03);
    for (int8_t i = 16; i < 18; i++)
    {
        printf("%y ", network_buffer[i]);
    }

    printf("\n");
    for (int32_t i = 18; i <= length; i++)
    {
        printf("%y ", network_buffer[i]);
    }
    printf("\n--------------------------------------------------------------------------------\n");
    textColor(0x0F);

    // call to the IP-TCP Stack
    ipTcpStack_recv((void*)(&(network_buffer[4])), packet_length);
}


void install_RTL8139(pciDev_t* device)
{
    for (uint8_t j=0;j<6;++j) // check network card BARs
    {
        device->bar[j].memoryType = device->bar[j].baseAddress & 0x01;

        if (device->bar[j].baseAddress) // check valid BAR
        {
            if (device->bar[j].memoryType == 0)
            {
                BaseAddressRTL8139_MMIO = device->bar[j].baseAddress &= 0xFFFFFFF0;

            }
            if (device->bar[j].memoryType == 1)
            {
                BaseAddressRTL8139_IO = device->bar[j].baseAddress &= 0xFFFC;
            }
        }
    }

    /*
    http://wiki.osdev.org/RTL8139

    Offset (from IO base) 	Size 	Name 
    --------------------------------------
    0x00 	                6 	MAC0-5 
    0x08 	                8 	MAR0-7 
    0x30 	                4 	RBSTART 
    0x37 	                1 	CMD 
    0x3C 	                2 	IMR 
    0x3E 	                2 	ISR

    Turning on the RTL8139:
    Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high. 
    this should essentially "power on" the device.

    Software Reset:
    Sending 0x10 to the Command register (0x37) will send the RTL8139 into a software reset. 
    Once that byte is sent, the RST bit must be checked to make sure that the chip has finished the reset. 
    If the RST bit is 1, then the reset is still in operation.

    Init Receive Buffer:
    Send the chip a memory location to use as its receive buffer start location. 
    One way to do it, would be to define a buffer variable 
    and send that variables memory location to the RBSTART register (0x30).
    */
    
    memset(network_buffer, 0x0, 8192+16); //clear receiving buffer
    kdebug(3, "RTL8139 MMIO: %X\n", BaseAddressRTL8139_MMIO);
    BaseAddressRTL8139_MMIO = (uint32_t) paging_acquire_pcimem(BaseAddressRTL8139_MMIO);
    printf("BaseAddressRTL8139_MMIO mapped to virtual address %X\n", BaseAddressRTL8139_MMIO);

    // "power on" the card
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x52)) = 0x00;

    // carry out reset of network card: set bit 4 at offset 0x37 (1 Byte)
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) = 0x10;

    // wait for the reset of the "reset flag"
    uint32_t k=0;
    while (true)
    {
        sleepMilliSeconds(10);
        if (!(*((volatile uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) & 0x10))
        {
            kdebug(3, "\nwaiting successful (%d).\n", k);
            break;
        }
        k++;
        if (k > 100)
        {
            printf("\nWaiting not successful! Finished by timeout.\n");
            break;
        }
    }
    kdebug(3, "mac address: %y-%y-%y-%y-%y-%y\n",
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+0), *((uint8_t*)(BaseAddressRTL8139_MMIO)+1),
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+2), *((uint8_t*)(BaseAddressRTL8139_MMIO)+3),
                *((uint8_t*)(BaseAddressRTL8139_MMIO)+4), *((uint8_t*)(BaseAddressRTL8139_MMIO)+5));

    // now we set the RE and TE bits from the "Command Register" to Enable Reciving and Transmission
    // activate transmitter and receiver: Set bit 2 (TE) and 3 (RE) in control register 0x37 (1 byte).
    // this has to be done early, otherwise the following instructions will be ignored (??)
    *((uint8_t*)(BaseAddressRTL8139_MMIO + 0x37)) = 0x0C; // 1100b

    // set TCR (transmit configuration register, 0x40, 4 byte) 
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x40)) = 0x03000700;       // TCR
    
    /*
    bit4 AR  - Accept Runt 
    bit3 AB  - Accept Broadcast:      Accept broadcast packets sent to mac FF:FF:FF:FF:FF:FF
    bit2 AM  - Accept Multicast:      Accept multicast packets. 
    bit1 APM - Accept Physical Match: Accept packets send to NIC's MAC address. 
    bit0 AAP - Accept All Packets
    */

    // bit7 is the WRAP bit, 0xF is AB+AM+APM+AAP
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x44)) = 0xF | (1<<7); // RCR  
        
    // set RCR (receive configuration register, 0x44, 4 byte)
    // *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x44)) = 0x0000070A; // RCR // ??? 
    

    // first 65536 bytes are our sending buffer and the last bytes are our receiving buffer
    // set RBSTART to our buffer for the Receive Buffer
    
    // init buffer for receiving. 8 KiB + 16 byte + 1500 byte for receive buffer and transmit buffer needed
    // physical address of the receive buffer has to be written to RBSTART (0x30, 4 byte)
    *((uint32_t*)(BaseAddressRTL8139_MMIO + 0x30)) = paging_get_phys_addr(kernel_pd, (void*)network_buffer);  

    // sets the TOK (interrupt if tx ok) and ROK (interrupt if rx ok) bits high
    // this allows us to get an interrupt if something happens...
    
    // set interrupt mask (0x3C, 2 byte)
    // *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3C)) = 0xFFFF; // all interrupts
    *((uint16_t*)(BaseAddressRTL8139_MMIO + 0x3C)) = 0x5; // only TOK and ROK

    irq_installHandler(32 + device->irq, rtl8139_handler);
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
