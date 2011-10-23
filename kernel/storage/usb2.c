/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb2.h"
#include "usb_hc.h"
#include "kheap.h"
#include "paging.h"
#include "video/console.h"
#include "timer.h"
#include "util.h"


uint8_t usbTransferEnumerate(port_t* port, uint8_t num)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: SET_ADDRESS");
    textColor(TEXT);
  #endif

    uint8_t new_address = num; // indicated port number

    usb_transfer_t transfer;
    usb_setupTransfer(port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x00, 5, 0, new_address, 0, 0);
    usb_inTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nnew address: %u", new_address);
    textColor(TEXT);
    waitForKeyStroke();
  #endif

    return new_address;
}

bool usbTransferDevice(usb2_Device_t* device)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: GET_DESCRIPTOR device, dev: %X endpoint: 0", device);
    textColor(TEXT);
  #endif

    struct usb2_deviceDescriptor descriptor;

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x80, 6, 1, 0, 0, 18);
    usb_inTransaction(&transfer, false, &descriptor, 18);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

    addDevice(&descriptor, device);
    showDevice(device);

    return (transfer.success);
}

void usbTransferConfig(usb2_Device_t* device)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: GET_DESCRIPTOR config, dev: %X endpoint: 0", device);
    textColor(TEXT);
  #endif

    char buffer[32];

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x80, 6, 2, 0, 0, 32);
    usb_inTransaction(&transfer, false, buffer, 32);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(LIGHT_GRAY);
    printf("\n---------------------------------------------------------------------\n");
    textColor(GREEN);
  #endif

    // parse to config (len=9,type=2), interface (len=9,type=4) or endpoint (len=7,type=5)
    void* addr     = buffer;
    void* lastByte = addr + (*(uint16_t*)(addr+2)); // totalLength (WORD)

  #ifdef _USB2_DIAGNOSIS_
    memshow(buffer, *(uint16_t*)(addr+2), false);
    putch('\n');
  #endif

    while ((uintptr_t)addr < (uintptr_t)lastByte)
    {
        uint8_t type =  *(uint8_t*)(addr+1);
        uint8_t length = *(uint8_t*)addr;

        if (length == 9 && type == 2)
        {
            struct usb2_configurationDescriptor* descriptor = addr;
            showConfigurationDescriptor(descriptor);
        }
        else if (length == 9 && type == 4)
        {
            struct usb2_interfaceDescriptor* descriptor = addr;
            showInterfaceDescriptor(descriptor);

            if (descriptor->interfaceClass == 8)
            {
                // store interface number for mass storage transfers
                device->numInterfaceMSD   = descriptor->interfaceNumber;
                device->InterfaceClass    = descriptor->interfaceClass;
                device->InterfaceSubclass = descriptor->interfaceSubclass;
            }
        }
        else if (length == 7 && type == 5)
        {
            struct usb2_endpointDescriptor* descriptor = addr;
            showEndpointDescriptor(descriptor);

            if((descriptor->endpointAddress & 0xF) < 3)
                device->endpoints[descriptor->endpointAddress & 0xF].mps = descriptor->maxPacketSize;

            // store endpoint numbers for IN/OUT mass storage transfers, attributes must be 0x2, because there are also endpoints with attributes 0x3(interrupt)
            if (descriptor->endpointAddress & 0x80 && descriptor->attributes == 0x2)
            {
                device->numEndpointInMSD = descriptor->endpointAddress & 0xF;
            }

            if (!(descriptor->endpointAddress & 0x80) && descriptor->attributes == 0x2)
            {
                device->numEndpointOutMSD = descriptor->endpointAddress & 0xF;
            }
        }
        else
        {
          #ifdef _USB_TRANSFER_DIAGNOSIS_
            printf("\nlength: %u type: %u - unknown\n", length, type);
          #endif
        }

        addr += length;
    }
}

void usbTransferString(usb2_Device_t* device)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: GET_DESCRIPTOR string, dev: %X endpoint: 0 languageIDs", device);
    textColor(TEXT);
  #endif

    struct usb2_stringDescriptor descriptor;

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x80, 6, 3, 0, 0, 12);
    usb_inTransaction(&transfer, false, &descriptor, 12);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB_TRANSFER_DIAGNOSIS_
    memshow(&descriptor, 12, false);
    putch('\n');
  #endif
    showStringDescriptor(&descriptor);
}

