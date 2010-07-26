#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "os.h"
#include "filesystem/fsmanager.h"
#include "filesystem/fat.h" // obsolete

#define PORTARRAYSIZE 20
#define DISKARRAYSIZE 20
#define PARTITIONARRAYSIZE 4

typedef struct
{
    void (*motorOff)(void*);
} portType_t;

typedef struct
{
    FS_ERROR (*readSector) (uint32_t, uint8_t*, void*);
    FS_ERROR (*writeSector)(uint32_t, uint8_t*, void*);
} diskType_t;

extern portType_t FDD,        USB,     RAM;
extern diskType_t FLOPPYDISK, USB_MSD, RAMDISK;

typedef struct disk
{
    diskType_t*  type;
    partition_t* partition[PARTITIONARRAYSIZE]; // NULL if partition is not used
    char         name[15];
    void*        data;                          // Contains additional information depending on disk-type
    uint32_t     accessRemaining;               // Used to control motor
} disk_t;

typedef struct 
{
    portType_t* type;
    disk_t*     insertedDisk; // NULL if no disk is inserted
    char        name[15];
    void*       data;         // Contains additional information depending on its type
} port_t;


void deviceManager_install(/*partition_t* system*/);
void deviceManager_checkDrives();
void attachPort(port_t* port);
void attachDisk(disk_t* disk);
void removeDisk(disk_t* disk);
void showPortList();
void showDiskList();

partition_t* getPartition(const char* path);
const char*  getFilename (const char* path);

FS_ERROR executeFile(const char* path);

FS_ERROR analyzeBootSector(void* buffer, partition_t* part);

FS_ERROR sectorRead       (uint32_t sector, uint8_t* buffer, partition_t* part);
FS_ERROR singleSectorRead (uint32_t sector, uint8_t* buffer, partition_t* part);
FS_ERROR sectorWrite      (uint32_t sector, uint8_t* buffer, partition_t* part);
FS_ERROR singleSectorWrite(uint32_t sector, uint8_t* buffer, partition_t* part);

#endif
