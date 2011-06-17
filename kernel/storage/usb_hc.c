/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb_hc.h"
#include "util.h"
#include "ehci.h"
#include "video/console.h"

void install_USB_HostController(pciDev_t* PCIdev)
{
    printf(" - USB ");
    switch(PCIdev->interfaceID)
    {
        case 0x00: printf("UHCI ");   break;
        case 0x10: printf("OHCI ");   break;
        case 0x20: printf("EHCI ");   break;
        case 0x30: printf("xHCI ");   break;
        case 0x80: printf("no HCI "); break;
        case 0xFE: printf("any ");    break;
    }

    uint8_t bus  = PCIdev->bus;
    uint8_t dev  = PCIdev->device;
    uint8_t func = PCIdev->func;

    for (uint8_t i = 0; i < 6; ++i) // check USB BARs
    {
        PCIdev->bar[i].memoryType = PCIdev->bar[i].baseAddress & 0x01;

        if (PCIdev->bar[i].baseAddress) // check valid BAR
        {
            if (PCIdev->bar[i].memoryType == 0)
            {
                printf("%Xh MMIO ", PCIdev->bar[i].baseAddress & 0xFFFFFFF0);
            }
            else if (PCIdev->bar[i].memoryType == 1)
            {
                printf("%xh I/O ",  PCIdev->bar[i].baseAddress & 0xFFFC);
            }

            // check Memory Size
            cli();
            pci_config_write_dword(bus, dev, func, PCI_BAR0 + 4*i, 0xFFFFFFFF);
            uintptr_t pciBar = pci_config_read(bus, dev, func, PCI_BAR0 + 4*i);
            pci_config_write_dword(bus, dev, func, PCI_BAR0 + 4*i, PCIdev->bar[i].baseAddress);
            sti();
            PCIdev->bar[i].memorySize = (~pciBar | 0x0F) + 1;
            printf("sz: %d ", PCIdev->bar[i].memorySize);

            // EHCI Data
            if (PCIdev->interfaceID == 0x20   // EHCI
               && PCIdev->bar[i].baseAddress) // valid BAR
            {
                ehci_install(PCIdev, i);
            }
        }
    }
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