void usbTransferStringUnicode(usb2_Device_t* device, uint32_t stringIndex)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: GET_DESCRIPTOR string, dev: %X endpoint: 0 stringIndex: %u", device, stringIndex);
    textColor(TEXT);
  #endif

    char buffer[64];

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x80, 6, 3, stringIndex, 0x0409, 64);
    usb_inTransaction(&transfer, false, buffer, 64);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB_TRANSFER_DIAGNOSIS_
    memshow(buffer, 64, false);
    putch('\n');
  #endif

    showStringDescriptorUnicode((struct usb2_stringDescriptorUnicode*)buffer, device, stringIndex);
}

// http://www.lowlevel.eu/wiki/USB#SET_CONFIGURATION
void usbTransferSetConfiguration(usb2_Device_t* device, uint32_t configuration)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\n\nUSB2: SET_CONFIGURATION %u", configuration);
    textColor(TEXT);
  #endif

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x00, 9, 0, configuration, 0, 0); // SETUP DATA0, 8 byte, request type, SET_CONFIGURATION(9), hi(reserved), configuration, index=0, length=0
    usb_inTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);
}

uint8_t usbTransferGetConfiguration(usb2_Device_t* device)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: GET_CONFIGURATION");
    textColor(TEXT);
  #endif

    char configuration;

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);
    usb_setupTransaction(&transfer, 8, 0x80, 8, 0, 0, 0, 1);
    usb_inTransaction(&transfer, false, &configuration, 1);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

    return configuration;
}

// new control transfer as TEST /////////////////////////////////////////////////
// seems not to work correct, does not set HALT ???
void usbSetFeatureHALT(usb2_Device_t* device, uint32_t endpoint)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: usbSetFeatureHALT, endpoint: %u", endpoint);
    textColor(TEXT);
  #endif

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, endpoint, 64);
    usb_setupTransaction(&transfer, 8, 0x02, 3, 0, 0, endpoint, 0);
    usb_inTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB_TRANSFER_DIAGNOSIS_
    printf("\nset HALT at dev: %X endpoint: %u", device, endpoint);
  #endif
}

void usbClearFeatureHALT(usb2_Device_t* device, uint32_t endpoint)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\n\nUSB2: usbClearFeatureHALT, endpoint: %u", endpoint);
    textColor(TEXT);
  #endif

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, endpoint, 64);
    usb_setupTransaction(&transfer, 8, 0x02, 1, 0, 0, endpoint, 0);
    usb_inTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

  #ifdef _USB2_DIAGNOSIS_
    printf("\nclear HALT at dev: %X endpoint: %u", device, endpoint);
  #endif
}

uint16_t usbGetStatus(usb2_Device_t* device, uint32_t endpoint)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    textColor(YELLOW);
    printf("\n\nUSB2: usbGetStatus at device: %X endpoint: %u", device, endpoint);
    textColor(TEXT);
  #endif

    uint16_t status;

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, endpoint, 64);
    usb_setupTransaction(&transfer, 8, 0x02, 0, 0, 0, endpoint, 2);
    usb_inTransaction(&transfer, false, &status, 2);
    usb_outTransaction(&transfer, true, 0, 0);
    usb_issueTransfer(&transfer);

    return status;
}

void addDevice(struct usb2_deviceDescriptor* d, usb2_Device_t* usbDev)
{
    usbDev->usbSpec               = d->bcdUSB;
    usbDev->usbClass              = d->deviceClass;
    usbDev->usbSubclass           = d->deviceSubclass;
    usbDev->usbProtocol           = d->deviceProtocol;
    usbDev->vendor                = d->idVendor;
    usbDev->product               = d->idProduct;
    usbDev->releaseNumber         = d->bcdDevice;
    usbDev->manufacturerStringID  = d->manufacturer;
    usbDev->productStringID       = d->product;
    usbDev->serNumberStringID     = d->serialNumber;
    usbDev->numConfigurations     = d->numConfigurations;
    usbDev->endpoints[0].mps      = d->maxPacketSize;
}

