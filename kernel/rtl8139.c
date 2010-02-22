/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"
#include "paging.h"

extern pciDev_t pciDev_Array[50];

extern uint8_t network_buffer[8192+16];  ///TEST for network card
extern uint32_t BaseAddressRTL8139_IO;   ///TEST for network card
extern uint32_t BaseAddressRTL8139_MMIO; ///TEST for network card
extern uint32_t BaseAddressRTL8139;      ///TEST for network card

/// this is the handler for an IRQ interrupt of our Network Card
// only here for tests --> TODO: own module
void rtl8139_handler(struct regs* r)
{
	/// TODO: ring buffer, we get always the first received data!

	// read bytes 003Eh bis 003Fh, Interrupt Status Register
    uint16_t val = *((uint16_t*)( BaseAddressRTL8139_MMIO + 0x3E ));

    char str[80];
    strcpy(str,"");

    if((val & 0x1) == 0x1)
    {
        strcpy(str,"Receive OK,");
    }
    if((val & 0x4) == 0x4)
    {
        strcpy(str,"Transfer OK,");
    }
    settextcolor(3,0);
    printformat("\n--------------------------------------------------------------------------------");
    settextcolor(14,0);
    printformat("\nRTL8139 IRQ: %y, %s  ", val,str);
    settextcolor(3,0);

    // reset interrupts bei writing 1 to the bits of offset 003Eh bis 003Fh, Interrupt Status Register
	*((uint16_t*)( BaseAddressRTL8139_MMIO + 0x3E )) = val;

	strcat(str,"   Receiving Buffer content:\n");
	printformat(str);

    int32_t length, ethernetType, i;
    length = network_buffer[3]*0x100 + network_buffer[2]; // Little Endian
    if (length>300) length = 300;
    ethernetType = network_buffer[16]*0x100 + network_buffer[17]; // Big Endian

    // output receiving buffer
    settextcolor(13,0);
    printformat("Flags: ");
    settextcolor(3,0);
    for(i=0;i<2;i++) {printformat("%y ",network_buffer[i]);}

    settextcolor(13,0); printformat("\tLength: "); settextcolor(3,0);
    for(i=2;i<4;i++) {printformat("%y ",network_buffer[i]);}

    settextcolor(13,0); printformat("\nMAC Receiver: "); settextcolor(3,0);
    for(i=4;i<10;i++) {printformat("%y ",network_buffer[i]);}

    settextcolor(13,0); printformat("MAC Transmitter: "); settextcolor(3,0);
    for(i=10;i<16;i++) {printformat("%y ",network_buffer[i]);}

    settextcolor(13,0);
    printformat("\nEthernet: ");
    settextcolor(3,0);
    if(ethernetType<=0x05DC){  printformat("type 1, "); }
    else                    {  printformat("type 2, "); }

    settextcolor(13,0);
    if(ethernetType<=0x05DC){  printformat("Length: "); }
    else                    {  printformat("Type: ");   }
    settextcolor(3,0);
    for(i=16;i<18;i++) {printformat("%y ",network_buffer[i]);}

    printformat("\n");
    for(i=18;i<=length;i++) {printformat("%y ",network_buffer[i]);}
    printformat("\n--------------------------------------------------------------------------------\n");

    settextcolor(15,0);
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

