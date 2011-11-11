#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "os.h"
#include "filesystem/fsmanager.h"

#define PORTARRAYSIZE 26
#define DISKARRAYSIZE 26
#define PARTITIONARRAYSIZE 4


struct port;

typedef struct
{
    void (*motorOff)(struct port*);
    void (*pollDisk)(struct port*);
} portType_t;

typedef struct
{
    FS_ERROR (*readSector) (uint32_t, void*, struct disk*);
    FS_ERROR (*writeSector)(uint32_t, void*, struct disk*);
} diskType_t;

extern portType_t FDD, USB_UHCI, USB_OHCI, USB_EHCI, RAM;
extern diskType_t FLOPPYDISK, USB_MSD, RAMDISK;

typedef struct disk
{
    diskType_t*  type;
    partition_t* partition[PARTITIONARRAYSIZE]; // 0 if partition is not used
    size_t       size;                          // size of disk in bytes
    char         name[15];
    void*        data;                          // Contains additional information depending on disk-type
    uint32_t     accessRemaining;               // Used to control motor
    struct port* port;

    // Technical data of the disk
    uint32_t sectorSize;    // Bytes per sector
    uint16_t secPerTrack;   // Number of sectors per track (if the disk is separated into tracks)
    uint16_t headCount;     // Number of heads (if the disk is separated into heads)
    uint8_t  BIOS_driveNum; // Number of this disk given by BIOS
} disk_t;

typedef struct port
{
    portType_t* type;
    disk_t*     insertedDisk; // 0 if no disk is inserted
    char        name[15];
    void*       data;         // Contains additional information depending on its type
} port_t;

// 16-byte partition record // http://en.wikipedia.org/wiki/Master_boot_record
typedef struct
{
    uint8_t  bootflag;     // Status: 0x80 = bootable (active), 0x00 = non-bootable, other = invalid
    uint8_t  startCHS[3];  // CHS address of first absolute sector in partition
    uint8_t  type;         // http://en.wikipedia.org/wiki/Partition_type
    uint8_t  endCHS[3];    // CHS address of last absolute sector in partition
    uint32_t startLBA;     // LBA of first absolute sector in the partition
    uint32_t sizeLBA;      // Number of sectors in partition
} __attribute__((packed)) partitionEntry_t;


void deviceManager_install(partition_t* systemPart);
void deviceManager_checkDrives();
void attachPort(port_t* port);
void attachDisk(disk_t* disk);
void removeDisk(disk_t* disk);
void showPortList();
void showDiskList();

partition_t* getPartition(const char* path);
const char*  getFilename (const char* path);

FS_ERROR analyzeDisk(disk_t* disk);

FS_ERROR sectorRead       (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR singleSectorRead (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR sectorWrite      (uint32_t sector, uint8_t* buffer, disk_t* disk);
FS_ERROR singleSectorWrite(uint32_t sector, uint8_t* buffer, disk_t* disk);


#endif
