/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb_hc.h"
#include "uhci.h"
#include "ohci.h"
#include "ehci.h"
#include "video/console.h"
#include "kheap.h"
#include "util.h"
#include "usb2.h"


void usb_hc_install(pciDev_t* PCIdev)
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

    for (uint8_t i = 0; i < 6; ++i) // check BARs
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
                  #ifdef _EHCI_ENABLE_
                    ehci_install(PCIdev, PCIdev->bar[i].baseAddress & 0xFFFFFFF0);
                  #endif
                    break;
                case OHCI:
                  #ifdef _OHCI_ENABLE_
                    ohci_install(PCIdev, PCIdev->bar[i].baseAddress & 0xFFFFFFF0, PCIdev->bar[i].memorySize);
                  #endif
                    break;
                case UHCI:
                  #ifdef _UHCI_ENABLE_
                    uhci_install(PCIdev, PCIdev->bar[i].baseAddress & 0xFFFFFFF0, PCIdev->bar[i].memorySize);
                  #endif
                    break;
            }
        }
    }
}


void usb_setupTransfer(port_t* usbPort, usb_transfer_t* transfer, usb_tranferType_t type, uint32_t endpoint, size_t maxLength)
{
    transfer->HC           = usbPort;
    transfer->transactions = list_create();
    transfer->endpoint     = endpoint;
    transfer->type         = type;
    transfer->packetSize   = min(maxLength, ((usb2_Device_t*)usbPort->insertedDisk->data)->mps[endpoint]);
    transfer->success      = false;

    if (transfer->HC->type == &USB_EHCI)
    {
        ehci_setupTransfer(transfer);
    }
    else if (transfer->HC->type == &USB_OHCI)
    {
        ohci_setupTransfer(transfer);
    }
    else if (transfer->HC->type == &USB_UHCI)
    {
        uhci_setupTransfer(transfer);
    }
    else
    {
        printf("\nUnknown port type.");
    }
}

void usb_setupTransaction(usb_transfer_t* transfer, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length)
{
    usb_transaction_t* transaction = malloc(sizeof(usb_transaction_t), 0, "usb_transaction_t");
    transaction->type = USB_TT_SETUP;

    if (transfer->HC->type == &USB_EHCI)
    {
        ehci_setupTransaction(transfer, transaction, toggle, tokenBytes, type, req, hiVal, loVal, index, length);
    }
    else if (transfer->HC->type == &USB_OHCI)
    {
        ohci_setupTransaction(transfer, transaction, toggle, tokenBytes, type, req, hiVal, loVal, index, length);
    }
    else if (transfer->HC->type == &USB_UHCI)
    {
        uhci_setupTransaction(transfer, transaction, toggle, tokenBytes, type, req, hiVal, loVal, index, length);
    }
    else
    {
        printf("\nUnknown port type.");
    }
    list_append(transfer->transactions, transaction);
}

void usb_inTransaction(usb_transfer_t* transfer, bool toggle, void* buffer, size_t length)
{
    size_t clampedLength = min(transfer->packetSize, length);

    usb_transaction_t* transaction = malloc(sizeof(usb_transaction_t), 0, "usb_transaction_t");
    transaction->type = USB_TT_IN;

    if (transfer->HC->type == &USB_EHCI)
    {
        ehci_inTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else if (transfer->HC->type == &USB_OHCI)
    {
        ohci_inTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else if (transfer->HC->type == &USB_UHCI)
    {
        uhci_inTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else
    {
        printf("\nUnknown port type.");
    }

    list_append(transfer->transactions, transaction);

    length -= clampedLength;
    if(length > 0)
    {
        usb_inTransaction(transfer, !toggle, buffer+clampedLength, length);
    }
}

void usb_outTransaction(usb_transfer_t* transfer, bool toggle, void* buffer, size_t length)
{
    size_t clampedLength = min(transfer->packetSize, length);

    usb_transaction_t* transaction = malloc(sizeof(usb_transaction_t), 0, "usb_transaction_t");
    transaction->type = USB_TT_OUT;

    if (transfer->HC->type == &USB_EHCI)
    {
        ehci_outTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else if (transfer->HC->type == &USB_OHCI)
    {
        ohci_outTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else if (transfer->HC->type == &USB_UHCI)
    {
        uhci_outTransaction(transfer, transaction, toggle, buffer, clampedLength);
    }
    else
    {
        printf("\nUnknown port type.");
    }

    list_append(transfer->transactions, transaction);

    length -= clampedLength;
    if(length > 0)
    {
        usb_outTransaction(transfer, !toggle, buffer+clampedLength, length);
    }
}

void usb_issueTransfer(usb_transfer_t* transfer)
{
    if (transfer->HC->type == &USB_EHCI)
    {
        ehci_issueTransfer(transfer);
    }
    else if (transfer->HC->type == &USB_OHCI)
    {
        ohci_issueTransfer(transfer);
    }
    else if (transfer->HC->type == &USB_UHCI)
    {
        uhci_issueTransfer(transfer);
    }
    else
    {
        printf("\nUnknown port type.");
    }

    for(dlelement_t* e = transfer->transactions->head; e != 0; e = e->next)
        free(e->data);
    list_free(transfer->transactions);
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
