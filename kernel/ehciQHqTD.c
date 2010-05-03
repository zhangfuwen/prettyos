/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "usb2.h"
#include "ehci.h"
#include "ehciQHqTD.h"

void*     DataQTD;
void*     SetupQTD;

uintptr_t QTDpage0;
uintptr_t DataQTDpage0;
uintptr_t MSDStatusQTDpage0;
uintptr_t SetupQTDpage0;

// Queue Head (QH)

void createQH(void* address, uint32_t horizPtr, void* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    struct ehci_qhd* head = (struct ehci_qhd*)address;
    memset(address, 0, sizeof(struct ehci_qhd));

    head->horizontalPointer      =   horizPtr | 0x2; // bit 2:1   00b iTD,   01b QH,   10b siTD,   11b FSTN
    head->deviceAddress          =   device;         // The device address
    head->inactive               =   0;
    head->endpoint               =   endpoint;       // Endpoint 0 contains Device infos such as name
    head->endpointSpeed          =   2;              // 00b = full speed; 01b = low speed; 10b = high speed
    head->dataToggleControl      =   1;              // Get the Data Toggle bit out of the included qTD
    head->H                      =   H;
    head->maxPacketLength        =   packetSize;     // It's 64 bytes for a control transfer to a high speed device.
    head->controlEndpointFlag    =   0;              // Only used if Endpoint is a control endpoint and not high speed
    head->nakCountReload         =   0;              // This value is used by HC to reload the Nak Counter field.
    head->interruptScheduleMask  =   0;              // not used for async schedule
    head->splitCompletionMask    =   0;              // unused if (not low/full speed and in periodic schedule)
    head->hubAddr                =   0;              // unused if high speed (Split transfer)
    head->portNumber             =   0;              // unused if high speed (Split transfer)
    head->mult                   =   1;              // One transaction to be issued for this endpoint per micro-frame.
                                                     // Maybe unused for non interrupt queue head in async list
    if (firstQTD == NULL)
    {
        head->qtd.next = 0x1;
    }
    else
    {
        uint32_t physNext = paging_get_phys_addr(kernel_pd, firstQTD);
        head->qtd.next = physNext;
    }
}


// Queue Element Transfer Descriptor (qTD)

ehci_qtd_t* allocQTD(uintptr_t next)
{    
    ehci_qtd_t* td = (ehci_qtd_t*)malloc(sizeof(ehci_qtd_t), 0x20); // 32 Byte alignment

    if (next != 0x1)
    {
        td->next = paging_get_phys_addr(kernel_pd, (void*)next);         
    }
    else
    {
        td->next = 0x1; // no next qTD
    }
    return td;
}

uintptr_t allocQTDbuffer(ehci_qtd_t* td)
{
    void* data = malloc(PAGESIZE, PAGESIZE); // Enough for a full page
    memset(data,0,PAGESIZE);    

    td->buffer0 = paging_get_phys_addr(kernel_pd, data);
    td->buffer1 = td->buffer2 = td->buffer3 = td->buffer4 = 0x0;
    td->extend0 = td->extend1 = td->extend2 = td->extend3 = td->extend4 = 0x0;

    return (uintptr_t)data;
}

void* createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt = td->next; /// 0x1;     // No alternative next, so T-Bit is set to 1
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

    td->nextAlt = td->next; /// 0x1;     // No alternative next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = direction;  // OUT = 0, IN = 1
    td->token.errorCounter = 0x3;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = tokenBytes; // dependent on transfer
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    QTDpage0 = allocQTDbuffer(td);

    if (tokenBytes > 0) // data are transferred, no handshake
    {
        DataQTDpage0 = QTDpage0;
    }
    
    return (void*)td;
}

void* createQTD_MSDStatus(uintptr_t next, bool toggle)
{
    ehci_qtd_t* td = allocQTD(next);

    td->nextAlt = td->next;              // No alternative next, so T-Bit is set to 1
    td->token.status       = 0x80;       // This will be filled by the Host Controller
    td->token.pid          = IN;         // OUT = 0, IN = 1
    td->token.errorCounter = 0x3;        // Written by the Host Controller.
    td->token.currPage     = 0x0;        // Start with first page. After that it's written by Host Controller???
    td->token.interrupt    = 0x1;        // We want an interrupt after complete transfer
    td->token.bytes        = 13;         // dependent on transfer, here 13 byte status information
    td->token.dataToggle   = toggle;     // Should be toggled every list entry

    MSDStatusQTDpage0 = allocQTDbuffer(td);

    (*(((uint32_t*)MSDStatusQTDpage0)+0)) = 0x53425355; // magic USBS 
    (*(((uint32_t*)MSDStatusQTDpage0)+1)) = 0xAAAAAAAA; // CSWTag
    (*(((uint32_t*)MSDStatusQTDpage0)+2)) = 0xAAAAAAAA; //
    (*(((uint32_t*)MSDStatusQTDpage0)+3)) = 0xFFFFFFAA; //

    return (void*)td;
}

void* createQTD_Handshake(uint8_t direction)
{
    return createQTD_IO(0x1, direction, 1,  0);
}

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
            {
                printf("%c", *((uint8_t*)virtAddrBuf0+c));
            }
        }
        else
        {
            textColor(0x03);
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
    showData(virtAddrBuf0, size, false);
}

void showStatusbyteQTD(void* addressQTD)
{
    textColor(0x0F);
    uint8_t statusbyte = *((uint8_t*)addressQTD+8);
    printf("\nqTD Status: %y\t", statusbyte);

    // analyze status byte (cf. EHCI 1.0 spec, Table 3-16 Status in qTD Token)

    // Status not OK
    textColor(0x0E);
    if (statusbyte & (1<<7)) { printf("Active - HC transactions enabled"); }
    if (statusbyte & (1<<6)) { printf("Halted - serious error at the device/endpoint"); }
    if (statusbyte & (1<<5)) { printf("Data Buffer Error (overrun or underrun)"); }
    if (statusbyte & (1<<4)) { printf("Babble (fatal error leads to Halted)"); }
    if (statusbyte & (1<<3)) { printf("Transaction Error (XactErr)- host received no valid response device"); }
    if (statusbyte & (1<<2)) { printf("Missed Micro-Frame"); }
    if (statusbyte & (1<<1)) { printf("Do Complete Split"); }
    if (statusbyte & (1<<0)) { printf("Do Ping"); }
    textColor(0x0A);

    // Status OK
    if (statusbyte == 0)     { printf("OK (no bit set)"); }
    textColor(0x0F);
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
