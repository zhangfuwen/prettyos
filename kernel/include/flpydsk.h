#ifndef _FLPYDSK_DRIVER_H
#define _FLPYDSK_DRIVER_H

#include "os.h"
#include "devicemanager.h"

#define DMA_BUFFER 0x1000 // start of dma tranfer buffer, end: 0x10000 (64KB border).
                          // It must be below 16MB = 0x1000000 and in identity mapped memory!

#define MAX_ATTEMPTS_FLOPPY_DMA_BUFFER 60
#define SECTOR 0
#define TRACK  1

typedef struct {
    uint8_t ID;
    bool    motor;
    bool    RW_Lock;
    port_t  drive;
} floppy_t;

void floppy_install();
void flpydsk_install(int32_t irq);

void flpydsk_initialize_dma();
void flpydsk_dma_read();
void flpydsk_dma_write();

uint8_t flpydsk_read_status();
void flpydsk_write_dor(uint8_t val);
void flpydsk_send_command(uint8_t cmd);
uint8_t flpydsk_read_data();
void flpydsk_write_ccr(uint8_t val);
void flpydsk_wait_irq();
void i86_flpy_irq(registers_t* r);
void flpydsk_check_int(uint32_t* st0, uint32_t* cyl);
void flpydsk_control_motor(bool b);
int32_t flpydsk_calibrate(floppy_t* drive);
void flpydsk_drive_data(uint32_t stepr, uint32_t loadt, uint32_t unloadt, int32_t dma);
void flpydsk_disable_controller();
void flpydsk_enable_controller();
void flpydsk_reset();
int32_t flpydsk_seek(uint32_t cyl, uint32_t head);
void flpydsk_lba_to_chs(int32_t lba, int32_t* head, int32_t* track, int32_t* sector);
void flpydsk_refreshVolumeNames();

int32_t flpydsk_transfer_sector(uint8_t head, uint8_t track, uint8_t sector, uint8_t operation);

int32_t flpydsk_readSector(uint32_t sector, uint8_t* buffer);
int32_t flpydsk_read_sector(uint32_t sectorLBA, bool single);
int32_t flpydsk_write_sector(int32_t sectorLBA, bool single);

int32_t flpydsk_read_ia (int32_t i, void* a, int8_t option);
int32_t flpydsk_write_ia(int32_t i, void* a, int8_t option);

#endif
