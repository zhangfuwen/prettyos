/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "paging.h"
#include "kheap.h"
#include "console.h"
#include "util.h"
#include "ehci.h"
#include "ehciQHqTD.h"
#include "usb2.h"
#include "usb2_msd.h"

extern usb2_Device_t usbDevices[17]; // ports 1-16 // 0 not used

static void waitForKeyStroke()
{
   textColor(0x0D);
   printf("\n>>> Press key to go on with USB-Test. <<<");
   textColor(0x0F);
   while(!keyboard_getChar());
   printf("\n");
}

// Bulk-Only Mass Storage get maximum number of Logical Units
uint8_t usbTransferBulkOnlyGetMaxLUN(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: usbTransferBulkOnlyGetMaxLUN, dev: %d interface: %d", device, numInterface); textColor(0x0F);
    #endif

    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next      = createQTD_Handshake(OUT); // Handshake is the opposite direction of Data
    next = DataQTD  = createQTD_IO( (uintptr_t)next, IN, 1, 1);  // IN DATA1, 1 byte
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0xA1, 0xFE, 0, 0, numInterface, 1);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 10100001b     11111110b 0000h  Interface 0001h     1 byte

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
    return *((uint8_t*)DataQTDpage0);
}

// Bulk-Only Mass Storage Reset
void usbTransferBulkOnlyMassStorageReset(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: usbTransferBulkOnlyMassStorageReset, dev: %d interface: %d", device, numInterface); textColor(0x0F);
    #endif

    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x21, 0xFF, 0, 0, numInterface, 0);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 00100001b     11111111b 0000h  Interface 0000h     none

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(uint32_t device, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, bool MSDStatus)
{
    void* QH_Out = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    void* QH_In  = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; 
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH_Out);

    // OUT qTD
    // No handshake 
    void* cmdQTD  = createQTD_IO(0x01, OUT, 0, 31); // OUT DATA0, 31 byte

    // http://en.wikipedia.org/wiki/SCSI_CDB
    struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
    memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw

    switch (SCSIcommand)
    {
    case 0x00: // test unit ready(6)

        cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 0;
        cbw->CBWFlags              = 0x00; // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;    // only bits 3:0
        cbw->CBWCBLength           = 6;    // only bits 4:0
        cbw->commandByte[0] = 0x00; // Operation code
        cbw->commandByte[1] = 0;    // Reserved
        cbw->commandByte[2] = 0;    // Reserved
        cbw->commandByte[3] = 0;    // Reserved
        cbw->commandByte[4] = 0;    // Reserved
        cbw->commandByte[5] = 0;    // Control
        break;

    case 0x03: // Request Sense(6)

        cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 18;
        cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;    // only bits 3:0
        cbw->CBWCBLength           = 6;    // only bits 4:0
        cbw->commandByte[0] = 0x03; // Operation code
        cbw->commandByte[1] = 0;    // Reserved
        cbw->commandByte[2] = 0;    // Reserved
        cbw->commandByte[3] = 0;    // Reserved
        cbw->commandByte[4] = 18;   // Allocation length (max. bytes)
        cbw->commandByte[5] = 0;    // Control
        break;

    case 0x12: // Inquiry(6) 

        cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 0x24;
        cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;    // only bits 3:0
        cbw->CBWCBLength           = 6;    // only bits 4:0
        cbw->commandByte[0] = 0x12; // Operation code
        cbw->commandByte[1] = 0;    // Reserved
        cbw->commandByte[2] = 0;    // Reserved
        cbw->commandByte[3] = 0;    // Reserved
        cbw->commandByte[4] = 0x24; // Allocation length (max. bytes)
        cbw->commandByte[5] = 0;    // Control
        break;

    case 0x25: // read capacity(10)

        cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 8;
        cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
        cbw->CBWLUN                =  0; // only bits 3:0
        cbw->CBWCBLength           = 10; // only bits 4:0
        cbw->commandByte[0] = 0x25; // Operation code
        cbw->commandByte[1] = 0;    // 7:5 LUN  4:1 reserved  0 RelAddr
        cbw->commandByte[2] = BYTE4(LBA);    // LBA MSB
        cbw->commandByte[3] = BYTE3(LBA);    // LBA
        cbw->commandByte[4] = BYTE2(LBA);    // LBA
        cbw->commandByte[5] = BYTE1(LBA);    // LBA LSB
        cbw->commandByte[6] = 0;    // Reserved
        cbw->commandByte[7] = 0;    // Reserved
        cbw->commandByte[8] = 0;    // 7:1 Reserved  0 PMI
        cbw->commandByte[9] = 0;    // Control
        break;

    case 0x28: // read(10)

        cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = TransferLength;
        cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
        cbw->CBWLUN                =  0; // only bits 3:0
        cbw->CBWCBLength           = 10; // only bits 4:0
        cbw->commandByte[0] = 0x28; // Operation code
        cbw->commandByte[1] = 0;    // Reserved
        cbw->commandByte[2] = BYTE4(LBA);    // LBA MSB
        cbw->commandByte[3] = BYTE3(LBA);    // LBA
        cbw->commandByte[4] = BYTE2(LBA);    // LBA
        cbw->commandByte[5] = BYTE1(LBA);    // LBA LSB
        cbw->commandByte[6] = 0;    // Reserved
        cbw->commandByte[7] = BYTE2(TransferLength);
        cbw->commandByte[8] = BYTE1(TransferLength);
        cbw->commandByte[9] = 0;    // Control
        break;
    }

    // OUT QH
    createQH(QH_Out, paging_get_phys_addr(kernel_pd, QH_In), cmdQTD,  1, device, endpointOut, 512);

    // IN qTDs
    void* next = createQTD_Handshake(OUT); // Handshake
    if (MSDStatus==true)
    {
        if (TransferLength > 0)
        {
            next = createQTD_MSDStatus((uintptr_t)next, 1); // next, toggle // IN 13 byte
            DataQTD = createQTD_IO((uintptr_t)next, IN,  0, TransferLength); // IN/OUT DATA0, ... byte
        }
        else
        {
            DataQTD = createQTD_MSDStatus((uintptr_t)next, 0); // next, toggle // IN 13 byte
        }
    }

    if (MSDStatus==false)
    {
        if (TransferLength > 0)
        {
            DataQTD = createQTD_IO((uintptr_t)next, IN, 0, TransferLength); // IN DATA0, ... byte
        }
        else
        {
            DataQTD = NULL; // no qTD
        }
    }

    // IN QH
    createQH(QH_In, paging_get_phys_addr(kernel_pd, QH_Out), DataQTD, 0, device, endpointIn, 512); // endpoint IN/OUT for MSD

    performAsyncScheduler();

    if (TransferLength)
    {
        printf("\n");
        showPacket(DataQTDpage0,TransferLength);
        showPacketAlphaNumeric(DataQTDpage0,TransferLength);
    }

    if (MSDStatus)
    {
        printf("\n");
        showPacket(MSDStatusQTDpage0,13);

        if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x0 )
        {
            printf("\nCommand Passed (\"good status\") ");
        }
        if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x1 )
        {
            printf("\nCommand failed");
        }
        if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x2 )
        {
            printf("\nPhase Error");
        }
    }

    // transfer diagnosis
    showStatusbyteQTD(DataQTD);
}

