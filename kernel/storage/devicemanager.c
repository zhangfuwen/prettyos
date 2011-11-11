/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "devicemanager.h"
#include "video/console.h"
#include "util/util.h"
#include "kheap.h"
#include "usb_msd.h"
#include "flpydsk.h"
#include "filesystem/fat.h"
#include "uhci.h"
#ifdef _READCACHE_DIAGNOSIS_
  #include "timer.h"
#endif


disk_t* disks[DISKARRAYSIZE];
port_t* ports[PORTARRAYSIZE];
partition_t* systemPartition = 0;

portType_t FDD      = {.motorOff = &flpydsk_motorOff, .pollDisk = 0},
           USB_UHCI = {.motorOff = 0,                 .pollDisk = &uhci_pollDisk},
           USB_OHCI = {.motorOff = 0,                 .pollDisk = 0},
           USB_EHCI = {.motorOff = 0,                 .pollDisk = 0},
           RAM      = {.motorOff = 0,                 .pollDisk = 0};

diskType_t FLOPPYDISK = {.readSector = &flpydsk_readSector, .writeSector = &flpydsk_writeSector},
           USB_MSD    = {.readSector = &usb_read,           .writeSector = &usb_write},
           RAMDISK    = {.readSector = 0,                   .writeSector = 0};

// ReadCache
#define NUMREADCACHE 20

typedef struct
{
    uint8_t buffer[512];
    bool valid;
    disk_t* disk;
    uint32_t sector;
} readcache_t;

static readcache_t readcaches[NUMREADCACHE];
static uint8_t     currReadCache = 0;


void deviceManager_install(partition_t* systemPart)
{
    memset(disks, 0, DISKARRAYSIZE*sizeof(disk_t*));
    memset(ports, 0, PORTARRAYSIZE*sizeof(port_t*));

    for (uint16_t i = 0; i < NUMREADCACHE; i++) // Invalidate all read caches
    {
        readcaches[i].valid = false;
    }

    systemPartition = systemPart;
}

void deviceManager_checkDrives()
{
    for (size_t i = 0; i < PORTARRAYSIZE; i++)
    {
        if(ports[i] != 0 && ports[i]->type->pollDisk != 0)
        {
            ports[i]->type->pollDisk(ports[i]);
        }

        if (ports[i] != 0 && ports[i]->type->motorOff != 0 && ports[i]->insertedDisk->accessRemaining == 0)
        {
            ports[i]->type->motorOff(ports[i]);
        }
    }
}

void attachPort(port_t* port)
{
    for (size_t i=0; i<PORTARRAYSIZE; i++)
    {
        if (ports[i] == 0)
        {
            ports[i] = port;
            return;
        }
    }
}

void attachDisk(disk_t* disk)
{
    // Later: Searching correct ID in device-File
    for (size_t i=0; i<DISKARRAYSIZE; i++)
    {
        if (disks[i] == 0)
        {
            disks[i] = disk;
            return;
        }
    }
}

void removeDisk(disk_t* disk)
{
    for (size_t i=0; i<DISKARRAYSIZE; i++)
    {
        if (disks[i] == disk)
        {
            disks[i] = 0;
            return;
        }
    }
}

void showPortList()
{
    textColor(HEADLINE);
    printf("\n\nAvailable ports:");
    textColor(TABLE_HEADING);
    printf("\nType            Letter\tName\t\tInserted disk");
    printf("\n----------------------------------------------------------------------");
    textColor(TEXT);

    for (size_t i=0; i < PORTARRAYSIZE; i++)
    {
        if (ports[i] != 0)
        {
            if (ports[i]->type == &FDD) // Type
                printf("\nFDD            ");
            else if (ports[i]->type == &RAM)
                printf("\nRAM            ");
            else if (ports[i]->type == &USB_OHCI)
                printf("\nUSB 1.1 (OHCI) ");
            else if (ports[i]->type == &USB_UHCI)
                printf("\nUSB 1.1 (UHCI) ");
            else if (ports[i]->type == &USB_EHCI)
                printf("\nUSB 2.0        ");

            textColor(IMPORTANT);
            printf("\t%c", 'A'+i); // number
            textColor(TEXT);
            printf("\t%s", ports[i]->name); // The ports name

            if (ports[i]->insertedDisk != 0)
            {
                if(ports[i]->type == &FDD)
                    flpydsk_refreshVolumeName(ports[i]->insertedDisk);
                printf("\t%s", ports[i]->insertedDisk->name); // Attached disk
            }
            else
            {
               printf("\t-");
            }
        }
    }
    textColor(TABLE_HEADING);
    printf("\n----------------------------------------------------------------------\n");
    textColor(TEXT);
}

