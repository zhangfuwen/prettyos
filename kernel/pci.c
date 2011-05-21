/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "pci.h"
#include "list.h"
#include "storage/usb_hc.h"
#include "network/rtl8139.h"
#include "network/pcnet.h"
#include "video/console.h"
#include "kheap.h"

#ifdef _PCI_VEND_PROD_LIST_
  #include "pciVendProdList.h" // http://www.pcidatabase.com/pci_c_header.php
#endif


static pciDev_t* pciDev_Array[PCIARRAYSIZE];

void analyzeHostSystemError(pciDev_t* pciDev)
{
     // check pci status register of the device
     uint32_t pciStatus = pci_config_read(pciDev->bus, pciDev->device, pciDev->func, PCI_STATUS);

     printf("\nPCI status word: %x\n",pciStatus);
     textColor(0x03);
     // bits 0...2 reserved
     if(pciStatus & BIT(3))  printf("Interrupt Status\n");
     if(pciStatus & BIT(4))  printf("Capabilities List\n");
     if(pciStatus & BIT(5))  printf("66 MHz Capable\n");
     // bit 6 reserved
     if(pciStatus & BIT(7))  printf("Fast Back-to-Back Transactions Capable\n");
     textColor(0x0C);
     if(pciStatus & BIT(8))  printf("Master Data Parity Error\n");
     // DEVSEL Timing: bits 10:9
     if(pciStatus & BIT(11)) printf("Signalled Target-Abort\n");
     if(pciStatus & BIT(12)) printf("Received Target-Abort\n");
     if(pciStatus & BIT(13)) printf("Received Master-Abort\n");
     if(pciStatus & BIT(14)) printf("Signalled System Error\n");
     if(pciStatus & BIT(15)) printf("Detected Parity Error\n");
     textColor(0x0F);
}

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint16_t content)
{
    // example: PCI_VENDOR_ID 0x0200 ==> length: 0x02 reg: 0x00 offset: 0x00
    uint8_t length  = content >> 8;
    uint8_t reg_off = content & 0x00FF;
    uint8_t reg     = reg_off & 0xFC;     // bit mask: 11111100b
    uint8_t offset  = reg_off % 0x04;     // remainder of modulo operation provides offset

    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus    << 16)
        | (device << 11)
        | (func   <<  8)
        | (reg));

    // use offset to find searched content
    uint32_t readVal = inportl(PCI_CONFIGURATION_DATA) >> (8 * offset);

    switch (length)
    {
        case 1:
            readVal &= 0x000000FF;
            break;
        case 2:
            readVal &= 0x0000FFFF;
            break;
        case 4:
            readVal &= 0xFFFFFFFF;
            break;
    }
    return readVal;
}

void pci_config_write_byte(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t val)
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC));

    outportb(PCI_CONFIGURATION_DATA + (reg & 0x03), val);
} /// correctness of function pci_config_write_byte checked with bar0 from EHCI - ehenkes, 2010-03-24

void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val)
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC));

    outportl(PCI_CONFIGURATION_DATA, val);
}

