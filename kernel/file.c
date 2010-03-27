/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "file.h"
#include "flpydsk.h"
#include "fat12.h"
#include "task.h"
#include "kheap.h"

int32_t initCache() /// floppy
{
    int32_t retVal0 = flpydsk_read_ia(0,track0,TRACK);
    int32_t retVal1 = flpydsk_read_ia(1,track1,TRACK);

    if (retVal0 || retVal1)
        return -1;
    else
        return 0;
}

int32_t flpydsk_load(const char* name, const char* ext) /// load file <--- TODO: make it general!
{
    int32_t retVal;
    struct file f;
    uint32_t firstCluster = 0;

    flpydsk_control_motor(true);
    retVal = initCache();
    if (retVal)
    {
        settextcolor(12,0);
        printf("track0 & track1 read error.\n");
        settextcolor(2,0);
    }

    printf("Load and execute ");
    settextcolor(14,0);
    printf("-->%s.%s<--",name,ext);
    settextcolor(2,0);
    printf(" from floppy disk\n");

    firstCluster = search_file_first_cluster(name,ext,&f); // now working with cache
    if (firstCluster==0)
    {
        settextcolor(4,0);
        printf("file is not ok (no valid first cluster)\n");
        settextcolor(15,0);
        flpydsk_control_motor(false);
        return -1;
    }

    /// ********************************** File memory prepared **************************************///

    uint8_t* file = malloc(f.size,PAGESIZE); /// TODO: free allocated memory, if program is finished
    printf("FileSize: %d Byte, 1st Cluster: %d, Memory: %X\n",f.size, f.firstCluster, file);
    sleepSeconds(3); // show screen output

    ///************** FAT ********************///
    // printf("\nFAT1 parsed 12-bit-wise: ab cd ef --> dab efc\n");

    /// TODO: read only entries which are necessary for file_ia
    ///       combine reading FAT entry and data sector

    // whole FAT is read from index 0 to FATMAXINDEX (= 2849 = 0xB21)
    for (uint32_t i=0;i<FATMAXINDEX;i++)
    {
        read_fat(&fat_entry[i], i, FAT1_SEC, track0);
    }

    // load file to memory
    retVal = file_ia(fat_entry,firstCluster,file); // read sectors of file
    ///************** FAT ********************///

    #ifdef _DIAGNOSIS_
        printf("\nFile content (start of first 5 clusters): ");
        printf("\n1st sector:\n"); for (uint16_t i=   0;i<  20;i++) {printf("%y ",file[i]);}
        printf("\n2nd sector:\n"); for (uint16_t i= 512;i< 532;i++) {printf("%y ",file[i]);}
        printf("\n3rd sector:\n"); for (uint16_t i=1024;i<1044;i++) {printf("%y ",file[i]);}
        printf("\n4th sector:\n"); for (uint16_t i=1536;i<1556;i++) {printf("%y ",file[i]);}
        printf("\n5th sector:\n"); for (uint16_t i=2048;i<2068;i++) {printf("%y ",file[i]);}
        printf("\n\n");
    #endif

    if (!retVal)
    {
        char Buffer[10];
        strcpy(Buffer, name);
        strcat(Buffer, ".");
        strcat(Buffer, ext);
        /// START TASK AND INCREASE TASKCOUNTER
        if (elf_exec(file, f.size, Buffer)) // execute loaded file
        {
            userTaskCounter++;         // an additional user-program has been started
            free(file);
        }
        else
        {
            settextcolor(14,0);
            printf("File has unknown format and will not be executed.\n");
            settextcolor(15,0);
            // other actions?
            free(file); // still needed in kernel?
        }
    }
    else if (retVal==-1)
    {
        settextcolor(4,0);
        printf("file was not executed due to FAT error.\n");
        settextcolor(15,0);
    }
    flpydsk_control_motor(false);
    sleepSeconds(3); // show screen output
    return 0;
}