void showDevice(usb2_Device_t* usbDev)
{
    textColor(IMPORTANT);
    if (usbDev->usbSpec == 0x0100 || usbDev->usbSpec == 0x0110 || usbDev->usbSpec == 0x0200 || usbDev->usbSpec == 0x0300)
    {
        textColor(SUCCESS);
        printf("\nUSB %y.%y\t", BYTE2(usbDev->usbSpec), BYTE1(usbDev->usbSpec)); // e.g. 0x0210 means 2.10
        textColor(TEXT);
    }
    else
    {
        textColor(ERROR);
        printf("\nInvalid USB version %u.%u !", BYTE2(usbDev->usbSpec), BYTE1(usbDev->usbSpec));
        textColor(TEXT);
        return;
    }

    if (usbDev->usbClass == 0x09)
    {
        switch (usbDev->usbProtocol)
        {
            case 0:
                printf("Full speed USB hub");
                break;
            case 1:
                printf("Hi-speed USB hub with single TT");
                break;
            case 2:
                printf("Hi-speed USB hub with multiple TTs");
                break;
        }
    }

    printf("\nendpoint 0 mps: %u byte.", usbDev->endpoints[0].mps); // MPS0, must be 8,16,32,64
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    printf("vendor:            %xh\n",   usbDev->vendor);
    printf("product:           %xh\t",   usbDev->product);
    printf("release number:    %u.%u\n", BYTE2(usbDev->releaseNumber), BYTE1(usbDev->releaseNumber));
    printf("manufacturer:      %xh\t",   usbDev->manufacturerStringID);
    printf("product:           %xh\n",   usbDev->productStringID);
    printf("serial number:     %xh\t",   usbDev->serNumberStringID);
    printf("number of config.: %u\n",    usbDev->numConfigurations); // number of possible configurations
    printf("numInterfaceMSD:   %u\n",    usbDev->numInterfaceMSD);
  #endif
    textColor(TEXT);
}

void showConfigurationDescriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        textColor(IMPORTANT);
        printf("length:               %u\t\t", d->length);
        printf("descriptor type:      %u\n", d->descriptorType);
        textColor(LIGHT_GRAY);
        printf("total length:         %u\t", d->totalLength);
      #endif
        textColor(IMPORTANT);
        printf("\nNumber of interfaces: %u", d->numInterfaces);
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        printf("ID of config:         %xh\t", d->configurationValue);
        printf("ID of config name     %xh\n", d->configuration);
        printf("remote wakeup:        %s\t", d->attributes & BIT(5) ? "yes" : "no");
        printf("self-powered:         %s\n", d->attributes & BIT(6) ? "yes" : "no");
        printf("max power (mA):       %u\n", d->maxPower*2); // 2 mA steps used
      #endif
        textColor(TEXT);
    }
}

