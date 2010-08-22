/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "paging.h"
#include "kheap.h"
#include "usb2.h"
#include "ehci.h"
#include "ehciQHqTD.h"
#include "video/console.h"

void*        DataQTD;            // pointer to IO qTD transferring data
void*        SetupQTD;           // pointer to Setup qTD transferring control transfer command
extern void* StatusQTD;          // pointer to IN qTD transferring CSW

uintptr_t    QTDpage0;           // pointer to qTD page0 (general)
uintptr_t    DataQTDpage0;       // pointer to qTD page0 (In/Out data)
uintptr_t    MSDStatusQTDpage0;  // pointer to qTD page0 (IN, mass storage device status)
uintptr_t    SetupQTDpage0;      // pointer to qTD page0 (OUT, setup control transfer)

void* globalqTD[3];
void* globalqTDbuffer[3];

const uint32_t CSWMagicNotOK = 0x01010101;

extern const uint8_t ALIGNVALUE;

/////////////////////
// Queue Head (QH) //
/////////////////////

void createQH(void* address, uint32_t horizPtr, void* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    struct ehci_qhd* head = (struct ehci_qhd*)address;
    memset(address, 0, sizeof(struct ehci_qhd));

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
    head->nakCountReload        =  15;              // this value is used by EHCI to reload the Nak Counter field. 0=ignores NAK counter.
    head->interruptScheduleMask =   0;              // not used for async schedule
    head->splitCompletionMask   =   0;              // unused if (not low/full speed and in periodic schedule)
    head->hubAddr               =   0;              // unused if high speed (Split transfer)
    head->portNumber            =   0;              // unused if high speed (Split transfer)
    head->mult                  =   1;              // 1-3 transaction per micro-frame, 0 means undefined results
    if (firstQTD == 0)
        head->qtd.next = 0x1;
    else
    {
        uint32_t physNext = paging_get_phys_addr(firstQTD);
        head->qtd.next = physNext;
    }
}

/////////////////////////////////////////////
// Queue Element Transfer Descriptor (qTD) //
/////////////////////////////////////////////

ehci_qtd_t* allocQTD(uintptr_t next)
{
    ehci_qtd_t* td = (ehci_qtd_t*)malloc(sizeof(ehci_qtd_t), ALIGNVALUE, "qTD"); // can be 32 byte alignment
    memset(td,0,sizeof(ehci_qtd_t));

    if (next != 0x1)
        td->next = paging_get_phys_addr((void*)next);
    else
        td->next = 0x1;

    globalqTD[0] = (void*)td; // save pointer for later free(pointer)

    return td;
}

uintptr_t allocQTDbuffer(ehci_qtd_t* td)
{
    void* data = malloc(PAGESIZE, ALIGNVALUE, "qTD-buffer"); // Enough for a full page
    memset(data,0,PAGESIZE);

    td->buffer0 = paging_get_phys_addr(data);
    td->buffer1 = td->buffer2 = td->buffer3 = td->buffer4 = 0x0;
    td->extend0 = td->extend1 = td->extend2 = td->extend3 = td->extend4 = 0x0;

    globalqTDbuffer[0] = (void*)data; // save pointer for later free(pointer)

    return (uintptr_t)data;
}

void* createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;        // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = 0x2;        // SETUP = 2
    td->token.errorCounter = 0x3;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    SetupQTDpage0 = allocQTDbuffer(td);

    struct ehci_request* request = (struct ehci_request*)SetupQTDpage0;
    request->type    = type;
    request->request = req;
    request->valueHi = hiVal;
    request->valueLo = loVal;
    request->index   = index;
    request->length  = length;

    return (void*)td;
}

void* createQTD_IO(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;        // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = direction;  // OUT = 0, IN = 1
    td->token.errorCounter = 0x3;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    QTDpage0 = allocQTDbuffer(td);

    if (tokenBytes > 0) // data are transferred, no handshake
        DataQTDpage0 = QTDpage0;

    return (void*)td;
}

void* createQTD_IO_OUT(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes, uint8_t* buffer)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;        // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = direction;  // OUT = 0, IN = 1
    td->token.errorCounter = 0x3;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    DataQTDpage0 = QTDpage0 = allocQTDbuffer(td);
    memcpy((void*)QTDpage0,(void*)buffer,512);

    return (void*)td;
}

