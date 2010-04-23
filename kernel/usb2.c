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

usb2_Device_t usbDevices[16];

static void performAsyncScheduler()
{
	// Enable Async...
    USBINTflag = false;
    pOpRegs->USBSTS |= STS_USBINT;
    pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE;

    int8_t timeout=40;
    while (!USBINTflag) // set by interrupt
    {
        timeout--;
        if(timeout>0)
        {
            sleepMilliSeconds(20);
            //printf("#");
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
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
	
	sleepMilliSeconds(100);
}

uint8_t usbTransferEnumerate(uint8_t j)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: SET_ADDRESS"); settextcolor(15,0);
    #endif

    uint8_t new_address = j+1; // indicated port number

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
    SetupQTD   = createQTD_SETUP((uint32_t)next, 0, 8, 0x00, 5, 0, new_address, 0, 0); // SETUP DATA0, 8 byte, ..., SET_ADDRESS, hi, 0...127 (new address), index=0, length=0

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, 0, 0);

	performAsyncScheduler();
    
	return new_address; // new_address
}

void usbTransferDevice(uint32_t device, uint32_t endpoint)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR device, dev: %d endpoint: %d", device, endpoint); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(             0x1, OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uint32_t)next, IN,  1, 18);  // IN DATA1, 18 byte
    SetupQTD = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 1, 0, 0, 18); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, device, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, endpoint);

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");

	// showPacket(DataQTDpage0,18);
    addDevice ( (struct usb2_deviceDescriptor*)DataQTDpage0, &usbDevices[device] );
    showDevice( &usbDevices[device] );
}

void usbTransferConfig(uint32_t device, uint32_t endpoint)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR config, dev: %d endpoint: %d", device, endpoint); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,              OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uint32_t)next, IN,  1, PAGESIZE);  // IN DATA1, 4096 byte
    SetupQTD = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 2, 0, 0, PAGESIZE); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, configuration, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, endpoint);

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
	
    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,32);
    #endif
	
    // parsen auf config (len=9,type=2), interface (len=9,type=4), endpoint (len=7,type=5)
	uintptr_t addrPointer = (uintptr_t)DataQTDpage0;
    uintptr_t lastByte    = addrPointer + (*(uint16_t*)(addrPointer+2)); // totalLength (WORD)
	// printf("\nlastByte: %X\n",lastByte); // test
	
	while(addrPointer<lastByte)
	{
		bool found = false;
		// printf("addrPointer: %X\n",addrPointer); // test
		if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 2) ) // length, type
		{
			showConfigurationDesriptor((struct usb2_configurationDescriptor*)addrPointer);
			addrPointer += 9;
			found = true;
		}
	    
		if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 4) ) // length, type
		{
			showInterfaceDesriptor((struct usb2_interfaceDescriptor*)addrPointer);
			addrPointer += 9;
			found = true;
		}

		if ( ((*(uint8_t*)addrPointer) == 7) && ((*(uint8_t*)(addrPointer+1)) == 5) ) // length, type
		{
			showEndpointDesriptor ((struct usb2_endpointDescriptor*)addrPointer);
			addrPointer += 7;
			found = true;
		} 
		
		if ( ((*(uint8_t*)(addrPointer+1)) != 2 ) && ((*(uint8_t*)(addrPointer+1)) != 4 ) && ((*(uint8_t*)(addrPointer+1)) != 5 ) ) // length, type
		{
			settextcolor(9,0);
			printf("\nlength: %d type: %d unknown\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
			settextcolor(15,0);
			addrPointer += *(uint8_t*)addrPointer;
			found = true;
		} 
		
		if (found == false)
		{
			printf("\nlength: %d type: %d not found\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
			break;
		}
		
		settextcolor(13,0);
        printf("\n>>> Press key to go on with data analysis from config descriptor. <<<");
        settextcolor(15,0);
        while(!checkKQ_and_return_char());
        printf("\n");
	}
}

