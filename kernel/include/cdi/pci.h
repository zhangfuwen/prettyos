/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_PCI_H_
#define _CDI_PCI_H_

/// #include <stdint.h> /// CDI-style
#include "os.h"         /// PrettyOS work-around

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


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu
 * in die uebergebene Liste eingefuegt.
 */
void cdi_pci_get_all_devices(cdi_list_t list);

/**
 * Gibt die Information zu einem PCI-Geraet frei
 */
void cdi_pci_device_destroy(struct cdi_pci_device* device);

/**
 * Reserviert die IO-Ports des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_ioports(struct cdi_pci_device* device);

/**
 * Gibt die IO-Ports des PCI-Geraets frei
 */
void cdi_pci_free_ioports(struct cdi_pci_device* device);

/**
 * Reserviert den MMIO-Speicher des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_memory(struct cdi_pci_device* device);

/**
 * Gibt den MMIO-Speicher des PCI-Geraets frei
 */
void cdi_pci_free_memory(struct cdi_pci_device* device);

/**
 * \if german
 * Signalisiert CDI-Treibern direkten Zugriff auf den PCI-Konfigurationsraum
 * \elseif english
 * Indicates direct access to the PCI configuration space to CDI drivers
 * \endif
 */
#define CDI_PCI_DIRECT_ACCESS

/**
 * \if german
 * Liest ein Byte (8 Bit) aus dem PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 *
 * @return Der Wert an diesem Offset
 * \elseif english
 * Reads a byte (8 bit) from the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 *
 * @return The corresponding value
 * \endif
 */
uint8_t cdi_pci_config_readb(struct cdi_pci_device* device, uint8_t offset);

/**
 * \if german
 * Liest ein Word (16 Bit) aus dem PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 *
 * @return Der Wert an diesem Offset
 * \elseif english
 * Reads a word (16 bit) from the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 *
 * @return The corresponding value
 * \endif
 */
uint16_t cdi_pci_config_readw(struct cdi_pci_device* device, uint8_t offset);

/**
 * \if german
 * Liest ein DWord (32 Bit) aus dem PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 *
 * @return Der Wert an diesem Offset
 * \elseif english
 * Reads a dword (32 bit) from the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 *
 * @return The corresponding value
 * \endif
 */
uint32_t cdi_pci_config_readl(struct cdi_pci_device* device, uint8_t offset);

/**
 * \if german
 * Schreibt ein Byte (8 Bit) in den PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 * @param value Der zu setzende Wert
 * \elseif english
 * Writes a byte (8 bit) into the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 * @param value Value to be set
 * \endif
 */
void cdi_pci_config_writeb(struct cdi_pci_device* device, uint8_t offset,
    uint8_t value);

/**
 * \if german
 * Schreibt ein Word (16 Bit) in den PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 * @param value Der zu setzende Wert
 * \elseif english
 * Writes a word (16 bit) into the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 * @param value Value to be set
 * \endif
 */
void cdi_pci_config_writew(struct cdi_pci_device* device, uint8_t offset,
    uint16_t value);

/**
 * \if german
 * Schreibt ein DWord (32 Bit) in den PCI-Konfigurationsraum eines PCI-Geraets
 *
 * @param device Das Geraet
 * @param offset Der Offset im Konfigurationsraum
 * @param value Der zu setzende Wert
 * \elseif english
 * Writes a dword (32 bit) into the PCI configuration space of a PCI device
 *
 * @param device The device
 * @param offset The offset in the configuration space
 * @param value Value to be set
 * \endif
 */
void cdi_pci_config_writel(struct cdi_pci_device* device, uint8_t offset,
    uint32_t value);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