void* createQTD_MSDStatus(uintptr_t next, bool toggle)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt            = 0x1;    // No alternate next, so T-Bit is set to 1
    td->token.status       = 0x80;   // This will be filled by the Host Controller
    td->token.pid          = IN;     // OUT = 0, IN = 1
    td->token.errorCounter = 0x3;    // Written by the Host Controller.
    td->token.currPage     = 0x0;    // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;    // We want an interrupt after complete transfer
    td->token.bytes        = 13;     // dependent on transfer, here 13 byte status information
    td->token.dataToggle   = toggle; // Should be toggled every list entry

    MSDStatusQTDpage0 = allocQTDbuffer(td);

    (*(((uint32_t*)MSDStatusQTDpage0)+0)) = CSWMagicNotOK; // magic USBS
    (*(((uint32_t*)MSDStatusQTDpage0)+1)) = 0xAAAAAAAA; // CSWTag
    (*(((uint32_t*)MSDStatusQTDpage0)+2)) = 0xAAAAAAAA;
    (*(((uint32_t*)MSDStatusQTDpage0)+3)) = 0xFFFFFFAA;

    return (void*)td;
}

void* createQTD_Handshake(uint8_t direction)
{
    return createQTD_IO(0x1, direction, 1, 0);
}


////////////////////
// analysis tools //
////////////////////

static void showData(uint32_t virtAddrBuf0, uint32_t size, bool alphanumeric)
{
    #ifdef _USB_DIAGNOSIS_
    printf("virtAddrBuf0 %X : ", virtAddrBuf0);
    #endif
    for (uint32_t c=0; c<size; c++)
    {
        if (alphanumeric)
        {
            textColor(0x0F);
            if ( (*((uint8_t*)virtAddrBuf0+c)>=0x20) && (*((uint8_t*)virtAddrBuf0+c)<=0x7E) )
                printf("%c", *((uint8_t*)virtAddrBuf0+c));
        }
        else
        {
            textColor(0x07);
            printf("%y ", *((uint8_t*)virtAddrBuf0+c));
            textColor(0x0F);
        }
    }
    printf("\n");
}

void showPacketAlphaNumeric(uint32_t virtAddrBuf0, uint32_t size)
{
    showData(virtAddrBuf0, size, true);
}

void showPacket(uint32_t virtAddrBuf0, uint32_t size)
{
  #ifdef _USB_DIAGNOSIS_
    showData(virtAddrBuf0, size, false);
  #endif
}

uint32_t showStatusbyteQTD(void* addressQTD)
{
    uint8_t statusbyte = getField(addressQTD, 8, 0, 8);
    if (statusbyte != 0x00)
    {
        printf("\n");
        textColor(0x0E);
        if (statusbyte & BIT(7)) { printf("Active - HC transactions enabled"); }
        textColor(0x0C);
        if (statusbyte & BIT(6)) { printf("Halted - serious error at the device/endpoint"); }
        if (statusbyte & BIT(5)) { printf("Data Buffer Error (overrun or underrun)"); }
        if (statusbyte & BIT(4)) { printf("Babble (fatal error leads to Halted)"); }
        if (statusbyte & BIT(3)) { printf("Transaction Error (XactErr)- host received no valid response device"); }
        if (statusbyte & BIT(2)) { printf("Missed Micro-Frame"); }
        textColor(0x0E);
        if (statusbyte & BIT(1)) { printf("Do Complete Split"); }
        if (statusbyte & BIT(0)) { printf("Do Ping"); }
        textColor(0x0F);
    }
    return statusbyte;
}

/////////////////////////////////////
// Asynchronous schedule traversal //
/////////////////////////////////////

/*
ehci spec 1.0, chapter 4.4 Schedule Traversal Rules:

... , the host controller executes from the asynchronous schedule until the end of the micro-frame.
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

void checkAsyncScheduler()
{
    // cf. ehci spec 1.0, Figure 3-7. Queue Head Structure Layout

    textColor(0x02);

    // async scheduler: last QH accessed or QH to be accessed is shown by ASYNCLISTADDR register
    void* virtASYNCLISTADDR = paging_acquire_pcimem(pOpRegs->ASYNCLISTADDR);
    printf("\ncurr QH: %X ",paging_get_phys_addr(virtASYNCLISTADDR));

    // Last accessed & next to access QH, DWORD 0
    uintptr_t horizontalPointer = (*((uint32_t*)virtASYNCLISTADDR)) & 0xFFFFFFE0; // without last 5 bits
    /*
    uint32_t type = (BYTE1(*((uint32_t*)virtASYNCLISTADDR)) & 0x06) >> 1 ;       // bit 2:1
    uint32_t Tbit =  BYTE1(*((uint32_t*)virtASYNCLISTADDR)) & 0x01;              // bit 0
    */
    printf("\tnext QH: %X ",horizontalPointer);

    //printf("\ntype: %u T-bit: %u",type,Tbit);

    // Last accessed & next to access QH, DWORD 1
    /*
    uint32_t deviceAddress               = (BYTE1( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x7F);
    uint32_t inactivateOnNextTransaction = (BYTE1( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x80)>>7;
    uint32_t endpoint                    = (BYTE2( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x0F);
    uint32_t dataToggleControl           = (BYTE2( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x40)>>6;
    uint32_t Hbit                        = (BYTE2( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x80)>>7;
    uint32_t mult                        = (BYTE4( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0xC0)>>6;

    uint32_t maxPacket = (( BYTE4( *( ((uint32_t*)virtASYNCLISTADDR)+1) ) & 0x07 ) << 8 ) +
                            BYTE3( *( ((uint32_t*)virtASYNCLISTADDR)+1) );

    printf("\ndev: %u endp: %u inactivate: %u dtc: %u H: %u mult: %u maxPacket: %u",
             deviceAddress, endpoint, inactivateOnNextTransaction, dataToggleControl, Hbit, mult, maxPacket);
    */

    // Last accessed & next to access QH, DWORD 2
    // ...

    // Last accessed & next to access QH, DWORD 3
    uintptr_t firstQTD = (*( ((uint32_t*)virtASYNCLISTADDR)+3)) & 0xFFFFFFE0; // without last 5 bits
    printf("\ncurr qTD: %X",firstQTD);

    // Last accessed & next to access QH, DWORD 4
    uintptr_t nextQTD = (*( ((uint32_t*)virtASYNCLISTADDR)+4)) & 0xFFFFFFE0; // without last 5 bits
    printf("\tnext qTD: %X",nextQTD);

    // NAK counter in overlay area
    uint32_t NakCtr = (BYTE1( *( ((uint32_t*)virtASYNCLISTADDR)+5) ) & 0x1E)>>1;
    printf("\nNAK counter: %u",NakCtr);
    textColor(0x0E);
}

