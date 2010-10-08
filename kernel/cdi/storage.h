// Driver for mass-storage-devices
#ifndef CDI_STORAGE_H
#define CDI_STORAGE_H

#include "os.h"
#include <cdi.h>

struct cdi_storage_device {
    struct cdi_device   dev;
    size_t              block_size;
    uint64_t            block_count;
};

struct cdi_storage_driver {
    struct cdi_driver   drv;

    /* Liest Blocks ein
       start:  Blocknummer des ersten zu lesenden Blockes (angefangen bei 0).
       count:  Anzahl der zu lesenden Blocks
       buffer: Puffer in dem die Daten abgelegt werden sollen
       return: 0 bei Erfolg, -1 im Fehlerfall. */
    int (*read_blocks)(struct cdi_storage_device* device, uint64_t start, uint64_t count, void* buffer);

    /* Schreibt Blocks
       start:  Blocknummer des ersten zu schreibenden Blockes (angefangen bei 0).
       count:  Anzahl der zu schreibenden Blocks
       buffer: Puffer aus dem die Daten gelesen werden sollen
       return: 0 bei Erfolg, -1 im Fehlerfall */
    int (*write_blocks)(struct cdi_storage_device* device, uint64_t start, uint64_t count, void* buffer);
};

// Initialisiert die Datenstrukturen fuer einen Massenspeichertreiber (erzeugt die devices-Liste)
void cdi_storage_driver_init(struct cdi_storage_driver* driver);

// Deinitialisiert die Datenstrukturen fuer einen Massenspeichertreiber (gibt die devices-Liste frei)
void cdi_storage_driver_destroy(struct cdi_storage_driver* driver);

// Registiert einen Massenspeichertreiber
void cdi_storage_driver_register(struct cdi_storage_driver* driver);

// Initialisiert einen Massenspeicher
void cdi_storage_device_init(struct cdi_storage_device* device);

#endif
