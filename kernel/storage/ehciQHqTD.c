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

void createQH(ehci_qhd_t* head, uint32_t horizPtr, ehci_qtd_t* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    memset(head, 0, sizeof(ehci_qhd_t));
                                                    // bit 31:5 Horizontal Link Pointer, bit 4:3 reserved,
    head->horizontalPointer     =   horizPtr | 0x2; // bit 2:1  type:  00b iTD,   01b QH,   10b siTD,   11b FSTN
                                                    // bit 0    T-Bit: is set to zero
    head->deviceAddress         =   device;         // The device address
    head->inactive              =   0;
    head->endpoint              =   endpoint;       // endpoint 0 contains Device infos such as name
    head->endpointSpeed         =   2;              // 00b = full speed; 01b = low speed; 10b = high speed
    head->dataToggleControl     =   1;              // get the Data Toggle bit out of the included qTD
    head->H                     =   H;              // mark a queue head as being the head of the reclaim list
    head->maxPacketLength       =   packetSize;     // 64 byte for a control transfer to a high speed device
    head->controlEndpointFlag   =   0;              // only used if endpoint is a control endpoint and not high speed
    head->nakCountReload        =   0;              // this value is used by EHCI to reload the Nak Counter field. 0=ignores NAK counter.
    head->interruptScheduleMask =   0;              // not used for async schedule
    head->splitCompletionMask   =   0;              // unused if (not low/full speed and in periodic schedule)
    head->hubAddr               =   0;              // unused if high speed (Split transfer)
    head->portNumber            =   0;              // unused if high speed (Split transfer)
    head->mult                  =   1;              // 1-3 transaction per micro-frame, 0 means undefined results
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

    return td;
}

static void* allocQTDbuffer(ehci_qtd_t* td)
{
    void* data = malloc(PAGESIZE, PAGESIZE, "qTD-buffer"); // Enough for a full page
    memset(data, 0, PAGESIZE);

    td->buffer0 = paging_getPhysAddr(data);

    return data;
}

ehci_qtd_t* createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length, void** buffer)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;        // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = SETUP;      // SETUP = 2
    td->token.errorCounter = 0x0;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    usb_request_t* request = *buffer = allocQTDbuffer(td);
    request->type    = type;
    request->request = req;
    request->valueHi = hiVal;
    request->valueLo = loVal;
    request->index   = index;
    request->length  = length;

    return td;
}

ehci_qtd_t* createQTD_IO(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes, void** buffer)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;        // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = direction;  // OUT = 0, IN = 1
    td->token.errorCounter = 0x0;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    *buffer = allocQTDbuffer(td);

    return td;
}


////////////////////
// analysis tools //
////////////////////

uint8_t showStatusbyteQTD(ehci_qtd_t* qTD)
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
        if (qTD->token.status & BIT(0)) { printf("\nDo Ping"); }
        textColor(TEXT);
    }
    return qTD->token.status;
}

/////////////////////////////////////
// Asynchronous schedule traversal //
/////////////////////////////////////

