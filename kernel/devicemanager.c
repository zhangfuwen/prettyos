/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "devicemanager.h"
#include "fsmanager.h"
#include "console.h"
#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "usb2_msd.h"
#include "flpydsk.h"
#include "usb2.h"
#include "timer.h"
#include "fat12.h"

disk_t* disks[DISKARRAYSIZE];
port_t* ports[PORTARRAYSIZE];
partition_t* systemPartition;

uint32_t startSectorPartition = 0;
extern uint32_t usbMSDVolumeMaxLBA;

portType_t FDD,        USB,     RAM;
diskType_t FLOPPYDISK, USB_MSD, RAMDISK;

void deviceManager_install(/*partition_t* system*/)
{
    FDD.motorOff = &flpydsk_motorOff;
    USB.motorOff = 0;
    RAM.motorOff = 0;

    USB_MSD.readSector     = &usbRead;
    USB_MSD.writeSector    = &usbWrite;
    FLOPPYDISK.readSector  = &flpydsk_readSector;
    FLOPPYDISK.writeSector = &flpydsk_writeSector;
    RAMDISK.readSector     = 0;
    RAMDISK.writeSector    = 0;

    memset(disks, 0, DISKARRAYSIZE*sizeof(disk_t*));
    memset(ports, 0, PORTARRAYSIZE*sizeof(port_t*));
    //systemPartition = system;
}

void deviceManager_checkDrives()
{
    for(int i = 0; i < PORTARRAYSIZE; i++)
    {
        if(ports[i] != 0 && ports[i]->type->motorOff != 0 && ports[i]->insertedDisk->accessRemaining == 0)
            ports[i]->type->motorOff(ports[i]->data);
    }
}

void attachPort(port_t* port)
{
    for(uint8_t i=0; i<PORTARRAYSIZE; i++)
    {
        if(ports[i] == NULL)
        {
            ports[i] = port;
            return;
        }
    }
}

void attachDisk(disk_t* disk)
{
    // Later: Searching correct ID in device-File
    for(uint8_t i=0; i<DISKARRAYSIZE; i++)
    {
        if(disks[i] == NULL)
        {
            disks[i] = disk;
            return;
        }
    }
}

void removeDisk(disk_t* disk)
{
    for(uint8_t i=0; i<DISKARRAYSIZE; i++)
    {
        if(disks[i] == disk)
        {
            disks[i] = NULL;
            return;
        }
    }
}

void showPortList()
{
    textColor(0x02);
    printf("\n\nAvailable ports:");
    textColor(0x07);
    printf("\n\nType\tNumber\tName\t\tInserted disk");
    printf("\n----------------------------------------------------------------------");
    textColor(0x0F);

    for (uint8_t i = 0; i < PORTARRAYSIZE; i++)
    {
        if (ports[i] != NULL)
        {
            if(ports[i]->type == &FDD) // Type
                    printf("\nFDD ");
            else if(ports[i]->type == &RAM)
                    printf("\nRAM ");
            else if(ports[i]->type == &USB)
                    printf("\nUSB 2.0");

            textColor(0x0E);
            printf("\t%c", 'A'+i); // number
            textColor(0x0F);
            printf("\t%s", ports[i]->name); // The ports name

            if (ports[i]->insertedDisk != NULL)
            {
                flpydsk_refreshVolumeNames();
                if(ports[i]->type != &FDD || *ports[i]->insertedDisk->name != 0) // Floppy workaround
                    printf("\t%s",ports[i]->insertedDisk->name); // Attached disk
                else putch('\t');
            }
            else
            {
               printf("\t-");
            }
        }
    }
    textColor(0x07);
    printf("\n----------------------------------------------------------------------\n");
    textColor(0x0F);
}

