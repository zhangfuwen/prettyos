/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb_hc.h"
#include "util.h"

// pci devices list
extern pciDev_t pciDev_Array[PCIARRAYSIZE];

void install_USB_HostController(uint32_t num)
{
    uint8_t bus  = pciDev_Array[num].bus;
    uint8_t dev  = pciDev_Array[num].device;
    uint8_t func = pciDev_Array[num].func;

    printf(" USB ");
    switch(pciDev_Array[num].interfaceID)
    {
        case 0x00: printf("UHCI "); break;
        case 0x10: printf("OHCI "); break;
        case 0x20: printf("EHCI "); break;
        case 0x80: printf("no HCI "); break;
        case 0xFE: printf("any "); break;
    }

    for (uint8_t i=0;i<6;++i) // check USB BARs
    {
        pciDev_Array[num].bar[i].memoryType = pciDev_Array[num].bar[i].baseAddress & 0x01;

        if (pciDev_Array[num].bar[i].baseAddress) // check valid BAR
        {
            if (pciDev_Array[num].bar[i].memoryType == 0)
            {
                printf("%X MMIO ", pciDev_Array[num].bar[i].baseAddress & 0xFFFFFFF0);
            }
            else if (pciDev_Array[num].bar[i].memoryType == 1)
            {
                printf("%x I/O ",  pciDev_Array[num].bar[i].baseAddress & 0xFFFC);
            }

            // check Memory Size
            cli();
            pci_config_write_dword  (bus, dev, func, PCI_BAR0 + 4*i, 0xFFFFFFFF);
            uintptr_t pciBar = pci_config_read(bus, dev, func, PCI_BAR0 + 4*i);
            pci_config_write_dword  (bus, dev, func, PCI_BAR0 + 4*i, pciDev_Array[num].bar[i].baseAddress);
            sti();
            pciDev_Array[num].bar[i].memorySize = (~pciBar | 0x0F) + 1;
            printf("sz:%d ", pciDev_Array[num].bar[i].memorySize);

            /// EHCI Data
            if (pciDev_Array[num].interfaceID == 0x20   // EHCI
               && pciDev_Array[num].bar[i].baseAddress) // valid BAR
            {
                ehci_install(num,i);
            }
        } /// if USB
    } // for
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
