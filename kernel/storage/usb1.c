/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb1.h"


uint8_t usb1_TransferEnumerate(uint8_t j)
{
    return (0);
}

void usb1_TransferDevice(uint32_t device)
{
}

void usb1_TransferConfig(uint32_t device)
{
}

void usb1_TransferString(uint32_t device)
{
}

void usb1_TransferStringUnicode(uint32_t device, uint32_t stringIndex)
{
}

void usb1_TransferSetConfiguration(uint32_t device, uint32_t configuration)
{
}

uint8_t usb1_TransferGetConfiguration(uint32_t device)
{
    return (0);
}


void usb1_SetFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
}

void usb1_ClearFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
}

uint16_t usb1_GetStatus(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    return (0);
}

void usb1_addDevice(struct usb1_deviceDescriptor* d, usb1_Device_t* usbDev)
{
}

void usb1_showDevice(usb1_Device_t* usbDev)
{
}

void usb1_showConfigurationDescriptor(struct usb1_configurationDescriptor* d)
{
}

void usb1_showInterfaceDescriptor(struct usb1_interfaceDescriptor* d)
{
}

void usb1_showEndpointDescriptor(struct usb1_endpointDescriptor* d)
{
}

void usb1_showStringDescriptor(struct usb1_stringDescriptor* d)
{
}

void usb1_showStringDescriptorUnicode(struct usb1_stringDescriptorUnicode* d, uint32_t device, uint32_t stringIndex)
{
}


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