void showDiskList()
{
    textColor(0x02);
    printf("\n\nAttached disks:");
    textColor(0x07);
    printf("\n\nType\tNumber\tName\t\tPart.\tSerial");
    printf("\n----------------------------------------------------------------------");
    textColor(0x0F);

    for (uint8_t i=0; i<DISKARRAYSIZE; i++)
    {
        if (disks[i] != NULL)
        {
            if(disks[i]->type == &FLOPPYDISK) /// Todo: Move to flpydsk.c, name set on floppy insertion
            {
                flpydsk_refreshVolumeNames();
            }
            if(disks[i]->type == &FLOPPYDISK && *disks[i]->name == 0) continue; // Floppy workaround

            if(disks[i]->type == &FLOPPYDISK) // Type
                printf("\nFloppy");
            else if(disks[i]->type == &RAMDISK)
                printf("\nRAMdisk");
            else if(disks[i]->type == &USB_MSD)
                printf("\nUSB MSD");

            textColor(0x0E);
            printf("\t%u", i+1); // Number
            textColor(0x0F);

            printf("\t%s", disks[i]->name);   // Name of disk
            if (strlen(disks[i]->name) < 8) { printf("\t"); }

            for (uint8_t j = 0; j < PARTITIONARRAYSIZE; j++)
            {
                if (disks[i]->partition[j] == NULL) continue; // Empty

                if (j!=0) printf("\n\t\t\t"); // Not first, indent

                printf("\t%u", j); // Partition number

                if(disks[i]->type == &FLOPPYDISK) // Serial
                {
                    //HACK
                    free(disks[i]->partition[j]->serial);
                    disks[i]->partition[j]->serial = malloc(13, 0);
                    disks[i]->partition[j]->serial[12] = 0;
                    strncpy(disks[i]->partition[j]->serial, disks[i]->name, 12); // TODO: floppy disk device: use the current serials of the floppy disks
                    printf("\t%s", disks[i]->partition[j]->serial);
                }
                else if(disks[i]->type == &RAMDISK)
                    printf("\t%s", disks[i]->partition[j]->serial);
                else if(disks[i]->type == &USB_MSD)
                    printf("\t%s", ((usb2_Device_t*)disks[i]->data)->serialNumber);
                /// ifs should be changed to:
                ///printf("\t%s", disks[i]->partition[j]->serial); // serial of partition
            }
        }
    }
    textColor(0x07);
    printf("\n----------------------------------------------------------------------\n");
    textColor(0x0F);
}

const char* getFilename(const char* path)
{
    while(*path != '/' && *path != '|' && *path != '\\')
    {
        path++;
        if(*path == 0)
        {
            return(0);
        }
    }
    path++;
    return(path);
}

FS_ERROR loadFile(const char* filename, partition_t* part);
FS_ERROR executeFile(const char* path)
{
    partition_t* part = getPartition(path);
    if(part == 0)
    {
        return(CE_FILE_NOT_FOUND);
    }
    if(path == 0)
    {
        return(CE_INVALID_FILENAME);
    }

    // Load File
    waitForKeyStroke(); /// Why does loading from USB fail, if its not there?

    FAT_file_t* file = fopen(path, "r")->data;
    if(file != 0)
    {
        void* filebuffer = malloc(file->size, 0);
        FAT_fread(file, filebuffer, file->size);

        char tempfilename[12];
        tempfilename[11] = 0;
        strncpy(tempfilename, file->name, 11);
        elf_exec(filebuffer, file->size, tempfilename); // try to execute

        FAT_fclose(file);

        waitForKeyStroke(); /// Why does a #PF appear without it?
        return(CE_GOOD);
    }
    return(CE_FILE_NOT_FOUND);
}

