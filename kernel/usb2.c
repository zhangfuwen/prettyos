/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "kheap.h"
#include "paging.h"
#include "console.h"
#include "timer.h"
#include "util.h"
#include "ehci.h"
#include "ehciQHqTD.h"
#include "usb2.h"

usb2_Device_t usbDevices[17]; // ports 1-16 // 0 not used


uint8_t usbTransferEnumerate(uint8_t j)
{
    #ifdef _USB_DIAGNOSIS_
      textColor(0x0B); printf("\nUSB2: SET_ADDRESS"); textColor(0x0F);
    #endif

    uint8_t new_address = j+1; // indicated port number

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 5, 0, new_address, 0, 0); // SETUP DATA0, 8 byte, ..., SET_ADDRESS, hi, 0...127 (new address), index=0, length=0

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, 0, 0,64);

    performAsyncScheduler(true, false);

    return new_address; // new_address
}

void usbTransferDevice(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: GET_DESCRIPTOR device, dev: %d endpoint: 0", device); textColor(0x0F);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(              0x1, OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 18);  // IN DATA1, 18 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 1, 0, 0, 18); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, device, lo, index, length

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");

    // showPacket(DataQTDpage0,18);
    addDevice ( (struct usb2_deviceDescriptor*)DataQTDpage0, &usbDevices[device] );
    showDevice( &usbDevices[device] );
}

void usbTransferConfig(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
      textColor(0x0B); printf("\nUSB2: GET_DESCRIPTOR config, dev: %d endpoint: 0", device); textColor(0x0F);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, PAGESIZE);  // IN DATA1, 4096 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 2, 0, 0, PAGESIZE); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, configuration, lo, index, length

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");

    // parsen auf config (len=9,type=2), interface (len=9,type=4), endpoint (len=7,type=5)
    uintptr_t addrPointer = (uintptr_t)DataQTDpage0;
    uintptr_t lastByte    = addrPointer + (*(uint16_t*)(addrPointer+2)); // totalLength (WORD)
    // printf("\nlastByte: %X\n",lastByte); // test

    #ifdef _USB_DIAGNOSIS_
      showPacket(DataQTDpage0,(*(uint16_t*)(addrPointer+2)));
    #endif

    while(addrPointer<lastByte)
    {
        bool found = false;
        // printf("addrPointer: %X\n",addrPointer); // test
        if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 2) ) // length, type
        {
            showConfigurationDescriptor((struct usb2_configurationDescriptor*)addrPointer);
            addrPointer += 9;
            found = true;
        }

        if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 4) ) // length, type
        {
            showInterfaceDescriptor((struct usb2_interfaceDescriptor*)addrPointer);

            if (((struct usb2_interfaceDescriptor*)addrPointer)->interfaceClass == 8)
            {
                // store interface number for mass storage transfers
                usbDevices[device].numInterfaceMSD    = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceNumber;
                usbDevices[device].InterfaceClass     = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceClass;
                usbDevices[device].InterfaceSubclass  = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceSubclass;
            }
            addrPointer += 9;
            found = true;
        }

        if ( ((*(uint8_t*)addrPointer) == 7) && ((*(uint8_t*)(addrPointer+1)) == 5) ) // length, type
        {
            showEndpointDescriptor ((struct usb2_endpointDescriptor*)addrPointer);

            // store endpoint numbers for IN/OUT mass storage transfers, attributes must be 0x2, because there are also endpoints with attributes 0x3(interrupt)
            if ( (((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0x80) && (((struct usb2_endpointDescriptor*)addrPointer)->attributes == 0x2) )
            {
                usbDevices[device].numEndpointInMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
            }
            
            if (!(((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0x80) && (((struct usb2_endpointDescriptor*)addrPointer)->attributes == 0x2) )
            {
                usbDevices[device].numEndpointOutMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
            }

            addrPointer += 7;
            found = true;
        }

        if ( ((*(uint8_t*)(addrPointer+1)) != 2 ) && ((*(uint8_t*)(addrPointer+1)) != 4 ) && ((*(uint8_t*)(addrPointer+1)) != 5 ) ) // length, type
        {            
            if ( (*(uint8_t*)addrPointer) > 0)
            {
                textColor(0x09);
                printf("\nlength: %d type: %d unknown\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
                textColor(0x0F);
            }            
            addrPointer += *(uint8_t*)addrPointer;
            found = true;
        }

        if (found == false)
        {
            printf("\nlength: %d type: %d not found\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
            break;
        }

        textColor(0x0D);
        printf("\n>>> Press key to go on with data analysis from config descriptor. <<<");
        textColor(0x0F);
        while(!keyboard_getChar());
        printf("\n");
    }
}

void usbTransferString(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
      textColor(0x0B); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: 0 languageIDs", device); textColor(0x0F);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 12);  // IN DATA1, 12 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, 0, 0, 12); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, lo, index, length

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);

    #ifdef _USB_DIAGNOSIS_
      showPacket(DataQTDpage0,12);
    #endif
    showStringDescriptor((struct usb2_stringDescriptor*)DataQTDpage0);
}

void usbTransferStringUnicode(uint32_t device, uint32_t stringIndex)
{
    #ifdef _USB_DIAGNOSIS_
      textColor(0x0B); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: 0 stringIndex: %d", device, stringIndex); textColor(0x0F);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 64);  // IN DATA1, 64 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, stringIndex, 0x0409, 64); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, stringIndex, languageID, length

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);

    #ifdef _USB_DIAGNOSIS_
      showPacket(DataQTDpage0,64);
    #endif

    showStringDescriptorUnicode((struct usb2_stringDescriptorUnicode*)DataQTDpage0);
}

// http://www.lowlevel.eu/wiki/USB#SET_CONFIGURATION
void usbTransferSetConfiguration(uint32_t device, uint32_t configuration)
{
  #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: SET_CONFIGURATION %d",configuration); textColor(0x0F);
  #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 9, 0, configuration, 0, 0); // SETUP DATA0, 8 byte, request type, SET_CONFIGURATION(9), hi(reserved), configuration, index=0, length=0

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);
}

uint8_t usbTransferGetConfiguration(uint32_t device)
{
  #ifdef _USB_DIAGNOSIS_
      textColor(0x0B); printf("\nUSB2: GET_CONFIGURATION"); textColor(0x0F);
  #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(OUT);
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 1);  // IN DATA1, 1 byte
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 8, 0, 0, 0, 1); // SETUP DATA0, 8 byte, request type, GET_CONFIGURATION(9), hi, lo, index=0, length=1

    // Create QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);

    uint8_t configuration = *((uint8_t*)DataQTDpage0);
    return configuration;
}

void usbClearFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
  #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: usbClearFeatureHALT, endpoint: %d", endpoint); textColor(0x0F);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x02, 1, 0, 0, endpoint, 0);
    // bmRequestType bRequest  wValue   wIndex  wLength   Data
    //     00000010b       1b   0000h endpoint    0000h   none

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, endpoint, packetSize); // endpoint 

    performAsyncScheduler(true, false);
    printf("\nclear HALT at dev: %d endpoint: %d", device, endpoint);
}

uint16_t usbGetStatus(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    textColor(0x0E); 
    printf("\nusbGetStatus at device: %d endpoint: %d", device, endpoint); 
    textColor(0x0F);
    
    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(OUT);
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 2);  // IN DATA1, 2 byte
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x02, 0, 0, 0, endpoint, 2);
    // bmRequestType bRequest  wValue   wIndex  wLength   Data
    //     00000010b       0b   0000h endpoint    0002h   2 byte

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, endpoint, packetSize); // endpoint 

    performAsyncScheduler(true, false);
    uint16_t status = *((uint16_t*)DataQTDpage0);
    return status;
}

void addDevice(struct usb2_deviceDescriptor* d, usb2_Device_t* usbDev)
{
    usbDev->usbSpec               = d->bcdUSB;
    usbDev->usbClass              = d->deviceClass;
    usbDev->usbSubclass           = d->deviceSubclass;
    usbDev->usbProtocol           = d->deviceProtocol;
    usbDev->maxPacketSize         = d->maxPacketSize;
    usbDev->vendor                = d->idVendor;
    usbDev->product               = d->idProduct;
    usbDev->releaseNumber         = d->bcdDevice;
    usbDev->manufacturerStringID  = d->manufacturer;
    usbDev->productStringID       = d->product;
    usbDev->serNumberStringID     = d->serialNumber;
    usbDev->numConfigurations     = d->numConfigurations;
}

void showDevice(usb2_Device_t* usbDev)
{
    textColor(0x0A);
    printf("\nUSB specification: %d.%d\t\t",    usbDev->usbSpec>>8, usbDev->usbSpec&0xFF); // e.g. 0x0210 means 2.10
    printf("USB class:         %x\n",           usbDev->usbClass);
    printf("USB subclass:      %x\t",           usbDev->usbSubclass);
    printf("USB protocol       %x\n",           usbDev->usbProtocol);
    printf("max packet size:   %d\t\t",         usbDev->maxPacketSize); // MPS0, must be 8,16,32,64

  #ifdef _USB_DIAGNOSIS_
    printf("vendor:            %x\n",           usbDev->vendor);
    printf("product:           %x\t",           usbDev->product);
    printf("release number:    %d.%d\n",        usbDev->releaseNumber>>8, usbDev->releaseNumber&0xFF);  
    printf("manufacturer:      %x\t",           usbDev->manufacturerStringID);
    printf("product:           %x\n",           usbDev->productStringID);
    printf("serial number:     %x\t",           usbDev->serNumberStringID);
    printf("number of config.: %d\n",           usbDev->numConfigurations); // number of possible configurations
    printf("numInterfaceMSD:   %d\n",           usbDev->numInterfaceMSD);
  #endif
    textColor(0x0F);
}

void showConfigurationDescriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
        textColor(0x0A);
        printf("\n");
      #ifdef _USB_DIAGNOSIS_
        printf("length:               %d\t\t",  d->length);
        printf("descriptor type:      %d\n",  d->descriptorType);
      #endif
        textColor(0x07);
        printf("total length:         %d\t",  d->totalLength);
        textColor(0x0A);
        printf("number of interfaces: %d\n",  d->numInterfaces);
      #ifdef _USB_DIAGNOSIS_
        printf("ID of config:         %x\t",  d->configurationValue);
        printf("ID of config name     %x\n",  d->configuration);
        printf("remote wakeup:        %s\t",  d->attributes & (1<<5) ? "yes" : "no");
        printf("self-powered:         %s\n",  d->attributes & (1<<6) ? "yes" : "no");
        printf("max power (mA):       %d\n",  d->maxPower*2); // 2 mA steps used
      #endif
        textColor(0x0F);
    }
}

void showInterfaceDescriptor(struct usb2_interfaceDescriptor* d)
{
    if (d->length)
    {
        textColor(0x0E);
        printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
        textColor(0x0A);
      #ifdef _USB_DIAGNOSIS_
        printf("length:               %d\t\t", d->length);          // 9
        printf("descriptor type:      %d\n",   d->descriptorType);  // 4
      #endif
        printf("interface number:     %d\t\t", d->interfaceNumber);
      #ifdef _USB_DIAGNOSIS_
        printf("alternate Setting:    %d\n",   d->alternateSetting);
      #endif
        printf("number of endpoints:  %d\n",   d->numEndpoints);
        printf("interface class:      %d\n",   d->interfaceClass);
        printf("interface subclass:   %d\n",   d->interfaceSubclass);
        printf("interface protocol:   %d\n",   d->interfaceProtocol);
      #ifdef _USB_DIAGNOSIS_
        printf("interface:            %x\n",   d->interface);
      #endif
        textColor(0x0F);
    }
}

void showEndpointDescriptor(struct usb2_endpointDescriptor* d)
{
    if (d->length)
    {
        textColor(0x0E);
        printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
        textColor(0x0A);
      #ifdef _USB_DIAGNOSIS_
        printf("length:            %d\t\t",  d->length);         // 7
        printf("descriptor type:   %d\n",    d->descriptorType); // 5
      #endif
        printf("endpoint in/out:   %s\t\t",  d->endpointAddress & 0x80 ? "in" : "out");
        printf("endpoint number:   %d\n",    d->endpointAddress & 0xF);
        printf("attributes:        %y\t\t",  d->attributes); // bit 1:0 00 control    01 isochronous    10 bulk                         11 interrupt
                                                            // bit 3:2 00 no sync    01 async          10 adaptive                     11 sync (only if isochronous)
                                                            // bit 5:4 00 data endp. 01 feedback endp. 10 explicit feedback data endp. 11 reserved (Iso Mode)
        if (d->attributes == 2)
        {
           printf("\nattributes: bulk - no sync - data endpoint\n");
        }
        printf("max packet size:   %d\n",  d->maxPacketSize);
      #ifdef _USB_DIAGNOSIS_
        printf("interval:          %d\n",  d->interval);
      #endif
        textColor(0x0F);
    }
}

void showStringDescriptor(struct usb2_stringDescriptor* d)
{
    if (d->length)
    {
        textColor(0x0A);

      #ifdef _USB_DIAGNOSIS_
        printf("\nlength:            %d\t\t",  d->length);     // 12
        printf("descriptor type:   %d\n",  d->descriptorType); //  3
      #endif

        for(int i=0; i<10;i++)
        {
           if (d->languageID[i])
           {
               if (d->languageID[i] == 0x409)
               {
                   printf("\nlanguage: German\t");
               }
               else
               {
                   printf("\nlanguage: %x\t", d->languageID[i]);
               }
           }
        }
        printf("\n");
        textColor(0x0F);
    }
}

void showStringDescriptorUnicode(struct usb2_stringDescriptorUnicode* d)
{
    if (d->length)
    {
        textColor(0x0A);

      #ifdef _USB_DIAGNOSIS_
        printf("\nlength:            %d\t\t",  d->length);
        printf("descriptor type:   %d\n",  d->descriptorType); // 3
        printf("string: ");
      #endif

        textColor(0x0E);
        for(int i=0; i<(d->length-2);i+=2) // show only low value of Unicode character
        {
            if (d->widechar[i])
            {
                putch(d->widechar[i]);
            }
        }
        printf("\n");
        textColor(0x0F);
    }
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
