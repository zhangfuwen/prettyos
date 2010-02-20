/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "pci.h"
#include "kheap.h" //# just for testing?

pciDev_t pciDev_Array[50];
uint8_t network_buffer[8192+16]; ///TEST for network card


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

// this is the handler for an IRQ interrupt of our Network Card
void rtl8139_handler(struct regs* r) {
	printformat("RTL8139 IRQ\t");
}

 void pciScan()
 {
    uint32_t i;
    settextcolor(15,0);
    uint8_t  bus                = 0; // max. 256
    uint8_t  device             = 0; // max.  32
    uint8_t  func               = 0; // max.   8

    uint32_t pciBar             = 0; // helper variable for memory size
    uint32_t EHCI_data          = 0; // helper variable for EHCI_data




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
                                    printformat("%d:%X I/O ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFFC );
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
						printformat("RTL8139 Network Card, Base Address0: %X\n", pciDev_Array[number].bar[0].baseAddress );

						//check:
						printformat("RTL8139 Network Card, Base Address1: %X\n", pciDev_Array[number].bar[1].baseAddress );
						printformat("RTL8139 Network Card, Base Address2: %X\n", pciDev_Array[number].bar[2].baseAddress );
						printformat("RTL8139 Network Card, Base Address3: %X\n", pciDev_Array[number].bar[3].baseAddress );
						printformat("RTL8139 Network Card, Base Address4: %X\n", pciDev_Array[number].bar[4].baseAddress );
						printformat("RTL8139 Network Card, Base Address5: %X\n", pciDev_Array[number].bar[5].baseAddress );

						// "power on" the card
						*((uint8_t*)( pciDev_Array[number].bar[1].baseAddress + 0x52 )) = 0x00;

						// do an Software reset on that card
						/* Einen Reset der Karte durchführen: Bit 4 im Befehlsregister (0x37, 1 Byte) setzen.
						   Wenn ich hier Portnummern von Registern angebe, ist damit der Offset zum ersten Port der Karte gemeint,
						   der durch die PCI-Funktionen ermittelt werden muss. */
						*((uint8_t*)( pciDev_Array[number].bar[1].baseAddress + 0x37 )) = 0x10;

						// and wait for the reset of the "reset flag"
						while(true)
						{
							if( !( *((uint16_t*)( pciDev_Array[number].bar[1].baseAddress + 0x62 )) & 0x8000 ) ) /// wo kommt das her? Basic Mode Control
							{
								break;
							}
						}

						printformat("waiting successful(%d)!\n", i);

						printformat("mac address: %y-%y-%y-%y-%y-%y\n",
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+0),
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+1),
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+2),
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+3),
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+4),
						            *((uint8_t*)(pciDev_Array[number].bar[1].baseAddress)+5) );

                        // now we set the RE and TE bits from the "Command Register" to Enable Reciving and Transmission
                        /*
                        Aktivieren des Transmitters und des Receivers: Setze Bits 2 und 3 (TE bzw. RE) im Befehlsregister (0x37, 1 Byte).
                        Dies darf angeblich nicht erst später geschehen, da die folgenden Befehle ansonsten ignoriert würden.
                        */
						*((uint8_t*)( pciDev_Array[number].bar[1].baseAddress + 0x37 )) = 0x0C; // 1100b

                        /*
                        TCR (Transmit Configuration Register, 0x40, 4 Bytes) und RCR (Receive Configuration Register, 0x44, 4 Bytes) setzen.
                        An dieser Stelle nicht weiter kommentierter Vorschlag: TCR = 0x03000700, RCR = 0x0000070a
                        */
                        *((uint32_t*)( pciDev_Array[number].bar[1].baseAddress + 0x40 )) = 0x03000700; //TCR
                        *((uint32_t*)( pciDev_Array[number].bar[1].baseAddress + 0x44 )) = 0x0000070a; //RCR
                        //*((uint32_t*)( pciDev_Array[number].bar[1].baseAddress + 0x44 )) = 0xF;        //RCR pci.c in rev. 108 ??
                        /*0xF means AB+AM+APM+AAP*/

						// first 65536 bytes are our sending buffer and the last bytes are our receiving buffer
						// set RBSTART to our buffer for the Receive Buffer
                        /*
                        Puffer für den Empfang (evtl auch zum Senden, das kann aber auch später passieren) initialisieren.
                        Wir brauchen bei den vorgeschlagenen Werten 8K + 16 Bytes für den Empfangspuffer und einen ausreichend großen Sendepuffer.
                        Was ausreichend bedeutet, ist dabei davon abhängig, welche Menge wir auf einmal absenden wollen.
                        Anschließend muss die physische(!) Adresse des Empfangspuffers nach RBSTART (0x30, 4 Bytes) geschrieben werden.
                        */
						*((uint32_t*)( pciDev_Array[number].bar[1].baseAddress + 0x30 )) = (uint32_t)network_buffer /* + 8192+16 */ ;

						// Sets the TOK (interrupt if tx ok) and ROK (interrupt if rx ok) bits high
						// this allows us to get an interrupt if something happens...
						/*
						Interruptmaske setzen (0x3C, 2 Bytes). In diesem Register können die Ereignisse ausgewählt werden,
						die einen IRQ auslösen sollen. Wir nehmen der Einfachkeit halber alle und setzen 0xffff.
						*/
						*((uint16_t*)( pciDev_Array[number].bar[1].baseAddress + 0x3C )) = 0xFFFF;

						printformat("All fine, install irq handler\n");
						irq_install_handler(32 + pciDev_Array[number].irq, rtl8139_handler);
					}

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

