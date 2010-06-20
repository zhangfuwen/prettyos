/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "devicemanager.h"
#include "console.h"
#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "fat12.h"
#include "usb2_msd.h"
#include "flpydsk.h"
#include "usb2.h"
#include "timer.h"

disk_t* disks[DISKARRAYSIZE];
port_t* ports[PORTARRAYSIZE];
partition_t* systemPartition;

uint32_t startSectorPartition = 0;
extern uint32_t usbMSDVolumeMaxLBA;

diskType_t FLOPPYDISK;
diskType_t USB_MSD;
diskType_t RAMDISK;

void deviceManager_install(/*partition_t* system*/)
{
    USB_MSD.readSector = &usbRead;
    FLOPPYDISK.readSector = &flpydsk_readSector;
    FLOPPYDISK.writeSector = &flpydsk_writeSector;

    memset(disks, 0, DISKARRAYSIZE*sizeof(disks));
    memset(ports, 0, PORTARRAYSIZE*sizeof(ports));
    //systemPartition = system;
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
            switch (ports[i]->type) // Type
            {
                case FDD:
                    printf("\nFDD ");
                    flpydsk_refreshVolumeNames(); // Floppy workaround
                    break;
                case RAM:
                    printf("\nRAM ");
                    break;
                case USB:
                    printf("\nUSB 2.0");
                    break;
            }
            
            textColor(0x0E);
            printf("\t%c", i+'A'); // number
            textColor(0x0F);
            printf("\t%s", ports[i]->name); // The ports name

            if (ports[i]->insertedDisk != NULL)
            {
                if(ports[i]->type != FDD || *ports[i]->insertedDisk->name != 0) // Floppy workaround
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
                    strncpy(disks[i]->partition[j]->serialNumber, disks[i]->name, 12); // TODO: floppy disk device: use the current serials of the floppy disks
                    printf("\t%s", disks[i]->partition[j]->serialNumber);
                }
                else if(disks[i]->type == &RAMDISK)
                    printf("\t%s", disks[i]->partition[j]->serialNumber);
                else if(disks[i]->type == &USB_MSD)
                    printf("\t%s", ((usb2_Device_t*)disks[i]->data)->serialNumber);
                /// ifs should be changed to:
                ///printf("\t%s", disks[i]->partition[j]->serialNumber); // serial of partition
            }
        }
    }
    textColor(0x07);
    printf("\n----------------------------------------------------------------------\n");
    textColor(0x0F);
}

