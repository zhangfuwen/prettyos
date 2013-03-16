#ifndef FLPYDSK_H
#define FLPYDSK_H

#include "os.h"
#include "devicemanager.h"
#include "tasking/synchronisation.h"

#define MAX_FLOPPY                     2
#define MAX_ATTEMPTS_FLOPPY_DMA_BUFFER 5


typedef enum
{
    SECTOR, TRACK
} FLOPPY_MODE;

typedef struct
{
    uint8_t  ID;
    bool     motor;
    mutex_t* RW_Lock;
    port_t   drive;
    uint32_t accessRemaining;

    // buffer
    uint32_t lastTrack;
    uint8_t* trackBuffer;
} floppy_t;


extern floppy_t* floppyDrive[MAX_FLOPPY];


void flpydsk_install(void);
void flpydsk_motorOn (port_t* port);
void flpydsk_motorOff(port_t* port);
void flpydsk_refreshVolumeName(disk_t* disk);
FS_ERROR flpydsk_readSector(uint32_t sector, void* buffer, disk_t* device);
FS_ERROR flpydsk_writeSector(uint32_t sector, void* buffer, disk_t* device);
FS_ERROR flpydsk_write_ia(int32_t i, void* a, FLOPPY_MODE option);


#endif