void showDiskList()
{
    textColor(HEADLINE);
    printf("\n\nAttached disks:");
    textColor(TABLE_HEADING);
    printf("\nType\tNumber\tSize\t\tName\t\tPart.\tSerial");
    printf("\n----------------------------------------------------------------------");
    textColor(TEXT);

    for (size_t i=0; i < DISKARRAYSIZE; i++)
    {
        if (disks[i] != 0)
        {
            if (disks[i]->type == &FLOPPYDISK) /// Todo: Move to flpydsk.c, name set on floppy insertion
            {
                flpydsk_refreshVolumeName(disks[i]);
            }

            if (disks[i]->type == &FLOPPYDISK && *disks[i]->name == 0) // Floppy workaround
                continue;

            if      (disks[i]->type == &FLOPPYDISK) printf("\nFloppy");
            else if (disks[i]->type == &RAMDISK)    printf("\nRAMdisk");
            else if (disks[i]->type == &USB_MSD)    printf("\nUSB MSD");

            textColor(IMPORTANT);
            printf("\t%u", i+1); // Number
            textColor(TEXT);

            if(printf("\t%Sa", disks[i]->size) < 8)
                putch('\t');

            printf("\t%s", disks[i]->name); // Name of disk

            if (strlen(disks[i]->name) < 8)
            {
                putch('\t');
            }

            for (uint8_t j=0; j < PARTITIONARRAYSIZE; j++)
            {
                if (disks[i]->partition[j] == 0) // Empty
                    continue;

                if (j!=0)
                {
                    printf("\n\t\t\t"); // Not first, indent
                }

                printf("\t%u\t", j); // Partition number

                if (disks[i]->type == &FLOPPYDISK) // Serial
                {
                    //HACK
                    if(disks[i]->partition[j]->serial == 0)
                    {
                        disks[i]->partition[j]->serial = malloc(13, 0, "devmgr-partserial");
                    }

                    disks[i]->partition[j]->serial[12] = 0;
                    strncpy(disks[i]->partition[j]->serial, disks[i]->name, 12); // TODO: floppy disk device: use the current serials of the floppy disks
                    puts(disks[i]->partition[j]->serial);
                }
                else if (disks[i]->type == &RAMDISK)
                {
                    puts(disks[i]->partition[j]->serial);
                }
                else if (disks[i]->type == &USB_MSD)
                {
                    puts(((usb_device_t*)disks[i]->data)->serialNumber);
                }
                /// TODO: ifs should be changed to:
                /// printf("\t%s", disks[i]->partition[j]->serial); // serial of partition
            }
        }
    }
    textColor(TABLE_HEADING);
    printf("\n----------------------------------------------------------------------\n");
    textColor(TEXT);
}

const char* getFilename(const char* path)
{
    if (strpbrk((char*)path,"/|\\") == 0)
    {
        return path;
    }
    else
    {
        while (*path != '/' && *path != '|' && *path != '\\')
        {
            path++;
            if (*path == 0)
            {
                return (0);
            }
        }
        path++;
        return (path);
    }
}


partition_t* getPartition(const char* path)
{
    int16_t PortID = -1;
    int16_t DiskID = -1;
    uint8_t PartitionID = 0;
    for (size_t i=0; path[i]; i++)
    {
        if ((path[i] == ':') || (path[i] == '-'))
        {
            if (isalpha(path[0]))
            {
                PortID = toUpper(path[0]) - 'A';
            }
            else
            {
                DiskID = atoi(path);
            }

            for (size_t j=i+1; path[j]; j++)
            {
                if ((path[j] == ':') || (path[j] == '-'))
                {
                    PartitionID = atoi(path+i+1);
                    break;
                }
                if (!isdigit(path[j]) || (path[j] == '/') || (path[j] == '|') || (path[j] == '\\'))
                {
                    break;
                }
            }
            break;
        }
        if (!isalnum(path[i]))
        {
            return (0);
        }
    }

    if ((PortID != -1) && (PortID < PORTARRAYSIZE))
    {
        if(ports[PortID] && ports[PortID]->insertedDisk)
        {
            return (ports[PortID]->insertedDisk->partition[PartitionID]);
        }
    }
    else
    {
        if (DiskID == 0)
        {
            return (systemPartition);
        }
        if (DiskID > 0 && DiskID <= DISKARRAYSIZE && disks[DiskID-1])
        {
            return (disks[DiskID-1]->partition[PartitionID]);
        }
    }
    return (0);
}

