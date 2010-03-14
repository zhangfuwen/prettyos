/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "cdi/pci.h"


void cdi_pci_get_all_devices(cdi_list_t list);

void cdi_pci_device_destroy(struct cdi_pci_device* device);

void cdi_pci_alloc_ioports(struct cdi_pci_device* device);

void cdi_pci_free_ioports(struct cdi_pci_device* device);

void cdi_pci_alloc_memory(struct cdi_pci_device* device);

void cdi_pci_free_memory(struct cdi_pci_device* device);

uint8_t cdi_pci_config_readb(struct cdi_pci_device* device, uint8_t offset);

uint16_t cdi_pci_config_readw(struct cdi_pci_device* device, uint8_t offset);

uint32_t cdi_pci_config_readl(struct cdi_pci_device* device, uint8_t offset);

void cdi_pci_config_writeb(struct cdi_pci_device* device, uint8_t offset, uint8_t value);

void cdi_pci_config_writew(struct cdi_pci_device* device, uint8_t offset, uint16_t value);

void cdi_pci_config_writel(struct cdi_pci_device* device, uint8_t offset, uint32_t value);

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
