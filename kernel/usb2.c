/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "usb2.h"
#include "console.h"
#include "timer.h"

uint8_t usbTransferEnumerate(uint8_t j)
{
    settextcolor(11,0); printf("\nUSB2: SET_ADDRESS"); settextcolor(15,0);

    uint8_t new_address = j+1; // indicated port number

	void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

	// Create QTDs (in reversed order)
	void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
	SetupQTD   = createQTD_SETUP((uint32_t)next, 0, 8, 0x00, 5, 0, new_address, 0, 0); // SETUP DATA0, 8 byte, ..., SET_ADDRESS, hi, 0...127 (new address), index=0, length=0

    // Create QH 1
    void* QH1 = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    createQH(QH1, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 0, 0, 0);

    // Create QH 2
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, QH1), NULL, 1, 0, 0);

	// Enable Async...
    printf("\nEnable Async Schedule\n");
	pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE;
    sleepSeconds(1);

	return new_address; // new_address
}

void usbTransferDevice(uint32_t device, uint32_t endpoint)
{
    settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR device, dev: %d endpoint: %d", device, endpoint); settextcolor(15,0);

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

	// Create QTDs (in reversed order)
	void* next   = createQTD_IO(           0x1, OUT, 1,  0);    // Handshake is the opposite direction of Data, therefore OUT after IN
	next = InQTD = createQTD_IO((uint32_t)next, IN,  1, 18);    // IN DATA1, 18 byte
    SetupQTD     = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 1, 0, 0, 18); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, device, lo, index, length

    // Create QH 1
    void* QH1 = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    createQH(QH1, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 0, device, endpoint);

    // Create QH 2
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, QH1), NULL, 1, device, endpoint);

    // Enable Async...
    printf("\nReset STS_USBINT and enable Async Schedule\n");
	USBINTflag = false;
	pOpRegs->USBSTS |= STS_USBINT;
    pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE;

	int8_t timeout=80;
	while (!USBINTflag) // set by interrupt
	{
		timeout--;
		if(timeout>0)
		{
		    sleepMilliSeconds(20);
		    printf("#");
		}
		else
		{
			settextcolor(12,0);
			printf("\ntimeout - no STS_USBINT set!");
			settextcolor(15,0);
			break;
		}
	};
    USBINTflag = false;
    pOpRegs->USBSTS |= STS_USBINT;

	showPacket(InQTDpage0,18);
    showDeviceDesriptor((struct usb2_deviceDescriptor*)InQTDpage0);
}

void usbTransferConfig(uint32_t device, uint32_t endpoint)
{
    settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR config, dev: %d endpoint: %d", device, endpoint); settextcolor(15,0);

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

	// Create QTDs (in reversed order)
 	void* next   = createQTD_IO(0x1,            OUT, 1,  0);    // Handshake is the opposite direction of Data, therefore OUT after IN
	next = InQTD = createQTD_IO((uint32_t)next, IN,  1, 32);    // IN DATA1, 32 byte
	SetupQTD     = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 2, 0, 0, 32); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, configuration, lo, index, length

    // Create QH 1
    void* QH1 = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    createQH(QH1, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 0, device, endpoint);

    // Create QH 2
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, QH1), NULL, 1, device, endpoint);

    // Enable Async...
    printf("\nReset STS_USBINT and enable Async Schedule\n");
	USBINTflag = false;
	pOpRegs->USBSTS |= STS_USBINT;
    pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE;

	int8_t timeout=80;
	while (!USBINTflag) // set by interrupt
	{
		timeout--;
		if(timeout>0)
		{
		    sleepMilliSeconds(20);
		    printf("#");
		}
		else
		{
			settextcolor(12,0);
			printf("\ntimeout - no STS_USBINT set!");
			settextcolor(15,0);
			break;
		}
	};
    USBINTflag = false;
    pOpRegs->USBSTS |= STS_USBINT;

 	showPacket(InQTDpage0,32);
    showConfigurationDesriptor((struct usb2_configurationDescriptor*)InQTDpage0);
    showInterfaceDesriptor( (struct usb2_interfaceDescriptor*)((uint8_t*)InQTDpage0 +  9 ));
	showEndpointDesriptor ( (struct usb2_endpointDescriptor*) ((uint8_t*)InQTDpage0 + 18 ));
	showEndpointDesriptor ( (struct usb2_endpointDescriptor*) ((uint8_t*)InQTDpage0 + 25 ));
}