/*
ehci spec 1.0, chapter 4.4 Schedule Traversal Rules:

..., the host controller executes from the asynchronous schedule until the end of the micro-frame.
When the host controller determines that it is time to execute from the asynchronous list,
it uses the operational register ASYNCLISTADDR to access the asynchronous schedule, see Figure 4-4.
The ASYNCLISTADDR register contains a physical memory pointer to the next queue head.
When the host controller makes a transition to executing the asynchronous schedule,
it begins by reading the queue head referenced by the ASYNCLISTADDR register.
Software must set queue head horizontal pointer T-bits to a zero for queue heads in the asynchronous schedule.

ehci spec 1.0, chapter 4.8 Asynchronous Schedule:

The Asynchronous schedule traversal is enabled or disabled via the Asynchronous Schedule Enable bit in the USBCMD register.
If the Asynchronous Schedule Enable bit is set to a zero, then the host controller simply does not try to access
the asynchronous schedule via the ASYNCLISTADDR register. Likewise, when the Asynchronous Schedule Enable bit is a one,
then the host controller does use the ASYNCLISTADDR register to traverse the asynchronous schedule.

Modifications to the Asynchronous Schedule Enable bit are not necessarily immediate.
Rather the new value of the bit will only be taken into consideration the next time the host controller needs to use
the value of the ASYNCLISTADDR register to get the next queue head.

The Asynchronous Schedule Status bit in the USBSTS register indicates status of the asynchronous schedule.
System software enables (or disables) the asynchronous schedule by writing a one (or zero) to the
Asynchronous Schedule Enable bit in the USBCMD register. Software then can poll the Asynchronous
Schedule Status bit to determine when the asynchronous schedule has made the desired transition.
Software must not modify the Asynchronous Schedule Enable bit unless the value of the Asynchronous Schedule
Enable bit equals that of the Asynchronous Schedule Status bit.

The asynchronous schedule is used to manage all Control and Bulk transfers. Control and Bulk transfers are
managed using queue head data structures. The asynchronous schedule is based at the ASYNCLISTADDR register.
The default value of the ASYNCLISTADDR register after reset is undefined and the schedule is
disabled when the Asynchronous Schedule Enable bit is a zero.

Software may only write this register with defined results when the schedule is disabled .e.g. Asynchronous
Schedule Enable bit in the USBCMD and the Asynchronous Schedule Status bit in the USBSTS register are zero.
System software enables execution from the asynchronous schedule by writing a valid memory address
(of a queue head) into this register. Then software enables the asychronous schedule by setting the
Asynchronous Schedule Enable bit is set to one. The asynchronous schedule is actually enabled when the
Asynchronous Schedule Status bit is a one.

When the host controller begins servicing the asynchronous schedule, it begins by using the value of the
ASYNCLISTADDR register. It reads the first referenced data structure and begins executing transactions and
traversing the linked list as appropriate. When the host controller "completes" processing the asynchronous
schedule, it retains the value of the last accessed queue head's horizontal pointer in the ASYNCLISTADDR register.
Next time the asynchronous schedule is accessed, this is the first data structure that will be serviced.
This provides round-robin fairness for processing the asynchronous schedule.

A host controller "completes" processing the asynchronous schedule when one of the following events occur:
• The end of a micro-frame occurs.
• The host controller detects an empty list condition (i.e. see Section 4.8.3)
• The schedule has been disabled via the Asynchronous Schedule Enable bit in the USBCMD register.

The queue heads in the asynchronous list are linked into a simple circular list as shown in Figure 4-4.
Queue head data structures are the only valid data structures that may be linked into the asynchronous schedule.
An isochronous transfer descriptor (iTD or siTD) in the asynchronous schedule yields undefined results.

The maximum packet size field in a queue head is sized to accommodate the use of this data structure for all non-isochronous transfer types.
The USB Specification, Revision 2.0 specifies the maximum packet sizes for all transfer types and transfer speeds.
System software should always parameterize the queue head data structures according to the core specification requirements.

ehci spec 1.0, chapter 4.8.3 Empty Asynchronous Schedule Detection
The Enhanced Host Controller Interface uses two bits to detect when the asynchronous schedule is empty.
The queue head data structure (see Figure 3-7) defines an H-bit in the queue head, which allows software to
mark a queue head as being the head of the reclaim list. The Enhanced Host Controller Interface also keeps
a 1-bit flag in the USBSTS register (Reclamation) that is set to a zero when the Enhanced Interface Host
Controller observes a queue head with the H-bit set to a one. The reclamation flag in the status register is set
to one when any USB transaction from the asynchronous schedule is executed (or whenever the asynchronous schedule starts, see Section 4.8.5).
If the Enhanced Host Controller Interface ever encounters an H-bit of one and a Reclamation bit of zero,
the EHCI controller simply stops traversal of the asynchronous schedule.
*/