int32_t flpydsk_write(const char* name, const char* ext, void* memory, uint32_t size)
{
    char bufName[8], bufExt[3];
    strncpy(bufName,name,8);
    strncpy(bufExt,ext, 3);

    // ensure that there is no zero in the string! Use 0x20 (SPACE).
    for (int32_t i=0;i<8;i++)
    {
        if (bufName[i]<0x20)
        {
            bufName[i] = 0x20;
        }
    }

    for (int32_t i=0;i<3;i++)
    {
        if (bufExt[i]<0x20)
        {
            bufExt[i] = 0x20;
        }
    }

    int32_t retVal=0;
    // struct file f;
    uint32_t firstCluster = 0; // no valid value for a file
    uint8_t  freeRootDirEntry = 0xFF;

    flpydsk_control_motor(true);
    retVal = initCache();
    if (retVal)
    {
        settextcolor(12,0);
        printf("track0 & track1 read error.\n");
        settextcolor(2,0);
    }
    // how many floppy disc sectors are needed?
    uint32_t neededSectors=0;
    if (size%512 == 0)
    {
        neededSectors = (size/512);
    }
    else
    {
        neededSectors = (size/512)+1;
    }
    printf("test file written to FDD: %d bytes, %d sectors needed.\n", size, neededSectors);

    // search first free cluster
    //  whole FAT is read from index 2 to maximum FATMAXINDEX (= 2849 = 0xB21)
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
    uint32_t rootdirStart = ROOT_SEC*0x200;
    for (uint16_t i=0;i<ROOT_DIR_ENTRIES;i++)
    {
        if (track1[(rootdirStart-9216)+i*0x20]==0x00)
        {
            printf("free root dir entry nr. %d\n\n",i);
            freeRootDirEntry = i;
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20   ], (void*)bufName,       8); // write name
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+ 8], (void*)bufExt,        3); // write extension
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+26], (void*)&firstCluster, 2); // write first cluster
            memcpy((void*)&track1[(rootdirStart-9216)+i*0x20+28], (void*)&size,         4); // write size
                           track1[(rootdirStart-9216)+i*0x20+11] = ATTR_ARCHIVE;            // archive
            break;
        }
        else
        {
            // printf("occupied root dir entry nr. %d first byte: %y\n",i,track1[(rootdirStart-9216/*track0*/)+i*0x20]);
        }
    }
    if (freeRootDirEntry==0xFF)
    {
        settextcolor(4,0);
        printf("no free root dir entry found.\n\n");
        settextcolor(15,0);
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

    // FAT chain
    // printf("\nFAT chain:\n");

/*
    // seems to be not necessary
    // write firstCluster from memory to floppy disk
    uint32_t timeout = 2; // limit
    while (flpydsk_write_ia(firstCluster+ADD, memory, SECTOR) != 0)
    {
        retVal = -1;
        timeout--;
        printf("error write_sector. attempts left: %d\n",timeout);
        if (timeout<=0)
        {
            printf("timeout\n");
            break;
        }
    }
    if (retVal==0)
    {
        printf("success write_sector %d (firstCluster)\t", firstCluster+ADD);
    }
*/

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
               printf("error write_sector %d.\n", FileCluster[i]+ADD);
           }
           else
           {
               // printf("success write_sector %d\t", FileCluster[i]+ADD);
           }
       }
    }
    //printf("\n\n");


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

    // motor off
    flpydsk_control_motor(false);

    return 0;
}

int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* fileData) /// load file
{
    uint8_t a[512];
    uint32_t sectornumber;
    uint32_t i, pos;  // i for FAT-index, pos for data position

    // copy first cluster
    sectornumber = firstCluster+ADD;
    printf("\n\n1st sector: %d\n",sectornumber);

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;
    while (flpydsk_read_ia(sectornumber,a,SECTOR) != 0)
    {
        retVal = -1;
        timeout--;
        printf("error read_sector. attempts left: %d\n",timeout);
        if (timeout<=0)
        {
            printf("timeout\n");
            break;
        }
    }
    if (retVal==0)
    {
        /// printf("success read_sector.\n");
    }

    memcpy((void*)fileData, (void*)a, 512);

    /// find second cluster and chain in fat /// FAT12 specific
    pos=0;
    i = firstCluster;
    while (fatEntry[i]!=0xFFF)
    {
        printf("\ni: %d FAT-entry: %x\t",i,fatEntry[i]);
        if ((fatEntry[i]<3) || (fatEntry[i]>MAX_BLOCK))
        {
            printf("FAT-error.\n");
            return -1;
        }

        // copy data from chain
        pos++;
        sectornumber = fatEntry[i]+ADD;
        printf("sector: %d\t",sectornumber);

        timeout = 2; // limit
        retVal  = 0;
        while (flpydsk_read_ia(sectornumber,a,SECTOR) != 0)
        {
            retVal = -1;
            timeout--;
            printf("error read_sector. attempts left: %d\n",timeout);
            if (timeout<=0)
            {
                printf("timeout\n");
                break;
            }
        }
        if (retVal==0)
        {
            /// printf("success read_sector.\n");
        }

        memcpy((void*)(fileData+pos*512), (void*)a, 512);

        // search next cluster of the fileData
        i = fatEntry[i];
    }
    printf("\n");
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