void performAsyncScheduler(bool stop, bool analyze, uint8_t velocity)
{
    if (analyze)
    {
      #ifdef _USB_DIAGNOSIS_
        printf("\nbefore aS:");
        checkAsyncScheduler();
      #endif
    }

    // Enable Asynchronous Schdeuler
    pOpRegs->USBSTS |= STS_USBINT;
    USBINTflag = false;

    pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE | CMD_ASYNCH_INT_DOORBELL; // switch on and set doorbell

    int8_t timeout=7;
    while (!(pOpRegs->USBSTS & STS_ASYNC_ENABLED)) // wait until it is really on
    {
        timeout--;
        if(timeout>0)
        {
            sleepMilliSeconds(20);
          #ifdef _USB_DIAGNOSIS_
            textColor(0x0D);
            printf(">");
            textColor(0x0F);
          #endif
       }
       else
       {
            textColor(0x0C);
            printf("\ntimeout - STS_ASYNC_ENABLED still not set!");
            textColor(0x0F);
            break;
        }
    }

    sleepMilliSeconds(100 + velocity * 100);

    timeout=7;
    while (!USBINTflag) // set by interrupt
    {
        timeout--;
        if(timeout>0)
        {
            sleepMilliSeconds(20);

          #ifdef _USB_DIAGNOSIS_
            textColor(0x0D);
            printf("#");
            textColor(0x0F);
          #endif
        }
        else
        {
            textColor(0x0C);
            printf("\ntimeout - no STS_USBINT set!");
            textColor(0x0F);
            break;
        }
    };

    pOpRegs->USBSTS |= STS_USBINT;
    USBINTflag = false;

    if (stop)
    {
        pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; // switch off

        timeout=7;
        while (pOpRegs->USBSTS & STS_ASYNC_ENABLED) // wait until it is really off
        {
            timeout--;
            if(timeout>0)
            {
                sleepMilliSeconds(20);
              #ifdef _USB_DIAGNOSIS_
                textColor(0x0D);
                printf("!");
                textColor(0x0F);
              #endif
            }
            else
            {
                textColor(0x0C);
                printf("\ntimeout - STS_ASYNC_ENABLED still set!");
                textColor(0x0F);
                break;
            }
        }
    }

    if (analyze)
    {
      #ifdef _USB_DIAGNOSIS_
        printf("\nafter aS:");
        checkAsyncScheduler();
      #endif
    }
}

void logBulkTransfer(usbBulkTransfer_t* bT)
{
    if (!bT->successfulCommand ||
        !bT->successfulCSW     ||
        (bT->DataBytesToTransferOUT && !bT->successfulDataOUT) ||
        (bT->DataBytesToTransferIN  && !bT->successfulDataIN))
    {
        textColor(0x03);
        printf("\nopcode: %y", bT->SCSIopcode);
        printf("  cmd: %s",    bT->successfulCommand ? "OK" : "Error");
        if (bT->DataBytesToTransferOUT)
        {
            printf("  data out: %s", bT->successfulDataOUT ? "OK" : "Error");
        }
        if (bT->DataBytesToTransferIN)
        {
            printf("  data in: %s",  bT->successfulDataIN  ? "OK" : "Error");
        }
        printf("  CSW: %s",    bT->successfulCSW     ? "OK" : "Error");
        textColor(0x0F);
    }
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
