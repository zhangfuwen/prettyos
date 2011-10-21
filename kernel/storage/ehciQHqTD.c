/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb2.h"
#include "ehciQHqTD.h"
#include "util.h"
#include "timer.h"
#include "paging.h"
#include "kheap.h"
#include "video/console.h"


/////////////////////
// Queue Head (QH) //
/////////////////////

void ehci_createQH(ehci_qhd_t* head, uint32_t horizPtr, ehci_qtd_t* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    memset(head, 0, sizeof(ehci_qhd_t));
                                                  // bit 31:5 Horizontal Link Pointer, bit 4:3 reserved,
    head->horizontalPointer     = horizPtr | 0x2; // bit 2:1  type:  00b iTD,   01b QH,   10b siTD,   11b FSTN
                                                  // bit 0    T-Bit: is set to zero
    head->deviceAddress         = device;         // The device address
    head->inactive              = 0;
    head->endpoint              = endpoint;       // endpoint 0 contains Device infos such as name
    head->endpointSpeed         = 2;              // 00b = full speed; 01b = low speed; 10b = high speed
    head->dataToggleControl     = 1;              // get the Data Toggle bit out of the included qTD
    head->H                     = H;              // mark a queue head as being the head of the reclaim list
    head->maxPacketLength       = packetSize;     // 64 byte for a control transfer to a high speed device
    head->controlEndpointFlag   = 0;              // only used if endpoint is a control endpoint and not high speed
    head->nakCountReload        = 0;              // this value is used by EHCI to reload the Nak Counter field. 0=ignores NAK counter.
    head->interruptScheduleMask = 0;              // not used for async schedule
    head->splitCompletionMask   = 0;              // unused if (not low/full speed and in periodic schedule)
    head->hubAddr               = 0;              // unused if high speed (Split transfer)
    head->portNumber            = 0;              // unused if high speed (Split transfer)
    head->mult                  = 1;              // 1-3 transaction per micro-frame, 0 means undefined results
    if (firstQTD == 0)
        head->qtd.next = 0x1;
    else
        head->qtd.next = paging_getPhysAddr(firstQTD);
}

/////////////////////////////////////////////
// Queue Element Transfer Descriptor (qTD) //
/////////////////////////////////////////////

static ehci_qtd_t* allocQTD(uintptr_t next)
{
    ehci_qtd_t* td = (ehci_qtd_t*)malloc(sizeof(ehci_qtd_t), 32, "qTD"); // can be 32 byte alignment
    memset(td,0,sizeof(ehci_qtd_t));

    if (next != 0x1)
        td->next = paging_getPhysAddr((void*)next);
    else
        td->next = 0x1;
    td->nextAlt            = 0x1;  // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80; // This will be filled by the Host Controller. Active bit set
    td->token.errorCounter = 0x0;  // Written by the Host Controller.
    td->token.currPage     = 0x0;  // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;  // We want an interrupt after complete transfer

    return td;
}

static void* allocQTDbuffer(ehci_qtd_t* td)
{
    void* data = malloc(PAGESIZE, PAGESIZE, "qTD-buffer"); // Enough for a full page
    memset(data, 0, PAGESIZE);

    td->buffer0 = paging_getPhysAddr(data);

    return data;
}

ehci_qtd_t* ehci_createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length, void** buffer)
{
    ehci_qtd_t* td = allocQTD(next);

    td->token.pid        = SETUP;      // SETUP = 2
    td->token.bytes      = tokenBytes; // dependent on transfer
    td->token.dataToggle = toggle;     // Should be toggled every list entry

    usb_request_t* request = *buffer = allocQTDbuffer(td);
    request->type    = type;
    request->request = req;
    request->valueHi = hiVal;
    request->valueLo = loVal;
    request->index   = index;
    request->length  = length;

    return td;
}

ehci_qtd_t* ehci_createQTD_IO(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes, void** buffer)
{
    ehci_qtd_t* td = allocQTD(next);

    td->token.pid        = direction;  // OUT = 0, IN = 1
    td->token.bytes      = tokenBytes; // dependent on transfer
    td->token.dataToggle = toggle;     // Should be toggled every list entry

    *buffer = allocQTDbuffer(td);

    return td;
}


////////////////////
// analysis tools //
////////////////////

uint8_t ehci_showStatusbyteQTD(ehci_qtd_t* qTD)
{
    if (qTD->token.status != 0x00)
    {
        printf("\n");
        textColor(ERROR);
        if (qTD->token.status & BIT(6)) { printf("\nHalted - serious error at the device/endpoint"); }
        if (qTD->token.status & BIT(5)) { printf("\nData Buffer Error (overrun or underrun)"); }
        if (qTD->token.status & BIT(4)) { printf("\nBabble - fatal error leads to Halted"); }
        if (qTD->token.status & BIT(3)) { printf("\nTransaction Error - host received no valid response device"); }
        if (qTD->token.status & BIT(2)) { printf("\nMissed Micro-Frame"); }
        textColor(IMPORTANT);
        if (qTD->token.status & BIT(7)) { printf("\nActive - HC transactions enabled"); }
        if (qTD->token.status & BIT(1)) { printf("\nDo Complete Split"); }
      #ifdef _EHCI_DIAGNOSIS_
        if (qTD->token.status & BIT(0)) { printf("\nDo Ping"); }
      #endif
        textColor(TEXT);
    }
    return qTD->token.status;
}