#ifdef _EHCI_DIAGNOSIS_
static void checkAsyncScheduler(ehci_t* e)
{
    // cf. ehci spec 1.0, Figure 3-7. Queue Head Structure Layout

    textColor(IMPORTANT);

    // async scheduler: last QH accessed or QH to be accessed is shown by ASYNCLISTADDR register
    void* virtASYNCLISTADDR = paging_acquirePciMemory(e->OpRegs->ASYNCLISTADDR, 1);
    printf("\ncurr QH: %Xh ", paging_getPhysAddr(virtASYNCLISTADDR));

    // Last accessed & next to access QH, DWORD 0
    uintptr_t horizontalPointer = (*((uint32_t*)virtASYNCLISTADDR)) & 0xFFFFFFE0; // without last 5 bits
    /*
    uint32_t type = (BYTE1(*((uint32_t*)virtASYNCLISTADDR)) & 0x06) >> 1 ;       // bit 2:1
    uint32_t Tbit =  BYTE1(*((uint32_t*)virtASYNCLISTADDR)) & 0x01;              // bit 0
    */
    printf("\tnext QH: %Xh ", horizontalPointer);

    //printf("\ntype: %u T-bit: %u",type,Tbit);

    // Last accessed & next to access QH, DWORD 1
    /*
    uint32_t deviceAddress               = (BYTE1(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x7F);
    uint32_t inactivateOnNextTransaction = (BYTE1(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x80)>>7;
    uint32_t endpoint                    = (BYTE2(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x0F);
    uint32_t dataToggleControl           = (BYTE2(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x40)>>6;
    uint32_t Hbit                        = (BYTE2(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x80)>>7;
    uint32_t mult                        = (BYTE4(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0xC0)>>6;

    uint32_t maxPacket = ((BYTE4(*(((uint32_t*)virtASYNCLISTADDR)+1)) & 0x07) << 8) +
                           BYTE3(*(((uint32_t*)virtASYNCLISTADDR)+1));

    printf("\ndev: %u endp: %u inactivate: %u dtc: %u H: %u mult: %u maxPacket: %u",
             deviceAddress, endpoint, inactivateOnNextTransaction, dataToggleControl, Hbit, mult, maxPacket);
    */

    // Last accessed & next to access QH, DWORD 2
    // ...

    // Last accessed & next to access QH, DWORD 3
    uintptr_t firstQTD = (*(((uint32_t*)virtASYNCLISTADDR)+3)) & 0xFFFFFFE0; // without last 5 bits
    printf("\ncurr qTD: %Xh", firstQTD);

    // Last accessed & next to access QH, DWORD 4
    uintptr_t nextQTD = (*(((uint32_t*)virtASYNCLISTADDR)+4)) & 0xFFFFFFE0; // without last 5 bits
    printf("\tnext qTD: %Xh", nextQTD);

    // NAK counter in overlay area
    uint32_t NakCtr = (BYTE1(*(((uint32_t*)virtASYNCLISTADDR)+5)) & 0x1E)>>1;
    printf("\nNAK counter: %u", NakCtr);
    textColor(TEXT);
}
#endif

void performAsyncScheduler(ehci_t* e, bool stop, bool analyze, uint8_t velocity)
{
 #ifdef _EHCI_DIAGNOSIS_
    if (analyze)
    {
        printf("\nbefore aS:");
        checkAsyncScheduler(e);
    }
  #endif

    e->USBINTflag = false;
    e->USBasyncIntFlag = false;

    // Enable Asynchronous Schdeuler
    e->OpRegs->USBCMD |= CMD_ASYNCH_ENABLE | CMD_ASYNCH_INT_DOORBELL; // switch on and set doorbell

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

    timeout = 3 * velocity + 1;
    while (true /*!e->USBasyncIntFlag*/ )
    {
        timeout--;
        if (timeout>0)
        {
            sleepMilliSeconds(10);
        }
        else
        {
          #ifdef _EHCI_DIAGNOSIS_
            textColor(ERROR);
            printf("\nTimeout Error - ASYNC_INT not set!");
            textColor(TEXT);
          #endif
            break;
        }
    }

    if (timeout > 0)
    {
      #ifdef _EHCI_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nASYNC_INT successfully set! timeout-counter: %u", timeout);
        textColor(TEXT);
      #endif
    }

    timeout=3;
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
          #ifdef _EHCI_DIAGNOSIS_
            textColor(ERROR);
            printf("\nTimeout Error - STS_USBINT not set!");
            textColor(TEXT);
          #endif
            break;
        }
    }

    if (stop)
    {
        e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; // switch off

        timeout=7;
        while (e->OpRegs->USBSTS & STS_ASYNC_ENABLED) // wait until it is really off
        {
            timeout--;
            if (timeout>0)
            {
                sleepMilliSeconds(10);
              #ifdef _EHCI_DIAGNOSIS_
                textColor(LIGHT_MAGENTA);
                printf("!");
              #endif
            }
            else
            {
                textColor(ERROR);
                printf("\ntimeout - STS_ASYNC_ENABLED still set!");
                textColor(TEXT);
                break;
            }
        }
    }

  #ifdef _EHCI_DIAGNOSIS_
    if (analyze)
    {
        printf("\nafter aS:");
        checkAsyncScheduler(e);
    }
  #endif
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
