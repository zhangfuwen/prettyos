#ifndef CDI_PCI_H
#define CDI_PCI_H

#include "os.h"

#include <cdi.h>
#include <cdi-osdep.h>
#include <cdi/lists.h>

struct cdi_pci_device {
    struct cdi_bus_data bus_data;

    uint16_t    bus;
    uint16_t    dev;
    uint16_t    function;

    uint16_t    vendor_id;
    uint16_t    device_id;

    uint8_t     class_id;
    uint8_t     subclass_id;
    uint8_t     interface_id;

    uint8_t     rev_id;

    uint8_t     irq;

    cdi_list_t resources;

    cdi_pci_device_osdep meta;
};

typedef enum {
    CDI_PCI_MEMORY,
    CDI_PCI_IOPORTS
} cdi_res_t;

struct cdi_pci_resource {
    cdi_res_t    type;
    uintptr_t    start;
    size_t       length;
    unsigned int index;
    void*        address;
};


// Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu in die uebergebene Liste eingefuegt.
void cdi_pci_get_all_devices(cdi_list_t list);

// Gibt die Information zu einem PCI-Geraet frei
void cdi_pci_device_destroy(struct cdi_pci_device* device);

// Reserviert die IO-Ports des PCI-Geraets fuer den Treiber
void cdi_pci_alloc_ioports(struct cdi_pci_device* device);

// Gibt die IO-Ports des PCI-Geraets frei
void cdi_pci_free_ioports(struct cdi_pci_device* device);

// Reserviert den MMIO-Speicher des PCI-Geraets fuer den Treiber
void cdi_pci_alloc_memory(struct cdi_pci_device* device);

// Gibt den MMIO-Speicher des PCI-Geraets frei
void cdi_pci_free_memory(struct cdi_pci_device* device);

// Indicates direct access to the PCI configuration space to CDI drivers
#define CDI_PCI_DIRECT_ACCESS

/* Reads a byte (8 bit) from the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   return: The corresponding value */
uint8_t cdi_pci_config_readb(struct cdi_pci_device* device, uint8_t offset);

/* Reads a word (16 bit) from the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   return: The corresponding value */
uint16_t cdi_pci_config_readw(struct cdi_pci_device* device, uint8_t offset);

/* Reads a dword (32 bit) from the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   return: The corresponding value */
uint32_t cdi_pci_config_readl(struct cdi_pci_device* device, uint8_t offset);

/* Writes a byte (8 bit) into the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   value:  Value to be set */
void cdi_pci_config_writeb(struct cdi_pci_device* device, uint8_t offset, uint8_t value);

/* Writes a word (16 bit) into the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   value:  Value to be set */
void cdi_pci_config_writew(struct cdi_pci_device* device, uint8_t offset, uint16_t value);

/* Writes a dword (32 bit) into the PCI configuration space of a PCI device
   device: The device
   offset: The offset in the configuration space
   value:  Value to be set */
void cdi_pci_config_writel(struct cdi_pci_device* device, uint8_t offset, uint32_t value);

#endif