partition_t* getPartition(const char* path)
{
    size_t length = strlen(path);
    char Buffer[10];

    int16_t PortID = -1;
    int16_t DiskID = -1;
    uint8_t PartitionID = 0;
    for(size_t i = 0; i < length && path[i]; i++)
    {
        if(path[i] == ':' || path[i] == '-')
        {
            strncpy(Buffer, path, i);
            Buffer[i] = 0;
            if(isalpha(Buffer[0]))
            {
                PortID = toUpper(Buffer[0]) - 'A';
            }
            else
            {
                DiskID = atoi(Buffer);
            }
            for(size_t j = i+1; j < length && path[j]; j++)
            {
                if(path[j] == ':' || path[j] == '-')
                {
                    strncpy(Buffer, path+i+1, j-i-1);
                    Buffer[j-i-1] = 0;
                    PartitionID = atoi(Buffer);
                    break;
                }
                if(!isdigit(path[j]) || path[j] == '/' || path[j] == '|' || path[j] == '\\')
                {
                    break;
                }
            }
            break;
        }
        if(!isalnum(path[i]))
        {
            return(0);
        }
    }
    if(PortID != -1 && PortID < PORTARRAYSIZE)
    {
        return(ports[PortID]->insertedDisk->partition[PartitionID]);
    }
    else
    {
        if(DiskID == 0)
        {
            return(systemPartition);
        }
        if(DiskID > 0 && DiskID <= DISKARRAYSIZE)
        {
            return(disks[DiskID-1]->partition[PartitionID]);
        }
    }
    return(0);
}

