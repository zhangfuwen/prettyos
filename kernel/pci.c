/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "pci.h"
#include "util.h"
#include "list.h"
#include "storage/usb_hc.h"
#include "network/network.h"
#include "video/console.h"
#include "kheap.h"

#ifdef _PCI_VEND_PROD_LIST_
  #include "pciVendProdList.h" // http://www.pcidatabase.com/pci_c_header.php
#endif


static list_t* devices = 0;

void pci_analyzeHostSystemError(pciDev_t* pciDev)
{
    // check pci status register of the device
    uint32_t pciStatus = pci_config_read(pciDev->bus, pciDev->device, pciDev->func, PCI_STATUS);

    textColor(HEADLINE);
    printf("\nPCI status word: %xh\n",pciStatus);
    textColor(TEXT);
    // bits 0...2 reserved
    if(pciStatus & BIT(3))  printf("Interrupt Status\n");
    if(pciStatus & BIT(4))  printf("Capabilities List\n");
    if(pciStatus & BIT(5))  printf("66 MHz Capable\n");
    // bit 6 reserved
    if(pciStatus & BIT(7))  printf("Fast Back-to-Back Transactions Capable\n");
    textColor(ERROR);
    if(pciStatus & BIT(8))  printf("Master Data Parity Error\n");
    // DEVSEL Timing: bits 10:9
    if(pciStatus & BIT(11)) printf("Signalled Target-Abort\n");
    if(pciStatus & BIT(12)) printf("Received Target-Abort\n");
    if(pciStatus & BIT(13)) printf("Received Master-Abort\n");
    if(pciStatus & BIT(14)) printf("Signalled System Error\n");
    if(pciStatus & BIT(15)) printf("Detected Parity Error\n");
    textColor(TEXT);
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

bool pci_deviceSentInterrupt(pciDev_t* dev)
{
    uint32_t statusRegister = pci_config_read(dev->bus, dev->device, dev->func, PCI_STATUS);
    return(statusRegister & BIT(3));
}

void pci_scan()
{
    devices = list_create();

    textColor(LIGHT_GRAY);
    printf("   => PCI devices:");
    textColor(TABLE_HEADING);
    printf("\nB:D:F\tIRQ\tDescription");
    printf("\n--------------------------------------------------------------------------------");
    textColor(TEXT);
    for (uint16_t bus = 0; bus < PCIBUSES; ++bus)
    {
        for (uint8_t device = 0; device < PCIDEVICES; ++device)
        {
            uint8_t headerType = pci_config_read(bus, device, 0, PCI_HEADERTYPE);
            uint8_t funcCount = PCIFUNCS;
            if (!(headerType & 0x80)) // Bit 7 in header type (Bit 23-16) --> multifunctional
            {
                funcCount = 1; // --> not multifunctional, only function 0 used
            }

            for (uint8_t func = 0; func < funcCount; ++func)
            {
                uint16_t vendorID = pci_config_read(bus, device, func, PCI_VENDOR_ID);
                if (vendorID && vendorID != 0xFFFF)
                {
                    pciDev_t* PCIdev = malloc(sizeof(pciDev_t), 0, "pciDev_t");
                    list_append(devices, PCIdev);

                    PCIdev->vendorID           = vendorID;
                    PCIdev->deviceID           = pci_config_read(bus, device, func, PCI_DEVICE_ID);
                    PCIdev->classID            = pci_config_read(bus, device, func, PCI_CLASS);
                    PCIdev->subclassID         = pci_config_read(bus, device, func, PCI_SUBCLASS);
                    PCIdev->interfaceID        = pci_config_read(bus, device, func, PCI_INTERFACE);
                    PCIdev->revID              = pci_config_read(bus, device, func, PCI_REVISION);
                    PCIdev->irq                = pci_config_read(bus, device, func, PCI_IRQLINE);
                    // Read BARs
                    for(uint8_t i = 0; i < 6; i++)
                    {
                        if(i < 2 || !(headerType & 0x01)) // Devices with header type 0x00 have 6 bars
                        {
                            PCIdev->bar[i].baseAddress = pci_config_read(bus, device, func, PCI_BAR0+i*4);
                            if(PCIdev->bar[i].baseAddress) // Valid bar
                            {
                                // Check memory type
                                PCIdev->bar[i].memoryType = PCIdev->bar[i].baseAddress & 0x01;
                                if(PCIdev->bar[i].memoryType == 0) // MMIO bar
                                    PCIdev->bar[i].baseAddress &= 0xFFFFFFF0;
                                else                               // IO bar
                                    PCIdev->bar[i].baseAddress &= 0xFFFC;

                                // Check Memory Size
                                cli();
                                pci_config_write_dword(bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF);
                                PCIdev->bar[i].memorySize = (~(pci_config_read(bus, device, func, PCI_BAR0 + 4*i)) | 0x0F) + 1;
                                pci_config_write_dword(bus, device, func, PCI_BAR0 + 4*i, PCIdev->bar[i].baseAddress);
                                sti();
                            }
                            else
                                PCIdev->bar[i].memoryType = PCI_INVALIDBAR;
                        }
                        else
                            PCIdev->bar[i].memoryType = PCI_INVALIDBAR;
                    }

                    PCIdev->bus    = bus;
                    PCIdev->device = device;
                    PCIdev->func   = func;

                    // Screen output
                    if (PCIdev->irq != 255)
                    {
                        printf("%d:%d.%d\t%d", PCIdev->bus, PCIdev->device, PCIdev->func, PCIdev->irq);

                      #ifdef _PCI_VEND_PROD_LIST_
                        // Find Vendor
                        bool found = false;
                        for(uint32_t i = 0; i < PCI_VENTABLE_LEN; i++)
                        {
                            if (PciVenTable[i].VenId == PCIdev->vendorID)
                            {
                                printf("\t%s", PciVenTable[i].VenShort); // Found! Display name and break out
                                found = true;
                                break;
                            }
                        }
                        if(!found)
                        {
                            printf("\tvend: %xh", PCIdev->vendorID); // Vendor not found, display ID
                        }
                        else
                        {
                            // Find Device
                            found = false;
                            for(uint32_t i = 0; i < PCI_DEVTABLE_LEN; i++)
                            {
                                if (PciDevTable[i].DevId == PCIdev->deviceID && PciDevTable[i].VenId == PCIdev->vendorID) // VendorID and DeviceID have to fit
                                {
                                    printf(", %s", PciDevTable[i].ChipDesc); // Found! Display name and break out
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if(!found)
                        {
                            printf(", dev: %xh", PCIdev->deviceID); // Device not found, display ID
                        }
                      #else
                        printf("\tvend: %xh, dev: %xh", PCIdev->vendorID, PCIdev->deviceID);
                      #endif

                        /// USB Host Controller
                        if (PCIdev->classID == 0x0C && PCIdev->subclassID == 0x03)
                        {
                            install_USB_HostController(PCIdev);
                        }
                        putch('\n');

                        /// network adapters
                        network_installDevice(PCIdev);
                    } // if irq != 255
                } // if pciVendor
            } // for function
        } // for device
    } // for bus
    textColor(TABLE_HEADING);
    printf("--------------------------------------------------------------------------------\n");
    textColor(TEXT);
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