FS_ERROR execute(const char* path)
{
    partition_t* part = getPartition(path);
    if(part == NULL)
    {
        return(CE_FILE_NOT_FOUND);
    }
    while(*path != '/' && *path != '|' && *path != '\\')
    {
        path++;
        if(*path == 0)
        {
            return(CE_INVALID_FILENAME);
        }
    }
    path++;
    return(loadFile(path, part));
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

FS_ERROR loadFile(const char* filename, partition_t* part)
{
    char partitionType[6];
    switch(part->type)
    {
        case 1:
            strcpy(partitionType,"FAT12");
            break;
        case 2:
            strcpy(partitionType,"FAT16");
            break;
        case 3:
            strcpy(partitionType,"FAT32");
            break;
        default:
            strcpy(partitionType,"???");
            break;
    }

    textColor(0x03);
    printf("\nbuffer:     %X", part->buffer);
    printf("\ntype:       %s", partitionType);
    printf("\nSecPerClus: %u", part->SecPerClus);
    printf("\nmaxroot:    %u", part->maxroot);
    printf("\nfatsize:    %u", part->fatsize);
    printf("\nfatcopy:    %u", part->fatcopy);
    printf("\nfirsts:     %u", part->firsts);
    printf("\nfat:        %u", part->fat);
    printf("\nroot:       %u", part->root);
    printf("\ndata:       %u", part->data);
    printf("\nmaxcls:     %u", part->maxcls);
    printf("\nmount:      %s", part->mount ? "yes" : "no");
    printf("\nserial:     %s", part->serialNumber);
    textColor(0x0F);

    waitForKeyStroke(); /// Why does loading from USB fail, if its not there?

    // file name
    FILE toCompare;
    FILE* fileptrTest = &toCompare;

    memset(fileptrTest->name, ' ', 11);

    int j = 0;
    for(; j < 8; j++)
    {
        if(filename[j] == '.')
        {
            j++;
            break;
        }
        if(filename[j] == 0) break;
        fileptrTest->name[j] = filename[j];
    }
    for(int i = 8; j < 11; i++, j++)
    {
        if(filename[j] == 0) break;
        fileptrTest->name[i] = filename[j];
    }
    if(fileptrTest->name[8] == ' ' && fileptrTest->name[9] == ' ' && fileptrTest->name[10] == ' ')
    {
        fileptrTest->name[8] = 'E'; fileptrTest->name[9] = 'L'; fileptrTest->name[10] = 'F';
    }

    // file to search
    FILE dest;
    dest.volume  = part;
    dest.dirclus = 0;
    dest.entry   = 0;
    if (dest.volume->type == FAT32)
    {
        dest.dirclus = dest.volume->FatRootDirCluster; 
    }

    uint8_t retVal = searchFile(&dest, fileptrTest, LOOK_FOR_MATCHING_ENTRY, 0); // searchFile(FILE* fileptrDest, FILE* fileptrTest, uint8_t cmd, uint8_t mode)
    if (retVal == CE_GOOD)
    {
        textColor(0x0A);
        printf("\n\nThe file was found on the device: %s", part->serialNumber);

        printf("\nnumber of entry in root dir: ");
        textColor(0x0E);
        printf("%u\n", dest.entry); // number of file entry "searched.xxx"

        fdopen(&dest, &(dest.entry), 'r');
        
        void* filebuffer = malloc(dest.size,PAGESIZE);
        fread(&dest, filebuffer, dest.size);

        elf_exec(filebuffer, dest.size, dest.name); // try to execute
 
        fclose(&dest);
    }

    waitForKeyStroke(); /// Why does a #PF appear without it?
    return(retVal);
}

int32_t analyzeBootSector(void* buffer, partition_t* part) // for first tests only
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


    // This is a FAT description
    if ((sec0->charsPerSector == 0x200) && (sec0->FATcount > 0)) // 512 byte per sector, at least one FAT
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
            if ( (((uint8_t*)buffer)[0x52] == 'F') && (((uint8_t*)buffer)[0x53] == 'A') && (((uint8_t*)buffer)[0x54] == 'T')
              && (((uint8_t*)buffer)[0x55] == '3') && (((uint8_t*)buffer)[0x56] == '2'))
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
        else // FAT12/16
        {
            if ( (FATn[0] == 'F') && (FATn[1] == 'A') && (FATn[2] == 'T') && (FATn[3] == '1') && (FATn[4] == '6') )
            {
                printf("\nThis is a volume formated with FAT16 ");
                volume_type = FAT16;                

                for (uint8_t i=0; i<4; i++)
                {
                    volume_serialNumber[i] = ((char*)buffer)[39+i]; // byte 39-42
                }
                printf("and serial number: %y %y %y %y", volume_serialNumber[0], volume_serialNumber[1], volume_serialNumber[2], volume_serialNumber[3]);


            }
            else if ( (FATn[0] == 'F') && (FATn[1] == 'A') && (FATn[2] == 'T') && (FATn[3] == '1') && (FATn[4] == '2') )
            {
                printf("\nThis is a volume formated with FAT12.\n");
                volume_type = FAT12;
            }
        }

        ///////////////////////////////////////////////////
        // store the determined volume data to partition //
        ///////////////////////////////////////////////////

        part->sectorSize     = volume_bytePerSector;
        part->buffer         = malloc(part->sectorSize,PAGESIZE); 
        part->type           = volume_type;        
        part->SecPerClus     = volume_SecPerClus;
        part->maxroot        = volume_maxroot;
        part->fatsize        = volume_fatsize;
        part->fatcopy        = volume_fatcopy;
        part->firsts         = volume_firstSector;
        part->fat            = volume_fat;    // reservedSectors
        part->root           = volume_root;   // reservedSectors + 2*SectorsPerFAT
        part->data           = volume_data;   // reservedSectors + 2*SectorsPerFAT + MaxRootEntries/DIRENTRIES_PER_SECTOR <--- FAT16
        part->maxcls         = volume_maxcls;
        part->mount          = true;
        part->FatRootDirCluster   = volume_FatRootDirCluster; 
        strncpy(part->serialNumber,volume_serialNumber,4); // ID of the partition
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
                if ( (*((uint32_t*)(&partitionTable[i][0x0C])) != 0) && (*((uint32_t*)(&partitionTable[i][0x0C])) != 0x80808080) ) // number of sectors
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
        if ( (((uint8_t*)buffer)[0x3] == 'N') && (((uint8_t*)buffer)[0x4] == 'T') && (((uint8_t*)buffer)[0x5] == 'F') && (((uint8_t*)buffer)[0x6] == 'S') )
        {
            printf("\nThis seems to be a volume formatted with NTFS.");
        }
        else
        {
            printf("\nThis seems to be neither a FAT description nor a MBR.");
        }
        return -1;
        textColor(0x0F);
    }
    return 0;
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