void usbTransferString(uint32_t device, uint32_t endpoint)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: %d languageIDs", device, endpoint); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,              OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uint32_t)next, IN,  1, 12);  // IN DATA1, 12 byte
    SetupQTD = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 3, 0, 0, 12); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, endpoint);

    performAsyncScheduler();
		
    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,12);
    #endif
	showStringDesriptor((struct usb2_stringDescriptor*)DataQTDpage0);
}

void usbTransferStringUnicode(uint32_t device, uint32_t endpoint, uint32_t stringIndex)
{
	#ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: %d stringIndex: %d", device, endpoint,stringIndex); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,              OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uint32_t)next, IN,  1, 64);  // IN DATA1, 64 byte
    SetupQTD = createQTD_SETUP((uint32_t)next, 0, 8, 0x80, 6, 3, stringIndex, 0x0409, 64); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, stringIndex, languageID, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, endpoint);

    performAsyncScheduler();
    	
    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,64);
    #endif

	showStringDesriptorUnicode((struct usb2_stringDescriptorUnicode*)DataQTDpage0);
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
       settextcolor(10,0);
       printf("\nUSB specification: %d.%d\t\t", usbDev->usbSpec>>8, usbDev->usbSpec&0xFF);     // e.g. 0x0210 means 2.10
       printf("USB class:         %x\n",    usbDev->usbClass);
       printf("USB subclass:      %x\t",    usbDev->usbSubclass);
       printf("USB protocol       %x\n",    usbDev->usbProtocol);
       printf("max packet size:   %d\t\t",    usbDev->maxPacketSize);             // MPS0, must be 8,16,32,64
       printf("vendor:            %x\n",    usbDev->vendor);
       printf("product:           %x\t",    usbDev->product);
       printf("release number:    %d.%d\n", usbDev->releaseNumber>>8, usbDev->releaseNumber&0xFF);  // release of the device
       printf("manufacturer:      %x\t",    usbDev->manufacturerStringID);
       printf("product:           %x\n",    usbDev->productStringID);
       printf("serial number:     %x\t",    usbDev->serNumberStringID);
       printf("number of config.: %d\n",    usbDev->numConfigurations); // number of possible configurations
       settextcolor(15,0);
}

void showConfigurationDesriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\n");
       #ifdef _USB_DIAGNOSIS_
	   printf("length:               %d\t\t",  d->length);
       printf("descriptor type:      %d\n",  d->descriptorType);
       #endif
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
       settextcolor(14,0);
	   printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
	   settextcolor(10,0);
       #ifdef _USB_DIAGNOSIS_
	   printf("length:               %d\t\t",  d->length);        // 9
       printf("descriptor type:      %d\n",  d->descriptorType);    // 4
       #endif
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
       settextcolor(14,0);
	   printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
	   settextcolor(10,0);
       #ifdef _USB_DIAGNOSIS_
	   printf("length:            %d\t\t",  d->length);       // 7
       printf("descriptor type:   %d\n",    d->descriptorType); // 5
       #endif
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
       settextcolor(15,0);
    }
}

void showStringDesriptorUnicode(struct usb2_stringDescriptorUnicode* d)
{
    if (d->length)
    {
       settextcolor(10,0);
       
       #ifdef _USB_DIAGNOSIS_
	     printf("\nlength:            %d\t\t",  d->length);     
         printf("descriptor type:   %d\n",  d->descriptorType); // 3
         printf("string: ");
	   #endif
	   
	   settextcolor(14,0);
	   for(int i=0; i<(d->length-2);i+=2) // show only low value of Unicode character
       {
		   if (d->widechar[i])
		   {
               putch(d->widechar[i]);
		   }
       }
	   printf("\n\n\n\n"); // can be deleted by "Port: ..., device attached" <--- TODO
       settextcolor(15,0);
    }
}

/*
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
*/


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