/////////////////////////////////////
// Asynchronous schedule traversal //
/////////////////////////////////////

static void enableAsyncScheduler(ehci_t* e)
{
    e->OpRegs->USBCMD |= CMD_ASYNCH_ENABLE;

    uint32_t timeout=7;
    while (!(e->OpRegs->USBSTS & STS_ASYNC_ENABLED)) // wait until it is really on
    {
        timeout--;
        if (timeout>0)
        {
            sleepMilliSeconds(10);
          #ifdef _EHCI_DIAGNOSIS_
            textColor(LIGHT_MAGENTA);
            printf(">");
            textColor(TEXT);
          #endif
        }
        else
        {
            textColor(ERROR);
            printf("\nTimeout Error - STS_ASYNC_ENABLED not set!");
            textColor(TEXT);
            break;
        }
    }
}

void ehci_initializeAsyncScheduler(ehci_t* e)
{
    e->idleQH = e->tailQH = malloc(sizeof(ehci_qhd_t), 32, "EHCI-QH");
    ehci_createQH(e->idleQH, paging_getPhysAddr(e->idleQH), 0, 1, 0, 0, 0);
    e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->idleQH);
    enableAsyncScheduler(e);
}

void ehci_addToAsyncScheduler(ehci_t* e, usb_transfer_t* transfer, uint8_t velocity)
{
    e->USBasyncIntFlag = false;
    e->USBINTflag = false;

    if(!(e->OpRegs->USBSTS & STS_ASYNC_ENABLED))
        enableAsyncScheduler(e); // Start async scheduler, when it is not running
    e->OpRegs->USBCMD |= CMD_ASYNCH_INT_DOORBELL; // Activate Doorbell: We would like to receive an asynchronous schedule interrupt

    ehci_qhd_t* oldTailQH = e->tailQH; // save old tail QH
    e->tailQH = transfer->data; // new QH is now end of Queue

    e->tailQH->horizontalPointer = paging_getPhysAddr(e->idleQH) | BIT(1); // Create ring. Link new QH with idleQH (always head of Queue)
    oldTailQH->horizontalPointer = paging_getPhysAddr(e->tailQH) | BIT(1); // Insert qh to Queue as element behind old queue head


    uint32_t timeout = 10 * velocity + 25; // Wait up to 250+100*velocity milliseconds for e->USBasyncIntFlag to be set
    while (!e->USBasyncIntFlag && timeout > 0)
    {
        timeout--;
        sleepMilliSeconds(10);
    }

    if (timeout > 0)
    {
      #ifdef _EHCI_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nASYNC_INT successfully set! timeout-counter: %u", timeout);
        textColor(TEXT);
      #endif
    }
    else
    {
        textColor(ERROR);
        printf("\nASYNC_INT not set!");
        textColor(TEXT);
    }


    timeout = 10 * velocity + 25; // Wait up to 100*velocity + 250 milliseconds for all transfers to be finished
    while(timeout > 0)
    {
        bool b = true;
        for(dlelement_t* elem = transfer->transactions->head; b && elem; elem = elem->next)
        {
            ehci_transaction_t* eT = ((usb_transaction_t*)elem->data)->data;
            b = b && !(eT->qTD->token.status & BIT(7));
        }
        if(b)
            break;
        sleepMilliSeconds(10);
        timeout--;
    }
    if(timeout == 0)
    {
        textColor(ERROR);
        printf("\nEHCI: Timeout!");
        textColor(TEXT);
    }

    /* TODO: Why does this optimized loop not work?
    timeout = 10 * velocity + 25; // Wait up to  100*velocity + 250 milliseconds for all transfers to be finished
    dlelement_t* dlE = transfer->transactions->head;
    while(dlE && timeout > 0)
    {
        ehci_transaction_t* eT = ((usb_transaction_t*)dlE->data)->data;
        while(!(eT->qTD->token.status & BIT(7)))
        {
            dlE = dlE->next;
            if(dlE == 0)
                goto done;
            eT = dlE->data;
        }
        sleepMilliSeconds(10);
        timeout--;
    }
    if(timeout == 0)
    {
        textColor(ERROR);
        printf("\nEHCI: Timeout!");
        textColor(TEXT);
    }
    done:*/


    timeout = 10; // Wait for e->USBINTflag to be set
    while (!e->USBINTflag) // set by interrupt
    {
        timeout--;
        if (timeout>0)
        {
            sleepMilliSeconds(10);

          #ifdef _EHCI_DIAGNOSIS_
            textColor(LIGHT_MAGENTA);
            printf("#");
            textColor(TEXT);
          #endif
        }
        else
        {
            textColor(ERROR);
            printf("\nTimeout Error - STS_USBINT not set!");
            textColor(TEXT);
            break;
        }
    }

    e->idleQH->horizontalPointer = paging_getPhysAddr(e->idleQH) | BIT(1); // Restore link of idleQH to idleQH (endless loop)
    e->tailQH = e->idleQH; // qh done. idleQH is end of Queue again (ring structure of asynchronous schedule)
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