void showInterfaceDescriptor(struct usb2_interfaceDescriptor* d)
{
    if (d->length)
    {
        putch('\n');
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        textColor(LIGHT_GRAY);
        printf("---------------------------------------------------------------------\n");
      #endif
        textColor(IMPORTANT);
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        printf("length:               %u\t\t", d->length);          // 9
        printf("descriptor type:      %u\n",   d->descriptorType);  // 4
      #endif
        switch (d->numEndpoints)
        {
            case 0:
                printf("Interface %u has no endpoint and belongs to class:\n", d->interfaceNumber);
                break;
            case 1:
                printf("Interface %u has only one endpoint and belongs to class:\n", d->interfaceNumber);
                break;
            default:
                printf("Interface %u has %u endpoints and belongs to class:\n", d->interfaceNumber, d->numEndpoints);
                break;
        }

        switch (d->interfaceClass)
        {
            case 0x01:
                printf("Audio");
                break;
            case 0x02:
                printf("Communications and CDC Control");
                break;
            case 0x03:
                printf("HID (Human Interface Device)");
                break;
            case 0x05:
                printf("Physical");
                break;
            case 0x06:
                printf("Image");
                break;
            case 0x07:
                printf("Printer");
                break;
            case 0x08:
                printf("Mass Storage, ");
                switch (d->interfaceSubclass)
                {
                    case 0x01:
                        printf("Reduced Block Commands, ");
                        break;
                    case 0x02:
                        printf("SFF-8020i or MMC-2(ATAPI), ");
                        break;
                    case 0x03:
                        printf("QIC-157 (tape device), ");
                        break;
                    case 0x04:
                        printf("UFI (e.g. Floppy Disk), ");
                        break;
                    case 0x05:
                        printf("SFF-8070i (e.g. Floppy Disk), ");
                        break;
                    case 0x06:
                        printf("SCSI transparent command set, ");
                        break;
                }
                switch (d->interfaceProtocol)
                {
                    case 0x00:
                        printf("CBI protocol with command completion interrupt.");
                        break;
                    case 0x01:
                        printf("CBI protocol without command completion interrupt.");
                        break;
                    case 0x50:
                        printf("Bulk-Only Transport protocol.");
                        break;
                }
                break;
            case 0x0A:
                printf("CDC-Data");
                break;
            case 0x0B:
                printf("Smart Card");
                break;
            case 0x0D:
                printf("Content Security");
                break;
            case 0x0E:
                printf("Video");
                break;
            case 0x0F:
                printf("Personal Healthcare");
                break;
            case 0xDC:
                printf("Diagnostic Device");
                break;
            case 0xE0:
                printf("Wireless Controller, subclass: %yh protocol: %yh.",d->interfaceSubclass,d->interfaceProtocol);
                break;
            case 0xEF:
                printf("Miscellaneous");
                break;
            case 0xFE:
                printf("Application Specific");
                break;
            case 0xFF:
                printf("Vendor Specific");
                break;
        }

      #ifdef _USB_TRANSFER_DIAGNOSIS_
        printf("\nalternate Setting:  %u\n",  d->alternateSetting);
        printf("interface class:      %u\n",  d->interfaceClass);
        printf("interface subclass:   %u\n",  d->interfaceSubclass);
        printf("interface protocol:   %u\n",  d->interfaceProtocol);
        printf("interface:            %xh\n", d->interface);
      #endif

        textColor(TEXT);
    }
}

void showEndpointDescriptor(struct usb2_endpointDescriptor* d)
{
  #ifdef _USB_TRANSFER_DIAGNOSIS_
    if (d->length)
    {
        textColor(LIGHT_GRAY);
        printf("\n---------------------------------------------------------------------\n");
        textColor(IMPORTANT);
        printf("length:      %u\t\t",   d->length);         // 7
        printf("descriptor type: %u\n", d->descriptorType); // 5
        printf("endpoint %u: %s, ",     d->endpointAddress & 0xF, d->endpointAddress & 0x80 ? "IN " : "OUT");
        printf("attributes: %yh\t\t",   d->attributes);
        // bit 1:0 00 control    01 isochronous    10 bulk                         11 interrupt
        // bit 3:2 00 no sync    01 async          10 adaptive                     11 sync (only if isochronous)
        // bit 5:4 00 data endp. 01 feedback endp. 10 explicit feedback data endp. 11 reserved (Iso Mode)

        if (d->attributes == 2)
        {
           printf(" bulk data,");
        }
        printf(" mps: %u byte",  d->maxPacketSize);
        printf("interval:          %u\n",  d->interval);
        textColor(TEXT);
    }
  #endif
}

