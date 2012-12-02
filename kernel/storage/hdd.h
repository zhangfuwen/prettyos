#ifndef HDD_H
#define HDD_H

#include "os.h"
#include "ata.h"
#include "tasking/synchronisation.h"
#include "devicemanager.h"


//WARNING: WIP! Do not use any of these functions on a real PC

typedef struct
{
    HDD_ATACHANNEL channel;
    mutex_t* rwLock;

    port_t* drive;

    bool dma; //Reserved for later use
    bool lba48;
} hdd_t;


void hdd_install(void);

FS_ERROR hdd_writeSectorPIO(uint32_t sector, void* buf, disk_t* device);
FS_ERROR hdd_readSectorPIO(uint32_t sector, void* buf, disk_t* device);


#endif