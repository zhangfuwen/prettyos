#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "os.h"
#include "fat.h"
#include "usb2.h"

#define MSDARRAYSIZE 30 

enum DEVICETYPE {FLOPPYDISK, RAMDISK, USBMSD};

typedef struct MassStorageDevice
{
    uint8_t         type;         // floppy, RAM disk, usbmsd, ...
    uint8_t         numberOfPartitions;
    PARTITION*      Partition[4]; // 4 primary partitions
    bool            connected;    // attached to PrettyOS
    uint8_t         portNumber;   // usb port: 0-15; 255: no usb port
    uint32_t        globalMSD;
    usb2_Device_t*  usb2Device;   
} MSD_t;

void MSDmanager_install();
void addToMSDmanager(MSD_t* msd);
void deleteFromMSDmanager(MSD_t* msd);
uint32_t getMSDVolumeNumber();
void showMSDAttached();

#endif
