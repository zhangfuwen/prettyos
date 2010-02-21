/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"

pciDev_t pciDev_Array[50];

uint8_t network_buffer[8192+16];  ///TEST for network card
uint32_t BaseAddressRTL8139_IO;   ///TEST for network card
uint32_t BaseAddressRTL8139_MMIO; ///TEST for network card
uint32_t BaseAddressRTL8139;      ///TEST for network card

/// this is the handler for an IRQ interrupt of our Network Card
// only here for tests --> TODO: own module
void rtl8139_handler(struct regs* r)
{
	// read bytes 003Eh bis 003Fh, Interrupt Status Register
    uint32_t val = inportw(BaseAddressRTL8139_IO + 0x3E);
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
	outportw( BaseAddressRTL8139_IO + 0x3E, val );

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

uint32_t pci_config_read( uint8_t bus, uint8_t device, uint8_t func, uint16_t content )
{
    // example: PCI_VENDOR_ID 0x0200 ==> length: 0x02 reg: 0x00 offset: 0x00
    uint8_t length  = content >> 8;
    uint8_t reg_off = content & 0x00FF;
    uint8_t reg     = reg_off & 0xFC;     // bit mask: 11111100b
    uint8_t offset  = reg_off % 0x04;     // remainder of modulo operation provides offset

    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus    << 16)
        | (device << 11)
        | (func   <<  8)
        | (reg         ));

    // use offset to find searched content
    uint32_t readVal = inportl(PCI_CONFIGURATION_DATA) >> (8 * offset);

    switch(length)
    {
        case 1:
            readVal &= 0x000000FF;
        break;
        case 2:
            readVal &= 0x0000FFFF;
        break;
        case 4:
            readVal &= 0xFFFFFFFF;
        break;
    }
    return readVal;
}

