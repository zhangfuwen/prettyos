/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "sys_speaker.h"
#include "usb2.h"

void testTransfer(uint32_t device)
{
    settextcolor(9,0);
    printf("\n>>> >>> function: testTransfer\n");
    settextcolor(15,0);

    settextcolor(3,0);
    printf("Test transfer with device address: %d\n", device);
    settextcolor(15,0);

    void* virtualAsyncList = malloc(sizeof(struct ehci_qhd), PAGESIZE);
    uint32_t phsysicalAddr = paging_get_phys_addr(kernel_pd, virtualAsyncList);
    pOpRegs->ASYNCLISTADDR = phsysicalAddr;

    // Create QTDs (in reversed order)
    void* next                = createQTD(0x1, 0x0, 1, 0);    // Handshake is the opposite direction of Data
    next           = InQTD    = createQTD((uint32_t)next, 0x1, 1, 18); // IN DATA1, 18 byte
    void* firstQTD = SetupQTD = createQTD((uint32_t)next, 0x2, 0,  8); // SETUP DATA0, 8 byte

    // Create QH 1
    void* QH1 = malloc(sizeof(struct ehci_qhd), PAGESIZE);
    createQH(QH1, paging_get_phys_addr(kernel_pd, virtualAsyncList), firstQTD, 0, device);

    // Create QH 2
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, QH1), NULL, 1, device);

    // Enable Async...
    printf("\nEnabling Async Schedule\n");
    pOpRegs->USBCMD = pOpRegs->USBCMD | CMD_ASYNCH_ENABLE;

    printf("\n");
    showPacket(InQTDpage0,18);
    showDeviceDesriptor((struct usb2_deviceDescriptor*)InQTDpage0);
}

void showDeviceDesriptor(struct usb2_deviceDescriptor* d)
{
    settextcolor(9,0);
    printf("\n>>> >>>function: showDeviceDesriptor\n");
    settextcolor(15,0);

    if (d->length)
    {
       settextcolor(10,0);
       printf("\nlength:            %d\n",  d->length);
       printf("descriptor type:   %d\n",    d->descriptorType);
       printf("USB specification: %d.%d\n", d->bcdUSB>>8, d->bcdUSB&0xFF);     // e.g. 0x0210 means 2.10
       printf("USB class:         %x\n",    d->deviceClass);
       printf("USB subclass:      %x\n",    d->deviceSubclass);
       printf("USB protocol       %x\n",    d->deviceProtocol);
       printf("max packet size:   %d\n",    d->maxPacketSize);             // MPS0, must be 8,16,32,64
       printf("vendor:            %x\n",    d->idVendor);
       printf("product:           %x\n",    d->idProduct);
       printf("release number:    %d.%d\n", d->bcdDevice>>8, d->bcdDevice&0xFF);  // release of the device
       printf("manufacturer:      %x\n",    d->manufacturer);
       printf("product:           %x\n",    d->product);
       printf("serial number:     %x\n",    d->serialNumber);
       printf("number of config.: %d\n",    d->numConfigurations); // number of possible configurations
       settextcolor(15,0);
    }
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