void showStringDescriptor(struct usb2_stringDescriptor* d)
{
    if (d->length)
    {
        textColor(IMPORTANT);

      #ifdef _USB_TRANSFER_DIAGNOSIS_
        printf("\nlength:          %u\t", d->length);         // 12
        printf("\tdescriptor type: %u\n", d->descriptorType); //  3
      #endif

        printf("\n\nlanguages: ");
        for (uint8_t i=0; i<10;i++)
        {
            if (d->languageID[i] >= 0x0400 && d->languageID[i] <= 0x0465)
            {
                switch (d->languageID[i])
                {
                    case 0x401:
                        printf("Arabic");
                        break;
                    case 0x404:
                        printf("Chinese");
                        break;
                    case 0x407:
                        printf("German");
                        break;
                    case 0x409:
                        printf("English");
                        break;
                    case 0x40A:
                        printf("Spanish");
                        break;
                    case 0x40C:
                        printf("French");
                        break;
                    case 0x410:
                        printf("Italian");
                        break;
                    case 0x411:
                        printf("Japanese");
                        break;
                    case 0x416:
                        printf("Portuguese");
                        break;
                    case 0x419:
                        printf("Russian");
                        break;
                    default:
                        printf("language code: %xh", d->languageID[i]);
                        /*Language Codes
                        ; 0x400 Neutral
                        ; 0x401 Arabic
                        ; 0x402 Bulgarian
                        ; 0x403 Catalan
                        ; 0x404 Chinese
                        ; 0x405 Czech
                        ; 0x406 Danish
                        ; 0x407 German
                        ; 0x408 Greek
                        ; 0x409 English
                        ; 0x40a Spanish
                        ; 0x40b Finnish
                        ; 0x40c French
                        ; 0x40d Hebrew
                        ; 0x40e Hungarian
                        ; 0x40f Icelandic
                        ; 0x410 Italian
                        ; 0x411 Japanese
                        ; 0x412 Korean
                        ; 0x413 Dutch
                        ; 0x414 Norwegian
                        ; 0x415 Polish
                        ; 0x416 Portuguese
                        ; 0x418 Romanian
                        ; 0x419 Russian
                        ; 0x41a Croatian
                        ; 0x41a Serbian
                        ; 0x41b Slovak
                        ; 0x41c Albanian
                        ; 0x41d Swedish
                        ; 0x41e Thai
                        ; 0x41f Turkish
                        ; 0x420 Urdu
                        ; 0x421 Indonesian
                        ; 0x422 Ukrainian
                        ; 0x423 Belarusian
                        ; 0x424 Slovenian
                        ; 0x425 Estonian
                        ; 0x426 Latvian
                        ; 0x427 Lithuanian
                        ; 0x429 Farsi
                        ; 0x42a Vietnamese
                        ; 0x42b Armenian
                        ; 0x42c Azeri
                        ; 0x42d Basque
                        ; 0x42f Macedonian
                        ; 0x436 Afrikaans
                        ; 0x437 Georgian
                        ; 0x438 Faeroese
                        ; 0x439 Hindi
                        ; 0x43e Malay
                        ; 0x43f Kazak
                        ; 0x440 Kyrgyz
                        ; 0x441 Swahili
                        ; 0x443 Uzbek
                        ; 0x444 Tatar
                        ; 0x446 Punjabi
                        ; 0x447 Gujarati
                        ; 0x449 Tamil
                        ; 0x44a Telugu
                        ; 0x44b Kannada
                        ; 0x44e Marathi
                        ; 0x44f Sanskrit
                        ; 0x450 Mongolian
                        ; 0x456 Galician
                        ; 0x457 Konkani
                        ; 0x45a Syriac
                        ; 0x465 Divehi */
                        break;
                }
            }
        }
        printf("\n");
        textColor(TEXT);
    }
}

void showStringDescriptorUnicode(struct usb2_stringDescriptorUnicode* d, usb2_Device_t* device, uint32_t stringIndex)
{
    if (d->length)
    {
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        textColor(IMPORTANT);
        printf("\nlength:          %u\t", d->length);
        printf("\tdescriptor type: %u", d->descriptorType);
        printf("\nstring:          ");
        textColor(DATA);
      #endif
        char asciichar[15];
        memset(asciichar, 0, 15);

        for (uint8_t i=0; i<min(30, (d->length-2)); i+=2) // show only low value of Unicode character
        {
            if (d->widechar[i])
            {
              #ifdef _USB_TRANSFER_DIAGNOSIS_
                putch(d->widechar[i]);
              #endif
                asciichar[i/2] = d->widechar[i];
            }
        }
      #ifdef _USB_TRANSFER_DIAGNOSIS_
        printf("\t");
        textColor(TEXT);
      #endif

        if (stringIndex == 2) // product name
        {
            strncpy(device->productName, asciichar, 15);

          #ifdef _USB_TRANSFER_DIAGNOSIS_
            printf(" product name: %s", device->productName);
          #endif
        }
        else if (stringIndex == 3) // serial number
        {
            // take the last 12 characters:

            int16_t last = strlen(asciichar); // store last position
            int16_t j = max(last-12, 0); // step 12 characters backwards, but not below zero

            for (uint16_t index=0; index<13; index++)
            {
                if (j+index>last)
                {
                    device->serialNumber[index] = 0;
                    break;
                }
                else
                {
                    device->serialNumber[index] = asciichar[j+index];
                }
            }
          #ifdef _USB_TRANSFER_DIAGNOSIS_
            printf(" serial: %s", device->serialNumber);
          #endif
        }
    }
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