void testMSD(uint8_t devAddr)
{
    // maxLUN (0 for USB-sticks)
    usbDevices[devAddr].maxLUN = 0;

    usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
    uint8_t statusByte;

    ///////// step 1: send SCSI comamnd "inquiry"

    textColor(0x09); printf("\n>>> SCSI: inquiry"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x12, 0, 0x24, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }    
    waitForKeyStroke();

    ///////// step 2: send SCSI comamnd "test unit ready(6)"

    textColor(0x09); printf("\n>>> SCSI: test unit ready"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }
    waitForKeyStroke();

    ///////// step 3: send SCSI comamnd "request sense"

    textColor(0x09); printf("\n>>> SCSI: request sense"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }
    waitForKeyStroke();

    ///////// step 4: send SCSI comamnd "test unit ready(6)"

    textColor(0x09); printf("\n>>> SCSI: test unit ready"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }
    waitForKeyStroke();

    ///////// step 5: send SCSI comamnd "request sense"

    textColor(0x09); printf("\n>>> SCSI: request sense"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
     textColor(0x02);
     printf("\n\nCommand Block Status Values in \"good status\"\n");
     textColor(0x0F);
    }
    waitForKeyStroke();
     
    ///////// step 6: send SCSI comamnd "read capacity(10)"

    //usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface

    textColor(0x09); printf("\n>>> SCSI: read capacity"); textColor(0x0F);
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x25, 0, 8, true); // dev, endp, cmd, LBA, transfer length, MSDStatus
    uint32_t lastLBA    = (*((uint8_t*)DataQTDpage0+0)) * 16777216 + (*((uint8_t*)DataQTDpage0+1)) * 65536 + (*((uint8_t*)DataQTDpage0+2)) * 256 + (*((uint8_t*)DataQTDpage0+3));
    uint32_t blocksize  = (*((uint8_t*)DataQTDpage0+4)) * 16777216 + (*((uint8_t*)DataQTDpage0+5)) * 65536 + (*((uint8_t*)DataQTDpage0+6)) * 256 + (*((uint8_t*)DataQTDpage0+7));
    uint32_t capacityMB = ((lastLBA+1)/1000000) * blocksize;

    textColor(0x0E);
    printf("\nCapacity: %d MB, Last LBA: %d, block size %d\n", capacityMB, lastLBA, blocksize);
    textColor(0x0F);
    waitForKeyStroke();


    ///////// step 7: send SCSI comamnd "read(10)", read 512 byte from LBA ..., get Status

    uint32_t length = 512; // number of byte to be read

    for(uint32_t sector=0; sector<10; sector++)
    {
     textColor(0x09); printf("\n>>> SCSI: read(10)"); textColor(0x0F);
     usbSendSCSIcmd(devAddr, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x28, sector, length, false); // dev, endp, cmd, LBA, transfer length, MSDStatus
     usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
     waitForKeyStroke();
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
