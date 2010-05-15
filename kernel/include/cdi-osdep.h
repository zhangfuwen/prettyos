#ifndef CDI_OSDEP_H
#define CDI_OSDEP_H

/* CDI_DRIVER shall be used exactly once for each CDI driver. 
   It registers the driver with the CDI library.
   name: Name of the driver
   drv:  A driver description (struct cdi_driver)
   deps: List of names of other drivers on which this driver depends */
#define CDI_DRIVER(name, drv, deps...) /* TODO */

// OS-specific PCI data.
typedef struct
{
} cdi_pci_device_osdep;

// OS-specific DMA data.
typedef struct
{
} cdi_dma_osdep;

// OS-specific data for memory areas.
typedef struct 
{
} cdi_mem_osdep;

#endif