void showDeviceDesriptor(struct usb2_deviceDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
       printf("\nlength:            %d\t\t",  d->length);
       printf("descriptor type:   %d\n",    d->descriptorType);
       printf("USB specification: %d.%d\t\t", d->bcdUSB>>8, d->bcdUSB&0xFF);     // e.g. 0x0210 means 2.10
       printf("USB class:         %x\n",    d->deviceClass);
       printf("USB subclass:      %x\t",    d->deviceSubclass);
       printf("USB protocol       %x\n",    d->deviceProtocol);
       printf("max packet size:   %d\t\t",    d->maxPacketSize);             // MPS0, must be 8,16,32,64
       printf("vendor:            %x\n",    d->idVendor);
       printf("product:           %x\t",    d->idProduct);
       printf("release number:    %d.%d\n", d->bcdDevice>>8, d->bcdDevice&0xFF);  // release of the device
       printf("manufacturer:      %x\t",    d->manufacturer);
       printf("product:           %x\n",    d->product);
       printf("serial number:     %x\t",    d->serialNumber);
       printf("number of config.: %d\n",    d->numConfigurations); // number of possible configurations
       settextcolor(15,0);
    }
}

void showConfigurationDesriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\nlength:               %d\t\t",  d->length);
       printf("descriptor type:      %d\n",  d->descriptorType);
       printf("total length:         %d\t",  d->totalLength);
       printf("number of interfaces: %d\n",  d->numInterfaces);
       printf("ID of config:         %x\t",  d->configurationValue);
       printf("ID of config name     %x\n",  d->configuration);
	   printf("remote wakeup:        %s\t",  d->attributes & (1<<5) ? "yes" : "no");
	   printf("self-powered:         %s\n",  d->attributes & (1<<6) ? "yes" : "no");
       printf("max power (mA):       %d\n",  d->maxPower*2); // 2 mA steps used
       settextcolor(15,0);
    }
}

void showInterfaceDesriptor(struct usb2_interfaceDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\nlength:               %d\t\t",  d->length);        // 9
       printf("descriptor type:      %d\n",  d->descriptorType);    // 4
       printf("interface number:     %d\t\t",  d->interfaceNumber);
       printf("alternate Setting:    %d\n",  d->alternateSetting);
       printf("number of endpoints:  %d\t\t",  d->numEndpoints);
	   printf("interface class:      %d\n",  d->interfaceClass);
	   printf("interface subclass:   %d\t\t",  d->interfaceSubclass);
	   printf("interface protocol:   %d\n",  d->interfaceProtocol);
       printf("interface:            %x\n",  d->interface);
       settextcolor(15,0);
    }
}

void showEndpointDesriptor(struct usb2_endpointDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\nlength:            %d\t\t",  d->length);       // 7
       printf("descriptor type:   %d\n",    d->descriptorType); // 5
	   printf("endpoint in/out:   %s\t\t",  d->endpointAddress & 0x80 ? "in" : "out");
	   printf("endpoint number:   %d\n",    d->endpointAddress & 0xF);
       printf("attributes:        %y\t\t",  d->attributes);     // bit 1:0 00 control       01 isochronous       10 bulk                            11 interrupt
	                                                            // bit 3:2 00 no sync       01 async             10 adaptive                        11 sync (only if isochronous)
	                                                            // bit 5:4 00 data endpoint 01 feedback endpoint 10 explicit feedback data endpoint 11 reserved (Iso Mode)
       printf("max packet size:   %d\n",  d->maxPacketSize);
	   printf("interval:          %d\n",  d->interval);
       settextcolor(15,0);
    }
}

void showStringDesriptor(struct usb2_stringDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\nlength:            %d\t\t",  d->length);     // ?
       printf("descriptor type:   %d\n",  d->descriptorType); // 3
	   for(int i=0; i<10;i++)
	   {
	       printf("lang: %x\t",  d->languageID[i]);
	   }
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