FS_ERROR analyzeBootSector(void* buffer, partition_t* part) // for first tests only
{
    uint32_t volume_bytePerSector;
    uint8_t  volume_type = FAT32;
    uint8_t  volume_SecPerClus;
    uint16_t volume_maxroot;
    uint32_t volume_fatsize;
    uint8_t  volume_fatcopy;
    uint32_t volume_firstSector;
    uint32_t volume_fat;
    uint32_t volume_root;
    uint32_t volume_data;
    uint32_t volume_maxcls;
    char     volume_serialNumber[4];

    struct boot_sector* sec0 = (struct boot_sector*)buffer;

    char SysName[9];
    char FATn[9];
    strncpy(SysName, sec0->SysName,   8);
    strncpy(FATn,    sec0->Reserved2, 8);
    SysName[8] = 0;
    FATn[8]    = 0;

    //HACK
    FAT_partition_t* FATpart = malloc(sizeof(FAT_partition_t), 0);
    part->data = FATpart;
    FATpart->part = part;
    part->type = &FAT;


    // This is a FAT description
    if (sec0->charsPerSector == 0x200 && sec0->FATcount > 0) // 512 byte per sector, at least one FAT
    {
        printf("\nOEM name:           %s"    ,SysName);
        printf("\tbyte per sector:        %u",sec0->charsPerSector);
        printf("\nsectors per cluster:    %u",sec0->SectorsPerCluster);
        printf("\treserved sectors:       %u",sec0->ReservedSectors);
        printf("\nnumber of FAT copies:   %u",sec0->FATcount);
        printf("\tmax root dir entries:   %u",sec0->MaxRootEntries);
        printf("\ntotal sectors (<65536): %u",sec0->TotalSectors1);
        printf("\tmedia Descriptor:       %y",sec0->MediaDescriptor);
        printf("\nsectors per FAT:        %u",sec0->SectorsPerFAT);
        printf("\tsectors per track:      %u",sec0->SectorsPerTrack);
        printf("\nheads/pages:            %u",sec0->HeadCount);
        printf("\thidden sectors:         %u",sec0->HiddenSectors);
        printf("\ntotal sectors (>65535): %u",sec0->TotalSectors2);
        printf("\tFAT 12/16:              %s",FATn);

        volume_bytePerSector = sec0->charsPerSector;
        volume_SecPerClus    = sec0->SectorsPerCluster;
        volume_maxroot       = sec0->MaxRootEntries;
        volume_fatsize       = sec0->SectorsPerFAT;
        volume_fatcopy       = sec0->FATcount;
        volume_firstSector   = startSectorPartition; // sec0->HiddenSectors; <--- not sure enough
        volume_fat           = volume_firstSector + sec0->ReservedSectors;
        volume_root          = volume_fat + volume_fatcopy * volume_fatsize;
        volume_data          = volume_root + volume_maxroot /(volume_bytePerSector/NUMBER_OF_BYTES_IN_DIR_ENTRY);
        volume_maxcls        = (usbMSDVolumeMaxLBA - volume_data - volume_firstSector) / volume_SecPerClus;


        uint32_t volume_FatRootDirCluster = 0; // only FAT32

        uint32_t volume_ClustersPerRootDir = volume_maxroot/(16 * volume_SecPerClus);; // FAT12 and FAT16

        startSectorPartition = 0; // important!

        if (FATn[0] != 'F') // FAT32
        {
            if ( ((uint8_t*)buffer)[0x52] == 'F' && ((uint8_t*)buffer)[0x53] == 'A' && ((uint8_t*)buffer)[0x54] == 'T' &&
                 ((uint8_t*)buffer)[0x55] == '3' && ((uint8_t*)buffer)[0x56] == '2')
            {
                printf("\nThis is a volume formated with FAT32 ");
                volume_type = FAT32;

                for (uint8_t i=0; i<4; i++)
                {
                    volume_serialNumber[i] = ((char*)buffer)[67+i]; // byte 67-70
                }
                printf("and serial number: %y %y %y %y", volume_serialNumber[0], volume_serialNumber[1], volume_serialNumber[2], volume_serialNumber[3]);

                volume_FatRootDirCluster = ((uint32_t*)buffer)[11]; // byte 44-47
                printf("\nThe root dir starts at cluster %u (expected: 2).", volume_FatRootDirCluster);

                volume_maxroot = 512; // i did not find this info about the maxium root dir entries, but seems to be correct
                volume_ClustersPerRootDir = volume_maxroot/(16 * volume_SecPerClus);

                printf("\nSectors per FAT: %u.", ((uint32_t*)buffer)[9]);  // byte 36-39
                volume_fatsize = ((uint32_t*)buffer)[9];
                volume_root    = volume_firstSector + sec0->ReservedSectors + volume_fatcopy * volume_fatsize + volume_SecPerClus * (volume_FatRootDirCluster-2);
                volume_data    = volume_root;
                volume_maxcls  = (usbMSDVolumeMaxLBA - volume_data - volume_firstSector) / volume_SecPerClus;
            }
        }
        else if (FATn[0] == 'F' && FATn[1] == 'A' && FATn[2] == 'T' && FATn[3] == '1' && FATn[4] == '6') // FAT16
        {
            printf("\nThis is a volume formated with FAT16 ");
            volume_type = FAT16;

            for (uint8_t i=0; i<4; i++)
            {
                volume_serialNumber[i] = ((char*)buffer)[39+i]; // byte 39-42
            }
            printf("and serial number: %y %y %y %y", volume_serialNumber[0], volume_serialNumber[1], volume_serialNumber[2], volume_serialNumber[3]);


        }
        else if (FATn[0] == 'F' && FATn[1] == 'A' && FATn[2] == 'T' && FATn[3] == '1' && FATn[4] == '2')
        {
            printf("\nThis is a volume formated with FAT12.\n");
            volume_type = FAT12;
        }

        ///////////////////////////////////////////////////
        // store the determined volume data to partition //
        ///////////////////////////////////////////////////

        FATpart->sectorSize = volume_bytePerSector;
        part->buffer        = malloc(volume_bytePerSector, 0);
        part->subtype       = volume_type;
        FATpart->SecPerClus = volume_SecPerClus;
        FATpart->maxroot    = volume_maxroot;
        FATpart->fatsize    = volume_fatsize;
        FATpart->fatcopy    = volume_fatcopy;
        part->start         = volume_firstSector;
        FATpart->fat        = volume_fat;    // reservedSectors
        FATpart->root       = volume_root;   // reservedSectors + 2*SectorsPerFAT
        FATpart->dataLBA    = volume_data;   // reservedSectors + 2*SectorsPerFAT + MaxRootEntries/DIRENTRIES_PER_SECTOR <--- FAT16
        FATpart->maxcls     = volume_maxcls;
        part->mount         = true;
        FATpart->FatRootDirCluster = volume_FatRootDirCluster;

        //HACK
        free(part->serial);
        part->serial = malloc(5, 0);
        part->serial[4] = 0;
        strncpy(part->serial, volume_serialNumber, 4); // ID of the partition
    }

    else if ( *((uint16_t*)((uint8_t*)buffer+444)) == 0x0000 )
    {
        textColor(0x0E);
        if ( *((uint8_t*)buffer+510)==0x55 && *((uint8_t*)buffer+511)==0xAA )
        {
            printf("\nThis seems to be a Master Boot Record:");

            textColor(0x0F);
            uint32_t discSignature = *((uint32_t*)((uint8_t*)buffer+440));
            printf("\n\nDisc Signature: %X\t",discSignature);

            uint8_t partitionTable[4][16];
            for (uint8_t i=0;i<4;i++) // four tables
            {
                for (uint8_t j=0;j<16;j++) // 16 bytes
                {
                    partitionTable[i][j] = *((uint8_t*)buffer+446+i*j);
                }
            }
            printf("\n");
            for (uint8_t i=0;i<4;i++) // four tables
            {
                if ( *((uint32_t*)(&partitionTable[i][0x0C])) != 0 && *((uint32_t*)(&partitionTable[i][0x0C])) != 0x80808080 ) // number of sectors
                {
                    textColor(0x0E);
                    printf("\npartition entry %u:",i);
                    if (partitionTable[i][0x00] == 0x80)
                    {
                        printf("  bootable");
                    }
                    else
                    {
                        printf("  not bootable");
                    }
                    textColor(0x0F);
                    printf("\ntype:               %y", partitionTable[i][0x04]);
                    textColor(0x07);
                    printf("\nfirst sector (CHS): %u %u %u", partitionTable[i][0x01],partitionTable[i][0x02],partitionTable[i][0x03]);
                    printf("\nlast  sector (CHS): %u %u %u", partitionTable[i][0x05],partitionTable[i][0x06],partitionTable[i][0x07]);
                    textColor(0x0F);

                    startSectorPartition = *((uint32_t*)(&partitionTable[i][0x08]));
                    printf("\nstart sector:       %u", startSectorPartition);

                    printf("\nnumber of sectors:  %u", *((uint32_t*)(&partitionTable[i][0x0C])));
                    printf("\n");
                }
                else
                {
                    textColor(0x0E);
                    printf("\nno partition table %u",i);
                    textColor(0x0F);
                }
            }
        }
    }
    else
    {
        textColor(0x0C);
        if ( ((uint8_t*)buffer)[0x3] == 'N' && ((uint8_t*)buffer)[0x4] == 'T' && ((uint8_t*)buffer)[0x5] == 'F' && ((uint8_t*)buffer)[0x6] == 'S' )
        {
            printf("\nThis seems to be a volume formatted with NTFS.");
        }
        else
        {
            printf("\nThis seems to be neither a FAT description nor a MBR.");
        }
        textColor(0x0F);
        return CE_UNSUPPORTED_FS;
    }
    return CE_GOOD;
}


FS_ERROR sectorWrite(uint32_t sector, uint8_t* buffer, partition_t* part)
{
  #ifdef _DEVMGR_DIAGNOSIS_
    textColor(0x0E); printf("\n>>>>> sectorWrite: %u <<<<<",lba); textColor(0x0F);
  #endif
    return part->disk->type->writeSector(sector, buffer, part->disk->data);
}
FS_ERROR singleSectorWrite(uint32_t sector, uint8_t* buffer, partition_t* part)
{
    part->disk->accessRemaining++;
    return sectorWrite(sector, buffer, part);
}

FS_ERROR sectorRead(uint32_t sector, uint8_t* buffer, partition_t* part)
{
  #ifdef _DEVMGR_DIAGNOSIS_
    textColor(0x03); printf("\n>>>>> sectorRead: %u <<<<<",lba); textColor(0x0F);
  #endif
    return part->disk->type->readSector(sector, buffer, part->disk->data);
}
FS_ERROR singleSectorRead(uint32_t sector, uint8_t* buffer, partition_t* part)
{
    part->disk->accessRemaining++;
    return sectorRead(sector, buffer, part);
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
