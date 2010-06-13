/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "file.h"
#include "util.h"
#include "flpydsk.h"
#include "fat12.h"
#include "task.h"
#include "kheap.h"

// reserve caches for 5 floppy disk tracks (18*512 byte)
uint8_t cache[5][9216];
uint8_t cacheTrackNumber[5];

static int32_t cacheFirstTracks() /// for flpydsk_write
{
    int32_t retVal0 = flpydsk_read_ia(0,track0,TRACK);
    int32_t retVal1 = flpydsk_read_ia(1,track1,TRACK);

    if (retVal0 || retVal1)
        return -1;
    else
        return 0;
}

int32_t flpydsk_write(const char* name, const char* ext, void* memory, uint32_t size)
{
    char bufName[8], bufExt[3];
    strncpy(bufName, name, 8);
    strncpy(bufExt, ext, 3);

    // ensure that there is no zero in the string! Use 0x20 (SPACE).
    for (uint8_t i = 0; i < 8; i++)
    {
        if (bufName[i]<0x20)
        {
            bufName[i] = 0x20;
        }
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        if (bufExt[i]<0x20)
        {
            bufExt[i] = 0x20;
        }
    }

    // struct file f;

    int32_t retVal = cacheFirstTracks();
    if (retVal)
    {
        textColor(0x0C);
        printf("track0 & track1 read error.\n");
        textColor(0x02);
    }
    // how many floppy disc sectors are needed?
    uint32_t neededSectors = size/512;
    if (size%512 != 0)
    {
        neededSectors++;
    }
    printf("\nSave data to Floppy Disk: %u bytes, %u sectors needed.\n", size, neededSectors);

    uint32_t firstCluster = 0; // no valid value for a file
    // search first free cluster
    // whole FAT is read from index 2 to maximum FATMAXINDEX (= 2849 = 0xB21)
    for (uint32_t i=2;i<FATMAXINDEX;i++)
    {
        read_fat(&fat_entry[i], i, FAT1_SEC, track0);
        if ((fat_entry[i]==0) && (fat_entry[i-1]==LAST_ENTRY_IN_FAT_CHAIN))
        {
            firstCluster = i; // free and behind a FAT chain
            break;
        }
    }

    // search free place in root dir and write entry
    uint8_t  freeRootDirEntry = 0xFF;
    uint32_t rootdirStart = ROOT_SEC*0x200;
    for (uint16_t i=0;i<ROOT_DIR_ENTRIES;i++)
    {
        if ((track1[(rootdirStart-9216)+i*0x20]==0x00)||(track1[(rootdirStart-9216)+i*0x20]==0xE5)) // 00h: nothing in it E5h: deleted entry
        {
            printf("free root dir entry nr. %u\n\n",i);
            freeRootDirEntry = i;
            memset((void*)&track1[(rootdirStart-9216)+i*0x20], 0x00,                   32); // 32 times zero
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20   ], (void*)bufName,       8); // write name
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+ 8], (void*)bufExt,        3); // write extension
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+26], (void*)&firstCluster, 2); // write first cluster
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+28], (void*)&size,         4); // write size
                           track1[(rootdirStart-9216)+i*0x20+11] = ATTR_ARCHIVE;            // archive
            break;
        }
        else
        {
            // printf("occupied root dir entry nr. %u first byte: %y\n",i,track1[(rootdirStart-9216/*track0*/)+i*0x20]);
        }
    }
    if (freeRootDirEntry==0xFF)
    {
        textColor(0x04);
        printf("no free root dir entry found.\n\n");
        textColor(0x0F);
    }

    // search "neededSectors" free sectors
    uint32_t FileCluster[neededSectors];
    int32_t counter = 0;
    for (uint32_t i=firstCluster;i<FATMAXINDEX;i++)
    {
        if (counter>=neededSectors)
        {
            break;
        }
        read_fat(&fat_entry[i], i, FAT1_SEC, track0);
        if (fat_entry[i]==0)
        {
            FileCluster[counter] = i;
            counter++;
        }
    }

    for (uint32_t i=0;i<neededSectors;i++)
    {
        if (i==(neededSectors-1))
        {
            fat_entry[FileCluster[i]] = LAST_ENTRY_IN_FAT_CHAIN;
        }
        else
        {
            fat_entry[FileCluster[i]] = FileCluster[i+1];
        }

       // write FAT cluster indices to FAT1 at track0 buffer
       // write_fat(int32_t fat,  int32_t index,  int32_t st_sec, uint8_t* buffer)
       write_fat(fat_entry[FileCluster[i]], FileCluster[i], FAT1_SEC, track0);

       // write sectors from memory to floppy disk
       if (FileCluster[i] != LAST_ENTRY_IN_FAT_CHAIN)
       {
           retVal = flpydsk_write_ia(FileCluster[i]+ADD, (memory+i*0x200), SECTOR);
           if (retVal != 0)
           {
               printf("error write_sector %u.\n", FileCluster[i]+ADD);
           }
           else
           {
               // printf("success write_sector %u\t", FileCluster[i]+ADD);
           }
       }
    }

    // write FAT1 to FAT2
    for (int32_t i=0;i<0x1000;i++)
    {
        track0[0x1400+i] = track0[0x200+i];
    }
    for (int32_t i=0;i<0x200;i++)
    {
        track1[i] = track0[0x1000+i];
    }

    // write track0, track1 from memory to floppy disk
    flpydsk_write_ia(0, track0, TRACK);
    flpydsk_write_ia(1, track1, TRACK);

    // free memory, if necessary

    return 0;
}

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
