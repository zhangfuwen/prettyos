#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "os.h"
#include "filesystem/fsmanager.h"

#define PORTARRAYSIZE 20
#define DISKARRAYSIZE 20
#define PARTITIONARRAYSIZE 4

typedef struct
{
    void (*motorOff)(void*);
} portType_t;

typedef struct
{
    FS_ERROR (*readSector) (uint32_t, void*, void*);
    FS_ERROR (*writeSector)(uint32_t, void*, void*);
} diskType_t;

extern portType_t FDD,        USB,     RAM;
extern diskType_t FLOPPYDISK, USB_MSD, RAMDISK;

typedef struct disk
{
    diskType_t*  type;
    partition_t* partition[PARTITIONARRAYSIZE]; // 0 if partition is not used
    char         name[15];
    void*        data;                          // Contains additional information depending on disk-type
    uint32_t     accessRemaining;               // Used to control motor

    // Technical data of the disk
    uint32_t sectorSize;    // Bytes per sector
    uint16_t secPerTrack;   // Number of sectors per track (if the disk is separated into tracks)
    uint16_t headCount;     // Number of heads (if the disk is separated into heads)
    uint8_t  BIOS_driveNum; // Number of this disk given by BIOS
} disk_t;

typedef struct
{
    portType_t* type;
    disk_t*     insertedDisk; // 0 if no disk is inserted
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

FS_ERROR analyzeDisk(disk_t* disk);

FS_ERROR sectorRead       (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR singleSectorRead (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR sectorWrite      (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR singleSectorWrite(uint32_t sector, uint8_t* buffer, disk_t* disk);

#endif