void pci_config_write_dword( uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val )
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC   ));

    outportl(PCI_CONFIGURATION_DATA, val);
}



 void pciScan()
 {
    uint32_t i,j,k;
    settextcolor(15,0);
    uint8_t  bus                = 0; // max. 256
    uint8_t  device             = 0; // max.  32
    uint8_t  func               = 0; // max.   8

    uint32_t pciBar             = 0; // helper variable for memory size
    uint32_t EHCI_data          = 0; // helper variable for EHCI_data

    //clear receiving buffer
    memset((void*) network_buffer, 0x0, 8192+16);

    // array of devices, PCIARRAYSIZE for first tests
    for(i=0;i<PCIARRAYSIZE;++i)
    {
        pciDev_Array[i].number = i;
    }

    int number=0;
    for(bus=0;bus<8;++bus)
    {
        for(device=0;device<32;++device)
        {
            for(func=0;func<8;++func)
            {
                uint16_t vendorID = pci_config_read( bus, device, func, PCI_VENDOR_ID);
                if( vendorID && (vendorID != 0xFFFF) )
                {
                    pciDev_Array[number].vendorID     = vendorID;
                    pciDev_Array[number].deviceID     = pci_config_read( bus, device, func, PCI_DEVICE_ID  );
                    pciDev_Array[number].classID      = pci_config_read( bus, device, func, PCI_CLASS      );
                    pciDev_Array[number].subclassID   = pci_config_read( bus, device, func, PCI_SUBCLASS   );
                    pciDev_Array[number].interfaceID  = pci_config_read( bus, device, func, PCI_INTERFACE  );
                    pciDev_Array[number].revID        = pci_config_read( bus, device, func, PCI_REVISION   );
                    pciDev_Array[number].irq          = pci_config_read( bus, device, func, PCI_IRQLINE    );
                    pciDev_Array[number].bar[0].baseAddress = pci_config_read( bus, device, func, PCI_BAR0 );
                    pciDev_Array[number].bar[1].baseAddress = pci_config_read( bus, device, func, PCI_BAR1 );
                    pciDev_Array[number].bar[2].baseAddress = pci_config_read( bus, device, func, PCI_BAR2 );
                    pciDev_Array[number].bar[3].baseAddress = pci_config_read( bus, device, func, PCI_BAR3 );
                    pciDev_Array[number].bar[4].baseAddress = pci_config_read( bus, device, func, PCI_BAR4 );
                    pciDev_Array[number].bar[5].baseAddress = pci_config_read( bus, device, func, PCI_BAR5 );

                    // Valid Device
                    pciDev_Array[number].bus    = bus;
                    pciDev_Array[number].device = device;
                    pciDev_Array[number].func   = func;

                    // output to screen
                    printformat("#%d %d:%d.%d\t dev:%x vend:%x",
                         number,
                         pciDev_Array[number].bus, pciDev_Array[number].device, pciDev_Array[number].func,
                         pciDev_Array[number].deviceID, pciDev_Array[number].vendorID );

                    if(pciDev_Array[number].irq!=255)
                    {
                        printformat(" IRQ:%d ", pciDev_Array[number].irq );
                    }
                    else // "255 means "unknown" or "no connection" to the interrupt controller"
                    {
                        printformat(" IRQ:-- ");
                    }

                    // test on USB
                    if( (pciDev_Array[number].classID==0x0C) && (pciDev_Array[number].subclassID==0x03) )
                    {
                        printformat(" USB ");
                        if( pciDev_Array[number].interfaceID==0x00 ) { printformat("UHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x10 ) { printformat("OHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x20 ) { printformat("EHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x80 ) { printformat("no HCI "); }
                        if( pciDev_Array[number].interfaceID==0xFE ) { printformat("any ");    }

                        for(i=0;i<6;++i) // check USB BARs
                        {
                            pciDev_Array[number].bar[i].memoryType = pciDev_Array[number].bar[i].baseAddress & 0x01;

                            if(pciDev_Array[number].bar[i].baseAddress) // check valid BAR
                            {
                                if(pciDev_Array[number].bar[i].memoryType == 0)
                                {
                                    printformat("%d:%X MEM ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0 );
                                }
                                if(pciDev_Array[number].bar[i].memoryType == 1)
                                {
                                    printformat("%d:%x I/O ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFC );
                                }

                                /// TEST Memory Size Begin
                                cli();
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF );
                                pciBar = pci_config_read( bus, device, func, PCI_BAR0 + 4*i             );
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i,
                                                          pciDev_Array[number].bar[i].baseAddress       );
                                sti();
                                pciDev_Array[number].bar[i].memorySize = (~pciBar | 0x0F) + 1;
                                printformat("sz:%d ", pciDev_Array[number].bar[i].memorySize );
                                /// TEST Memory Size End

                                /// TEST EHCI Data Begin
                                if(  (pciDev_Array[number].interfaceID==0x20)   // EHCI
                                   && pciDev_Array[number].bar[i].baseAddress ) // valid BAR
                                {
                                    /*
                                    Offset Size Mnemonic    Power Well   Register Name
                                    00h     1   CAPLENGTH      Core      Capability Register Length
                                    01h     1   Reserved       Core      N/A
                                    02h     2   HCIVERSION     Core      Interface Version Number
                                    04h     4   HCSPARAMS      Core      Structural Parameters
                                    08h     4   HCCPARAMS      Core      Capability Parameters
                                    0Ch     8   HCSP-PORTROUTE Core      Companion Port Route Description
                                    */

                                    uint32_t bar = pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0;

                                    EHCI_data = *((volatile uint8_t* )(bar + 0x00));
                                    printformat("\nBAR%d CAPLENGTH:  %x \t\t",i, EHCI_data);

                                    EHCI_data = *((volatile uint16_t*)(bar + 0x02));
                                    printformat(  "BAR%d HCIVERSION: %x \n",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)(bar + 0x04));
                                    printformat(  "BAR%d HCSPARAMS:  %X \t",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)(bar + 0x08));
                                    printformat(  "BAR%d HCCPARAMS:  %X \n",i, EHCI_data);
                                }
                                /// TEST EHCI Data End
                            } // if
                        } // for
                    } // if

					printformat("\n");


					/// test on the RTL8139 network card and test for some functions to work ;)
					// informations from the RTL8139 specification,
					// and the wikis http://wiki.osdev.org/RTL8139, http://lowlevel.brainsware.org/wiki/index.php/RTL8139

					if(	(pciDev_Array[number].deviceID == 0x8139) && (pciDev_Array[number].vendorID == 0x10EC) )
					{
						for(j=0;j<6;++j) // check network card BARs
                        {
                            pciDev_Array[number].bar[j].memoryType = pciDev_Array[number].bar[j].baseAddress & 0x01;

                            if(pciDev_Array[number].bar[j].baseAddress) // check valid BAR
                            {
                                if(pciDev_Array[number].bar[j].memoryType == 0)
                                {
                                    BaseAddressRTL8139_MMIO = pciDev_Array[number].bar[j].baseAddress &= 0xFFFFFFF0;

                                }
                                if(pciDev_Array[number].bar[j].memoryType == 1)
                                {
                                    BaseAddressRTL8139_IO = pciDev_Array[number].bar[j].baseAddress &= 0xFFFC;
                                }
                            }
                        }

                        /// important:
                        /// access to BaseAddressRTL8139_IO + offset: outxxx(BaseAddressRTL8139_IO + offset, data)
                        ///

                        printformat("\nUsed MMIO Base for RTL8139: %X",BaseAddressRTL8139_MMIO);

						// "power on" the card
						*((uint8_t*)( BaseAddressRTL8139_MMIO + 0x52 )) = 0x00;

						// do an Software reset on that card
						/* Einen Reset der Karte durchführen: Bit 4 im Befehlsregister (0x37, 1 Byte) setzen.
						   Wenn ich hier Portnummern von Registern angebe, ist damit der Offset zum ersten Port der Karte gemeint,
						   der durch die PCI-Funktionen ermittelt werden muss. */
						*((uint8_t*)( BaseAddressRTL8139_MMIO + 0x37 )) = 0x10;

						// and wait for the reset of the "reset flag"
						k=0;
						while(true)
						{
							if( !( *((uint16_t*)( BaseAddressRTL8139_MMIO + 0x62 )) & 0x8000 ) ) /// wo kommt das her? Basic Mode Control
							{
								break;
							}
							k++;
						}

						printformat("\nwaiting successful(%d)!\n", k);

						printformat("mac address: %y-%y-%y-%y-%y-%y\n",
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+0),
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+1),
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+2),
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+3),
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+4),
						            *((uint8_t*)(BaseAddressRTL8139_MMIO)+5) );

                        // now we set the RE and TE bits from the "Command Register" to Enable Reciving and Transmission
                        /*
                        Aktivieren des Transmitters und des Receivers: Setze Bits 2 und 3 (TE bzw. RE) im Befehlsregister (0x37, 1 Byte).
                        Dies darf angeblich nicht erst später geschehen, da die folgenden Befehle ansonsten ignoriert würden.
                        */
						*((uint8_t*)( BaseAddressRTL8139_MMIO + 0x37 )) = 0x0C; // 1100b

                        /*
                        TCR (Transmit Configuration Register, 0x40, 4 Bytes) und RCR (Receive Configuration Register, 0x44, 4 Bytes) setzen.
                        An dieser Stelle nicht weiter kommentierter Vorschlag: TCR = 0x03000700, RCR = 0x0000070a
                        */
                        *((uint32_t*)( BaseAddressRTL8139_MMIO + 0x40 )) = 0x03000700; //TCR
                        *((uint32_t*)( BaseAddressRTL8139_MMIO + 0x44 )) = 0x0000070a; //RCR
                        //*((uint32_t*)( BaseAddressRTL8139_MMIO + 0x44 )) = 0xF;        //RCR pci.c in rev. 108 ??
                        /*0xF means AB+AM+APM+AAP*/

						// first 65536 bytes are our sending buffer and the last bytes are our receiving buffer
						// set RBSTART to our buffer for the Receive Buffer
                        /*
                        Puffer für den Empfang (evtl auch zum Senden, das kann aber auch später passieren) initialisieren.
                        Wir brauchen bei den vorgeschlagenen Werten 8K + 16 Bytes für den Empfangspuffer und einen ausreichend großen Sendepuffer.
                        Was ausreichend bedeutet, ist dabei davon abhängig, welche Menge wir auf einmal absenden wollen.
                        Anschließend muss die physische(!) Adresse des Empfangspuffers nach RBSTART (0x30, 4 Bytes) geschrieben werden.
                        */
						*((uint32_t*)( BaseAddressRTL8139_MMIO + 0x30 )) = (uint32_t)network_buffer /* + 8192+16 */ ;

						// Sets the TOK (interrupt if tx ok) and ROK (interrupt if rx ok) bits high
						// this allows us to get an interrupt if something happens...
						/*
						Interruptmaske setzen (0x3C, 2 Bytes). In diesem Register können die Ereignisse ausgewählt werden,
						die einen IRQ auslösen sollen. Wir nehmen der Einfachkeit halber alle und setzen 0xffff.
						*/
						*((uint16_t*)( BaseAddressRTL8139_MMIO + 0x3C )) = 0xFF; // all interrupts
						//*((uint16_t*)( BaseAddressRTL8139_MMIO + 0x3C )) = 0x5; // only TOK and ROK

						printformat("All fine, install irq handler.\n");
						irq_install_handler(32 + pciDev_Array[number].irq, rtl8139_handler);
					} // if 8139 ...

                    ++number;
                } // if pciVendor

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if( !(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80) )
                {
                    break; // --> not multifunctional, only function 0 used
                }
            } // for function
        } // for device
    } // for bus
    printformat("\n");

	// for(;;){} //#
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