void pciScan()
{
    printf("\n");
    int number=0;
    for (uint8_t bus = 0; bus < PCIBUSES; ++bus)
    {
        for (uint8_t device = 0; device < PCIDEVICES; ++device)
        {
            for (uint8_t func = 0; func < PCIFUNCS; ++func)
            {
                uint16_t vendorID = pci_config_read(bus, device, func, PCI_VENDOR_ID);
                if (vendorID && vendorID != 0xFFFF)
                {
                    pciDev_Array[number] = malloc(sizeof(pciDev_t), 0, "pciDev_Array");

                    pciDev_Array[number]->vendorID           = vendorID;
                    pciDev_Array[number]->deviceID           = pci_config_read(bus, device, func, PCI_DEVICE_ID);
                    pciDev_Array[number]->classID            = pci_config_read(bus, device, func, PCI_CLASS);
                    pciDev_Array[number]->subclassID         = pci_config_read(bus, device, func, PCI_SUBCLASS);
                    pciDev_Array[number]->interfaceID        = pci_config_read(bus, device, func, PCI_INTERFACE);
                    pciDev_Array[number]->revID              = pci_config_read(bus, device, func, PCI_REVISION);
                    pciDev_Array[number]->irq                = pci_config_read(bus, device, func, PCI_IRQLINE);
                    pciDev_Array[number]->bar[0].baseAddress = pci_config_read(bus, device, func, PCI_BAR0);
                    pciDev_Array[number]->bar[1].baseAddress = pci_config_read(bus, device, func, PCI_BAR1);
                    pciDev_Array[number]->bar[2].baseAddress = pci_config_read(bus, device, func, PCI_BAR2);
                    pciDev_Array[number]->bar[3].baseAddress = pci_config_read(bus, device, func, PCI_BAR3);
                    pciDev_Array[number]->bar[4].baseAddress = pci_config_read(bus, device, func, PCI_BAR4);
                    pciDev_Array[number]->bar[5].baseAddress = pci_config_read(bus, device, func, PCI_BAR5);

                    // Valid Device
                    pciDev_Array[number]->bus    = bus;
                    pciDev_Array[number]->device = device;
                    pciDev_Array[number]->func   = func;
                    pciDev_Array[number]->number = bus*PCIDEVICES*PCIFUNCS + device*PCIFUNCS + func;


                    // Screen output
                    if (pciDev_Array[number]->irq!=255)
                    {
                        printf("#%d %d:%d.%d",
                            number,
                            pciDev_Array[number]->bus,
                            pciDev_Array[number]->device,
                            pciDev_Array[number]->func);

                        printf(" IRQ:%d", pciDev_Array[number]->irq);

                      #ifdef _PCI_VEND_PROD_LIST_
                        // Find Vendor
                        bool found = false;
                        for(uint32_t i = 0; i < PCI_VENTABLE_LEN; i++)
                        {
                            if (PciVenTable[i].VenId == pciDev_Array[number]->vendorID)
                            {
                                printf("\t%s", PciVenTable[i].VenShort); // Found! Display name and break out
                                found = true;
                                break;
                            }
                        }
                        if(!found)
                        {
                            printf("\tvend: %x", pciDev_Array[number]->vendorID); // Vendor not found, display ID
                        }
                        else
                        {
                            // Find Device
                            found = false;
                            for(uint32_t i = 0; i < PCI_DEVTABLE_LEN; i++)
                            {
                                if (PciDevTable[i].DevId == pciDev_Array[number]->deviceID && PciDevTable[i].VenId == pciDev_Array[number]->vendorID) // VendorID and DeviceID has to fit
                                {
                                    printf(" %s", PciDevTable[i].ChipDesc); // Found! Display name and break out
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if(!found)
                        {
                            printf(", dev: %x", pciDev_Array[number]->deviceID); // Device not found, display ID
                        }
                      #else
                        printf("\tvend:%x dev:%x", pciDev_Array[number]->vendorID, pciDev_Array[number]->deviceID);
                      #endif

                        /// USB Host Controller
                        if (pciDev_Array[number]->classID==0x0C && pciDev_Array[number]->subclassID==0x03)
                        {
                            install_USB_HostController(pciDev_Array[number]);
                        }
                        printf("\n");

                        /// network adapters
                        if (pciDev_Array[number]->deviceID == 0x8139) // RTL 8139
                        {
                            install_RTL8139(pciDev_Array[number]);
                        }
                        if (pciDev_Array[number]->deviceID == 0x2000 && pciDev_Array[number]->vendorID == 0x1022) // AMD PCNet III (Am79C973)
                        {
                            install_AMDPCnet(pciDev_Array[number]);
                        }
                    } // if irq != 255
                    ++number;
                } // if pciVendor
                else
                {
                    pciDev_Array[number] = 0;
                }

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if (!(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80))
                {
                    break; // --> not multifunctional, only function 0 used
                }
            } // for function
        } // for device
    } // for bus
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
