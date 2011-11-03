#ifndef PCI_H
#define PCI_H

#include "os.h"
#include "util/list.h"


#define PCI_CONFIGURATION_ADDRESS 0x0CF8   // Address I/O Port
#define PCI_CONFIGURATION_DATA    0x0CFC   // Data    I/O Port

#define PCI_VENDOR_ID   0x00 // length: 0x02 reg: 0x00 offset: 0x00
#define PCI_DEVICE_ID   0x02 // length: 0x02 reg: 0x00 offset: 0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION    0x08
#define PCI_CLASS       0x0B
#define PCI_SUBCLASS    0x0A
#define PCI_INTERFACE   0x09
#define PCI_HEADERTYPE  0x0E
#define PCI_BAR0        0x10
#define PCI_BAR1        0x14
#define PCI_BAR2        0x18
#define PCI_BAR3        0x1C
#define PCI_BAR4        0x20
#define PCI_BAR5        0x24
#define PCI_CAPLIST     0x34
#define PCI_IRQLINE     0x3C

#define PCI_CMD_IO        BIT(0)
#define PCI_CMD_MMIO      BIT(1)
#define PCI_CMD_BUSMASTER BIT(2)

#define PCIBUSES      256
#define PCIDEVICES    32
#define PCIFUNCS      8


enum
{
    PCI_MMIO, PCI_IO, PCI_INVALIDBAR
};

typedef struct
{
    uint32_t baseAddress;
    size_t   memorySize;
    uint8_t  memoryType;
} pciBar_t;

typedef struct
{
   uint8_t   bus;
   uint8_t   device;
   uint8_t   func;
   uint16_t  vendorID;
   uint16_t  deviceID;
   uint8_t   classID;
   uint8_t   subclassID;
   uint8_t   interfaceID;
   uint8_t   revID;
   uint8_t   irq;
   pciBar_t  bar[6];
   void*     data; // Pointer to internal data of associated driver.
} pciDev_t;


extern list_t* pci_devices;


void     pci_scan();
uint32_t pci_config_read       (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg_off, uint8_t length);
void     pci_config_write_byte (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t  val);
void     pci_config_write_word (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint16_t val);
void     pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val);
void     pci_analyzeHostSystemError(pciDev_t* pciDev);
bool     pci_deviceSentInterrupt(pciDev_t* dev);


#endif
