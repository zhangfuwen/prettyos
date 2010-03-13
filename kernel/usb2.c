/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "sys_speaker.h"
#include "usb2.h"

void showDeviceDesriptor(struct usb2_deviceDescriptor* d)
{
   settextcolor(10,0);
   printformat("\nlength:            %d\n",  d->length);
   printformat("descriptor type:   %d\n",    d->descriptorType);
   printformat("USB specification: %d.%d\n", d->bcdUSB>>8, d->bcdUSB&0xFF);     // e.g. 0x0210 means 2.10
   printformat("USB class:         %x\n",    d->deviceClass);
   printformat("USB subclass:      %x\n",    d->deviceSubclass);
   printformat("USB protocol       %x\n",    d->deviceProtocol);
   printformat("max packet size:   %d\n",    d->maxPacketSize);             // MPS0, must be 8,16,32,64
   printformat("vendor:            %x\n",    d->idVendor);
   printformat("product:           %x\n",    d->idProduct);
   printformat("release number:    %d.%d\n", d->bcdDevice>>8, d->bcdDevice);   // release of the device
   printformat("manufacturer:      %x\n",    d->manufacturer);
   printformat("product:           %x\n",    d->product);
   printformat("serial number:     %x\n",    d->serialNumber);
   printformat("number of config.: %d\n",    d->bumConfigurations); // number of possible configurations
   settextcolor(15,0);
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