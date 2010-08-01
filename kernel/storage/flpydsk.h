#ifndef FLPYDSK_DRIVER_H
#define FLPYDSK_DRIVER_H

#include "os.h"
#include "devicemanager.h"

#define DMA_BUFFER 0x1000 // start of dma tranfer buffer, end: 0x10000 (64 KiB border).
                          // It must be below 16 MiB = 0x1000000 and in identity mapped memory!

#define MAX_FLOPPY 2
#define MAX_ATTEMPTS_FLOPPY_DMA_BUFFER 60

typedef enum {
    SECTOR, TRACK
} FLOPPY_MODE;

typedef struct {
    uint8_t  ID;
    bool     motor;
    bool     RW_Lock;
    bool     receivedIRQ;
    port_t   drive;
    uint32_t accessRemaining;
} floppy_t;

void flpydsk_install();

void i86_flpy_irq(registers_t* r);
void flpydsk_initialize_dma();

void flpydsk_motorOn (void* drive);
void flpydsk_motorOff(void* drive);

void flpydsk_refreshVolumeNames();

FS_ERROR flpydsk_readSector(uint32_t sector, uint8_t* buffer, void* device);
FS_ERROR flpydsk_read_sector(uint32_t sectorLBA, bool single);
FS_ERROR flpydsk_writeSector(uint32_t sector, uint8_t* buffer, void* device);
FS_ERROR flpydsk_write_sector(uint32_t sectorLBA, bool single);

FS_ERROR flpydsk_read_ia (int32_t i, void* a, FLOPPY_MODE option);
FS_ERROR flpydsk_write_ia(int32_t i, void* a, FLOPPY_MODE option);

#endif