FS_ERROR analyzeDisk(disk_t* disk)
{
    uint8_t buffer[512];

    singleSectorRead(0, buffer, disk); // first sector of partition

    BPBbase_t* BPB = (BPBbase_t*)buffer; // BIOS Parameter Block (BPB)
    if (!(BPB->FATcount > 0 && BPB->bytesPerSector % 512 == 0 && BPB->bytesPerSector != 0) && // Data look not like a BPB ...
        (buffer[510] == 0x55 && buffer[511] == 0xAA)) // ... but like a Master Boot Record (MBR)
    {
        // Read partitions from MBR
        printf("\nFound MBR (DiskID: %xh):", ((uint16_t*)buffer)[440/2]);
        partitionEntry_t* entries = (partitionEntry_t*)(buffer+446);

        for (uint8_t i=0; i < 4; i++) // four entries in partition table
        {
            if (entries[i].type != 0) // valid entry
            {
                printf("\npartition entry %u: start: %u\tsize: %u\t type: ", i, entries[i].startLBA, entries[i].sizeLBA);

                disk->partition[i]         = malloc(sizeof(partition_t), 0, "partition_t");
                disk->partition[i]->start  = entries[i].startLBA;
                disk->partition[i]->size   = entries[i].sizeLBA;
                disk->partition[i]->disk   = disk;
                disk->partition[i]->serial = 0;

                if (analyzePartition(disk->partition[i]) != CE_GOOD)
                {
                    printf("unknown");
                }
            }
            else
            {
                disk->partition[i] = 0;
                // printf("\npartition entry %u: invalid", i);
            }
        }
    }
    else // There is just one partition
    {
        printf("       => Only single partition on disk. (type: ");

        disk->partition[0]         = malloc(sizeof(partition_t), 0, "partition_t");
        disk->partition[0]->start  = 0;
        disk->partition[0]->size   = disk->size; // Whole size of disk
        disk->partition[0]->disk   = disk;
        disk->partition[0]->serial = 0;
        disk->partition[1]         = 0;
        disk->partition[2]         = 0;
        disk->partition[3]         = 0;

        if (analyzePartition(disk->partition[0]) != CE_GOOD)
        {
            printf("unknown)");
            return (CE_NOT_FORMATTED);
        }
        putch(')');
    }
    return (CE_GOOD);
}


#ifdef _READCACHE_DIAGNOSIS_
static void logReadCache()
{
    for (uint16_t i = 0; i < NUMREADCACHE; i++)
    {
        if (readcaches[i].valid)
        {
            textColor(GREEN);
        }
        else
        {
            textColor(LIGHT_GRAY);
        }
        printf("\nReadcache: %u \tpart: %Xh sector: %u \tvalid: %s", i, readcaches[i].disk, readcaches[i].sector, readcaches[i].valid?"yes":"no");
    }
    textColor(LIGHT_GRAY);
    printf("\n-------------------------------------------------------------------------------");
    sleepMilliSeconds(500);
    textColor(TEXT);
}
#endif

static void fillReadCache(uint32_t sector, disk_t* disk, uint8_t* buffer)
{
    readcaches[currReadCache].sector = sector;
    readcaches[currReadCache].disk   = disk;
    readcaches[currReadCache].valid  = true;
    memcpy(readcaches[currReadCache].buffer, buffer, 512); // fill cache

    for (uint16_t i = 0; i < NUMREADCACHE; i++)
    {
        if (readcaches[i].sector == sector && readcaches[i].disk == disk && readcaches[i].valid && i!=currReadCache)
        {
            readcaches[i].valid = false;
        }
    }

    currReadCache++;
    currReadCache %= NUMREADCACHE;

  #ifdef _READCACHE_DIAGNOSIS_
    logReadCache();
  #endif
}

FS_ERROR sectorWrite(uint32_t sector, uint8_t* buffer, disk_t* disk)
{
  #ifdef _DEVMGR_DIAGNOSIS_
    textColor(YELLOW); printf("\n>>>>> sectorWrite: %u <<<<<", sector); textColor(TEXT);
  #endif

    for (uint16_t i = 0; i < NUMREADCACHE; i++)
    {
        if (readcaches[i].valid && readcaches[i].disk == disk && readcaches[i].sector == sector)
        {
            memcpy(readcaches[i].buffer, buffer, 512); // Update cache
        }
    }

    return disk->type->writeSector(sector, buffer, disk);
}

FS_ERROR singleSectorWrite(uint32_t sector, uint8_t* buffer, disk_t* disk)
{
    disk->accessRemaining++;
    return sectorWrite(sector, buffer, disk);
}


FS_ERROR sectorRead(uint32_t sector, uint8_t* buffer, disk_t* disk)
{
  #ifdef _DEVMGR_DIAGNOSIS_
    textColor(0x03); printf("\n>>>>> sectorRead: %u <<<<<", sector); textColor(TEXT);
  #endif

    for (uint16_t i = 0; i < NUMREADCACHE; i++)
    {
        if (readcaches[i].valid && readcaches[i].sector == sector && readcaches[i].disk == disk)
        {
          #ifdef _DEVMGR_DIAGNOSIS_
            printf("\nsector: %u <--- read from RAM Cache", readcaches[i].sector);
          #endif

            memcpy(buffer, readcaches[i].buffer, 512); // use read cache

            disk->accessRemaining--;
            return (CE_GOOD);
        }
    }

    FS_ERROR error = disk->type->readSector(sector, buffer, disk);
    if (error == CE_GOOD)
    {
        fillReadCache(sector, disk, buffer);
    }

    return error;
}

FS_ERROR singleSectorRead(uint32_t sector, uint8_t* buffer, disk_t* disk)
{
    disk->accessRemaining++;
    return sectorRead(sector, buffer, disk);
}

/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
