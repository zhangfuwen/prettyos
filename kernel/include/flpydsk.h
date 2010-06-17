#ifndef _FLPYDSK_DRIVER_H
#define _FLPYDSK_DRIVER_H

#include "os.h"
#include "devicemanager.h"

#define DMA_BUFFER 0x1000 // start of dma tranfer buffer, end: 0x10000 (64 KiB border).
                          // It must be below 16 MiB = 0x1000000 and in identity mapped memory!

#define MAX_FLOPPY 2
#define MAX_ATTEMPTS_FLOPPY_DMA_BUFFER 60
#define SECTOR 0
#define TRACK  1

typedef struct {
    uint8_t  ID;
    bool     motor;
    bool     RW_Lock;
    port_t   drive;
    uint32_t accessRemaining;
} floppy_t;

void floppy_install();
void flpydsk_install(int32_t irq);

void i86_flpy_irq(registers_t* r);
void flpydsk_control_motor(bool b);
void flpydsk_refreshVolumeNames();

int32_t flpydsk_readSector(uint32_t sector, uint8_t* buffer);
int32_t flpydsk_read_sector(uint32_t sectorLBA, bool single);
int32_t flpydsk_writeSector(uint32_t sector, uint8_t* buffer);
int32_t flpydsk_write_sector(uint32_t sectorLBA, bool single);

int32_t flpydsk_read_ia (int32_t i, void* a, int8_t option);
int32_t flpydsk_write_ia(int32_t i, void* a, int8_t option);

#endif
