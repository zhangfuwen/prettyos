/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "pci.h"
#include "paging.h"
#include "ehci.h"
#include "list.h"
#include "rtl8139.h"
#include "event_list.h"
#include "console.h"

uint8_t network_buffer[8192+16];  // TEST for network card
uint32_t BaseAddressRTL8139_IO;
uint32_t BaseAddressRTL8139_MMIO;

pciDev_t pciDev_Array[PCIARRAYSIZE];

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint16_t content)
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
        | (reg));

    // use offset to find searched content
    uint32_t readVal = inportl(PCI_CONFIGURATION_DATA) >> (8 * offset);

    switch (length)
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

void pci_config_write_byte(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t val)
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC));

    outportb(PCI_CONFIGURATION_DATA + (reg & 0x03), val);
} /// correctness of function pci_config_write_byte checked with bar0 from EHCI - ehenkes, 2010-03-24

void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val)
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC));

    outportl(PCI_CONFIGURATION_DATA, val);
}

void listPCI()
{
    listHead_t* pciDevList = listCreate();
    for (int i=0;i<PCIARRAYSIZE;++i)
    {
        if (pciDev_Array[i].vendorID && (pciDev_Array[i].vendorID != 0xFFFF) && (pciDev_Array[i].vendorID != 0xEE00))   // there is no vendor EE00h
        {
            listAppend(pciDevList, (void*)(pciDev_Array+i));
            settextcolor(2,0);
            printf("%X\t",pciDev_Array+i);
        }
    }
    putch('\n');
    for (int i=0;i<PCIARRAYSIZE;++i)
    {
        void* element = listGetElement(pciDevList,i);
        if (element)
        {
            settextcolor(2,0);
            printf("%X dev: %x vend: %x\t",
                       (pciDev_t*)element,
                       ((pciDev_t*)element)->deviceID,
                       ((pciDev_t*)element)->vendorID);
            settextcolor(15,0);
        }
    }
    puts("\n\n");
}

 void pciScan()
 {
    settextcolor(15,0);

    uint32_t pciBar  = 0; // helper variable for memory size

    // array of devices, PCIARRAYSIZE for first tests
    for (uint32_t i=0;i<PCIARRAYSIZE;++i)
    {
        pciDev_Array[i].number = i;
    }

    int number=0;
    for (uint8_t bus=0;bus<4;++bus) // we scan only four busses of 256 possibles
    {
        for (uint8_t device=0;device<32;++device)
        {
            for (uint8_t func=0;func<8;++func)
            {
                uint16_t vendorID = pci_config_read(bus, device, func, PCI_VENDOR_ID);
                if (vendorID && (vendorID != 0xFFFF))
                {
                    pciDev_Array[number].vendorID     = vendorID;
                    pciDev_Array[number].deviceID     = pci_config_read(bus, device, func, PCI_DEVICE_ID);
                    pciDev_Array[number].classID      = pci_config_read(bus, device, func, PCI_CLASS);
                    pciDev_Array[number].subclassID   = pci_config_read(bus, device, func, PCI_SUBCLASS);
                    pciDev_Array[number].interfaceID  = pci_config_read(bus, device, func, PCI_INTERFACE);
                    pciDev_Array[number].revID        = pci_config_read(bus, device, func, PCI_REVISION);
                    pciDev_Array[number].irq          = pci_config_read(bus, device, func, PCI_IRQLINE);
                    pciDev_Array[number].bar[0].baseAddress = pci_config_read(bus, device, func, PCI_BAR0);
                    pciDev_Array[number].bar[1].baseAddress = pci_config_read(bus, device, func, PCI_BAR1);
                    pciDev_Array[number].bar[2].baseAddress = pci_config_read(bus, device, func, PCI_BAR2);
                    pciDev_Array[number].bar[3].baseAddress = pci_config_read(bus, device, func, PCI_BAR3);
                    pciDev_Array[number].bar[4].baseAddress = pci_config_read(bus, device, func, PCI_BAR4);
                    pciDev_Array[number].bar[5].baseAddress = pci_config_read(bus, device, func, PCI_BAR5);

                    // Valid Device
                    pciDev_Array[number].bus    = bus;
                    pciDev_Array[number].device = device;
                    pciDev_Array[number].func   = func;

                    // output to screen
                    printf("#%d  %d:%d.%d\t dev:%x vend:%x",
                         number,
                         pciDev_Array[number].bus, pciDev_Array[number].device,
                         pciDev_Array[number].func,
                         pciDev_Array[number].deviceID,
                         pciDev_Array[number].vendorID);

                    if (pciDev_Array[number].irq!=255)
                    {
                        printf(" IRQ:%d ", pciDev_Array[number].irq);
                    }
                    else // "255 means "unknown" or "no connection" to the interrupt controller"
                    {
                        printf(" IRQ:-- ");
                    }

                    // test on USB
                    if ((pciDev_Array[number].classID==0x0C) && (pciDev_Array[number].subclassID==0x03))
                    {
                        printf(" USB ");
                        switch(pciDev_Array[number].interfaceID)
                        {
                            case 0x00: printf("UHCI "); break;
                            case 0x10: printf("OHCI "); break;
                            case 0x20: printf("EHCI "); break;
                            case 0x80: printf("no HCI "); break;
                            case 0xFE: printf("any "); break;
                        }

                        for (uint8_t i=0;i<6;++i) // check USB BARs
                        {
                            pciDev_Array[number].bar[i].memoryType = pciDev_Array[number].bar[i].baseAddress & 0x01;

                            if (pciDev_Array[number].bar[i].baseAddress) // check valid BAR
                            {
                                if (pciDev_Array[number].bar[i].memoryType == 0)
                                {
                                    printf("%X MMIO ", pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0);
                                }
                                else if (pciDev_Array[number].bar[i].memoryType == 1)
                                {
                                    printf("%x I/O ",  pciDev_Array[number].bar[i].baseAddress & 0xFFFC);
                                }

                                // TEST Memory Size Begin
                                cli();
                                pci_config_write_dword  (bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF);
                                pciBar = pci_config_read(bus, device, func, PCI_BAR0 + 4*i);
                                pci_config_write_dword  (bus, device, func, PCI_BAR0 + 4*i,
                                                          pciDev_Array[number].bar[i].baseAddress);
                                sti();
                                pciDev_Array[number].bar[i].memorySize = (~pciBar | 0x0F) + 1;
                                printf("sz:%d ", pciDev_Array[number].bar[i].memorySize);
                                // TEST Memory Size End

                                /// EHCI Data
                                if ((pciDev_Array[number].interfaceID==0x20)   // EHCI
                                   && pciDev_Array[number].bar[i].baseAddress) // valid BAR
                                {
                                    uintptr_t bar_phys = pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0;

                                    uintptr_t bar = (uintptr_t) paging_acquire_pcimem(bar_phys);
                                    uintptr_t offset = bar_phys%PAGESIZE;
                                    printf("\nEHCI_MMIO %X mapped to virt addr %X, offset: %x\n", bar_phys, bar, offset);

                                    if (!EHCIflag) // only the first EHCI is used
                                    {
                                        ODA.pciEHCInumber = number; /// TODO: implement for more than one EHCI
                                        EHCIflag = true; // only the first EHCI is used
                                        if(ODA.pciEHCInumber)
                                        {
                                            addEvent(&EHCI_INIT);
                                        }
                                        analyzeEHCI(bar,offset); // get data (capregs, opregs)
                                    }
                                }
                            } /// if USB
                        } // for
                    } // if

                    printf("\n");


                    /// RTL 8139 network card
                    if ((pciDev_Array[number].deviceID == 0x8139))
                    {
                        for (uint8_t j=0;j<6;++j) // check network card BARs
                        {
                            pciDev_Array[number].bar[j].memoryType = pciDev_Array[number].bar[j].baseAddress & 0x01;

                            if (pciDev_Array[number].bar[j].baseAddress) // check valid BAR
                            {
                                if (pciDev_Array[number].bar[j].memoryType == 0)
                                {
                                    BaseAddressRTL8139_MMIO = pciDev_Array[number].bar[j].baseAddress &= 0xFFFFFFF0;

                                }
                                if (pciDev_Array[number].bar[j].memoryType == 1)
                                {
                                    BaseAddressRTL8139_IO = pciDev_Array[number].bar[j].baseAddress &= 0xFFFC;
                                }
                            }
                        }
                        install_RTL8139(number);
                    }
                    ++number;
                } // if pciVendor

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if (!(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80))
                {
                    break; // --> not multifunctional, only function 0 used
                }
            } // for function
        } // for device
    } // for bus
    printf("\n");
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
