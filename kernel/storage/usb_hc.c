/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb_hc.h"
#include "ehci.h"
#include "uhci.h"
#include "video/console.h"


void install_USB_HostController(pciDev_t* PCIdev)
{
    printf(" - USB ");

    switch (PCIdev->interfaceID)
    {
        case UHCI:
            printf("UHCI");
            break;
        case OHCI:
            printf("OHCI");
            break;
        case EHCI:
            printf("EHCI");
            break;
        case XHCI:
            printf("xHCI");
            break;
        case NO_HCI:
            printf("no HCI");
            break;
        case ANY_HCI:
            printf("any");
            break;
    }

    for (uint8_t i = 0; i < 6; ++i) // check USB BARs
    {
      #ifdef _HCI_DIAGNOSIS_
        switch (PCIdev->bar[i].memoryType)
        {
            case PCI_MMIO:
                printf(" %Xh MMIO", PCIdev->bar[i].baseAddress & 0xFFFFFFF0);
                break;
            case PCI_IO:
                printf(" %xh I/O",  PCIdev->bar[i].baseAddress & 0xFFFC);
                break;
        }
      #endif

        if (PCIdev->bar[i].memoryType != PCI_INVALIDBAR)
        {
          #ifdef _HCI_DIAGNOSIS_
            printf(" sz: %d", PCIdev->bar[i].memorySize);
          #endif

            switch(PCIdev->interfaceID)
            {
            case EHCI:
                ehci_install(PCIdev, PCIdev->bar[i].baseAddress & 0xFFFFFFF0);
                break;
            case UHCI:
                uhci_install(PCIdev, PCIdev->bar[i].baseAddress & 0xFFFFFFF0, PCIdev->bar[i].memorySize);
                break;
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
