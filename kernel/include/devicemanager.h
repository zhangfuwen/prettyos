#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "os.h"
#include "fat.h"
#include "usb2.h"

#define PORTARRAYSIZE 20
#define DISKARRAYSIZE 20
#define PARTITIONARRAYSIZE 4

enum PORTTYPE {FDD, USB, RAM};
enum DISKTYPE {FLOPPYDISK, USB_MSD, RAMDISK};

typedef struct 
{
    uint8_t      type;
    partition_t* partition[PARTITIONARRAYSIZE]; // NULL if partition is not used
    void*        data;                          // Contains additional information depending on its type
    uint32_t     serial;
    char*        name;
} disk_t;

typedef struct 
{
    uint8_t type;
    disk_t* insertedDisk; // NULL if no disk is inserted
    void*   data;         // Contains additional information depending on its type
} port_t;

void deviceManager_install(/*partition_t* system*/);
void attachPort(port_t* port);
void attachDisk(disk_t* disk);
void removeDisk(disk_t* disk);
void showPortList();
void showDiskList();

partition_t* getPartition(const char* path);

#endif
