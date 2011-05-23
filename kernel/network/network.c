/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "network.h"
#include "rtl8139.h"
#include "rtl8168.h"
#include "pcnet.h"
#include "kheap.h"
#include "irq.h"
#include "util.h"
#include "video/console.h"
#include "netprotocol/ethernet.h"
#include "netprotocol/dhcp.h"


typedef enum {
    RTL8139, RTL8168, PCNET, ND_END
} network_drivers;

static network_driver_t drivers[ND_END] =
{
    {.install = &install_RTL8139, .interruptHandler = &rtl8139_handler, .sendPacket = &rtl8139_send},
    {.install = &install_RTL8168, .interruptHandler = &rtl8168_handler, .sendPacket = 0},
    {.install = &install_AMDPCnet, .interruptHandler = &PCNet_handler, .sendPacket = &PCNet_send}
};


bool network_installDevice(pciDev_t* device)
{
    textColor(0x0C);

    network_driver_t* driver = 0;
    if(device->deviceID == 0x8139 && device->vendorID == 0x10EC) // RTL 8139
    {
        driver = &drivers[RTL8139];
        printf("\nRealtek RTL8139");
    }
    else if (device->deviceID == 0x8168 && device->vendorID == 0x10EC) // RTL 8111b/8168
    {
        driver = &drivers[RTL8168];
        printf("\nRealtek RTL8168");
    }
    else if (device->deviceID == 0x2000 && device->vendorID == 0x1022) // AMD PCNet III (Am79C973)
    {
        driver = &drivers[PCNET];
        printf("\nAMD PCnet III");
    }

    if(driver == 0 || driver->install == 0) // PrettyOS does not know the card or the driver is not properly installed
    {
        textColor(0x0F);
        return(false);
    }

    printf(" network adapter:");

    // PrettyOS has a driver for this adapter. Install it.
    network_adapter_t* adapter = malloc(sizeof(network_adapter_t), 0, "network apdapter");
    adapter->driver = driver;
    adapter->PCIdev = device;

    // Set IRQ handler
    irq_installHandler(device->irq, driver->interruptHandler);

    // Detect MMIO and IO space
    uint16_t pciCommandRegister = pci_config_read(device->bus, device->device, device->func, 0x0204);
    pci_config_write_dword(device->bus, device->device, device->func, 0x04, pciCommandRegister /*already set*/ | BIT(2) /*bus master*/); // resets status register, sets command register

    for (uint8_t j=0;j<6;++j) // check network card BARs
    {
        device->bar[j].memoryType = device->bar[j].baseAddress & 0x01;

        if (device->bar[j].baseAddress) // check valid BAR
        {
            if (device->bar[j].memoryType == 0)
            {
                adapter->MMIO_base = (void*)(device->bar[j].baseAddress &= 0xFFFFFFF0);
            }
            else if (device->bar[j].memoryType == 1)
            {
                adapter->IO_base = device->bar[j].baseAddress &= 0xFFFC;
            }
        }
    }

    textColor(0x0F);
    
    // Input makes #PF at some computers (gets is source of error #PF)
    /*
    printf("\nPlease type in your IP address: ");
    char temp[30];
    memset(temp, 0, 30);
    gets(temp);
    for(uint8_t i_start = 0, i_end = 0, byte = 0; i_end < 30 && byte < 4; i_end++) 
    {
        if(temp[i_end] == 0)
        {
            adapter->IP_address[byte] = atoi(temp+i_start);
            break;
        }
        if(temp[i_end] == '.')
        {
            temp[i_end] = 0;
            adapter->IP_address[byte] = atoi(temp+i_start);
            i_start = i_end+1;
            byte++;
        }
    }
    */
    // Workaround: TODO
    adapter->IP_address[0] =  192;
    adapter->IP_address[1] =  168;
    adapter->IP_address[2] =   10;
    adapter->IP_address[3] =   97;
    // ------------------------------

    adapter->driver->install(adapter);

    // Try to get an IP by DHCP
    DHCP_Discover(adapter);

    textColor(0x0E);
    printf("\nMAC address: %y-%y-%y-%y-%y-%y", adapter->MAC_address[0], adapter->MAC_address[1], adapter->MAC_address[2],
                                               adapter->MAC_address[3], adapter->MAC_address[4], adapter->MAC_address[5]);

    printf("\t\tIP address: %u.%u.%u.%u\n", adapter->IP_address[0], adapter->IP_address[1], adapter->IP_address[2], adapter->IP_address[3]);
    textColor(0x0F);

    return(true);
}

bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length)
{
    return(adapter->driver->sendPacket != 0 && adapter->driver->sendPacket(adapter->data, buffer, length));
}

void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length) // Called by driver
{
    uint32_t ethernetType = (buffer[16] << 8) + buffer[17]; // Big Endian

    // output receiving buffer
    textColor(0x0D);
    printf("\nFlags: ");
    textColor(0x03);
    for (uint8_t i = 0; i < 2; i++)
    {
        printf("%y ", buffer[i]);
    }

    textColor(0x0D); printf("\tLength: ");
    textColor(0x03); printf("%d", length);

    textColor(0x0D); printf("\nMAC Receiver: "); textColor(0x03);
    for (uint8_t i = 4; i < 10; i++)
    {
        printf("%y ", buffer[i]);
    }

    textColor(0x0D); printf("MAC Transmitter: "); textColor(0x03);
    for (uint8_t i = 10; i < 16; i++)
    {
        printf("%y ", buffer[i]);
    }

    textColor(0x0D);
    printf("\nEthernet: ");

    textColor(0x03);
    if (ethernetType <= 1500) { printf("type 1, "); }
    else                      { printf("type 2, "); }

    textColor(0x0D);
    if (ethernetType <= 1500) { printf("Length: "); }
    else                      { printf("Type: ");   }

    textColor(0x03);
    for (uint8_t i = 16; i < 18; i++)
    {
        printf("%y ", buffer[i]);
    }

    uint32_t printlength = max(length, 80);
    printf("\n");

    for (uint32_t i = 18; i <= printlength; i++)
    {
        printf("%y ", buffer[i]);
    }
    textColor(0x0F);
    printf("\n");
    EthernetRecv(adapter, (void*)(&buffer[4]), length - 4);
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
