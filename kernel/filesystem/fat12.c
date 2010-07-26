/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "storage/flpydsk.h"
#include "fat12.h"
#include "video/console.h"
#include "util.h"

// cache memory for tracks 0 and 1
uint8_t track0[9216], track1[9216];

int32_t flpydsk_read_directory()
{
    int32_t error = -1; // return value

    memset((void*)DMA_BUFFER, 0x0, 0x2400); // 18 sectors: 18 * 512 = 9216 = 0x2400

    flpydsk_initialize_dma(); // important, if you do not use the unreliable autoinit bit of DMA

    /// TODO: change to read_ia(...)!
    int32_t retVal = flpydsk_read_sector(19, true); // start at 0x2600: root directory (14 sectors)
    if (retVal != 0)
    {
        printf("\nread error: %d\n",retVal);
    }
    printf("<Floppy Disc Directory>\n");

    for (uint8_t i=0;i<ROOT_DIR_ENTRIES;++i)       // 224 Entries * 32 Byte
    {
        if (((*((uint8_t*)(DMA_BUFFER + i*32)))      != 0x00) && // free from here on
            ((*((uint8_t*)(DMA_BUFFER + i*32)))      != 0xE5) && // 0xE5 deleted = free
            ((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) != 0x0F))   // 0x0F part of long file name
        {
            error = 0;
            int32_t start = DMA_BUFFER + i*32; // name
            int32_t count = 8;
            int32_t letters = 0;
            int8_t* end = (int8_t*)(start+count);
            for (; count != 0; --count)
            {
                if (*(end-count) != 0x20) // empty space in file name
                {
                    printf("%c",*(end-count));
                    letters++;
                }
            }            

            start = DMA_BUFFER + i*32 + 8; // extension

            if ((((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x08) == 0x08) ||  // volume label
                 ((*((uint8_t*) (start))   == 0x20) &&
                   (*((uint8_t*) (start+1)) == 0x20) &&
                   (*((uint8_t*) (start+2)) == 0x20)))                         // extension == three 'space'
            {
                // do nothing
            }
            else
            {
                printf("."); // usual separator between file name and file extension
            }

            count = 3;
            end = (int8_t*)(start+count);
            for (; count!=0; --count)
            {
                printf("%c",*(end-count));
            }
            
            if (letters<4) printf("\t"); 

            // filesize
            printf("\t%d byte", *((uint32_t*)(DMA_BUFFER + i*32 + 28)));

            // attributes
            printf("\t");
            if (*((uint32_t*)(DMA_BUFFER + i*32 + 28))<100)               printf("\t");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x08) == 0x08) printf(" (vol)");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x10) == 0x10) printf(" (dir)");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x01) == 0x01) printf(" (r/o)");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x02) == 0x02) printf(" (hid)");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x04) == 0x04) printf(" (sys)");
            if (((*((uint8_t*)(DMA_BUFFER + i*32 + 11))) & 0x20) == 0x20) printf(" (arc)");

            // 1st cluster: physical sector number  =  33  +  FAT entry number  -  2  =  FAT entry number  +  31
            printf("  1st sector: %d", *((uint16_t*)(DMA_BUFFER + i*32 + 26))+31);
            printf("\n"); // next root directory entry
        }
    }
    printf("\n");
    return error;
}


/*****************************************************************************
  The following functions are derived from free source code of the dynacube team.
  which was published in the year 2004 at http://www.dynacube.net
*****************************************************************************/

static int32_t flpydsk_prepare_boot_sector(struct boot_sector *bs) /// FAT12
{
    uint8_t a[512];

    int32_t retVal = flpydsk_read_ia(BOOT_SEC,a,SECTOR);
    if (retVal!=0)
    {
        printf("\nread error: %d\n",retVal);
    }

    int32_t i=0;
    for (uint8_t j=0;j<3;j++)
    {
        a[j]=bs->jumpBoot[j];
    }

    i+= 3;
    for (uint8_t j=0;j<8;j++,i++)
    {
        a[i]= bs->SysName[j];
    }

    a[i]   = BYTE1(bs->charsPerSector);
    a[i+1] = BYTE2(bs->charsPerSector);
    i+=2;

    a[i++] = bs->SectorsPerCluster;

    a[i]   = BYTE1(bs->ReservedSectors);
    a[i+1] = BYTE2(bs->ReservedSectors);
    i+=2;

    a[i++] = bs->FATcount;

    a[i]   = BYTE1(bs->MaxRootEntries);
    a[i+1] = BYTE2(bs->MaxRootEntries);
    i+=2;

    a[i]   = BYTE1(bs->TotalSectors1);
    a[i+1] = BYTE2(bs->TotalSectors1);
    i+=2;

    a[i++] = bs->MediaDescriptor;

    a[i]   = BYTE1(bs->SectorsPerFAT);
    a[i+1] = BYTE2(bs->SectorsPerFAT);
    i+=2;

    a[i]   = BYTE1(bs->SectorsPerTrack);
    a[i+1] = BYTE2(bs->SectorsPerTrack);
    i+=2;

    a[i]   = BYTE1(bs->HeadCount);
    a[i+1] = BYTE2(bs->HeadCount);
    i+=2;

    a[i]   = BYTE1(bs->HiddenSectors);
    a[i+1] = BYTE2(bs->HiddenSectors);
    a[i+2] = BYTE3(bs->HiddenSectors);
    a[i+3] = BYTE4(bs->HiddenSectors);
    i+=4;

    a[i]   = BYTE1(bs->TotalSectors2);
    a[i+1] = BYTE2(bs->TotalSectors2);
    a[i+2] = BYTE3(bs->TotalSectors2);
    a[i+3] = BYTE4(bs->TotalSectors2);
    i+=4;

    a[i++] = bs->DriveNumber;
    a[i++] = bs->Reserved1;
    a[i++] = bs->ExtBootSignature;

    a[i]   = BYTE1(bs->VolumeSerial);
    a[i+1] = BYTE2(bs->VolumeSerial);
    a[i+2] = BYTE3(bs->VolumeSerial);
    a[i+3] = BYTE4(bs->VolumeSerial);
    i+=4;

    for (uint8_t j=0;j<11;j++,i++)
    {
        a[i] = bs->VolumeLabel[j];
    }

    for (uint8_t j=0;j<8;j++,i++)
    {
        a[i] = bs->Reserved2[j];
    }

    // boot signature
    a[510]= 0x55; a[511]= 0xAA;

    // prepare sector 0 of track 0
    for (uint16_t k=0;k<511;k++)
    {
        track0[k] = a[k];
    }
    return 0;
}



int32_t flpydsk_format(char* vlab) /// VolumeLabel /// FAT12 and Floppy specific /// TODO: make general
{
    struct boot_sector b;
    uint8_t a[512];
    uint8_t i;

    // int32_t dt, tm; // for VolumeSerial

    printf("\n\nFormat process started.\n");

    for (i=0;i<11;i++)
    {
        if (vlab[i] == '\0')
        {
            break;
        }
    }

    for (uint8_t j=i;j<11;j++)
    {
        vlab[j] = ' ';
    }

    b.jumpBoot[0] = 0xeb;
    b.jumpBoot[1] = 0x3c;
    b.jumpBoot[2] = 0x90;

    b.SysName[0] = 'M';
    b.SysName[1] = 'S';
    b.SysName[2] = 'W';
    b.SysName[3] = 'I';
    b.SysName[4] = 'N';
    b.SysName[5] = '4';
    b.SysName[6] = '.';
    b.SysName[7] = '1';

    b.charsPerSector    =  512;
    b.SectorsPerCluster =    1;
    b.ReservedSectors   =    1;
    b.FATcount          =    2;
    b.MaxRootEntries    =  ROOT_DIR_ENTRIES;
    b.TotalSectors1     = 2880;
    b.MediaDescriptor   = 0xF0;
    b.SectorsPerFAT     =    9;
    b.SectorsPerTrack   =   18;
    b.HeadCount         =    2;
    b.HiddenSectors     =    0;
    b.TotalSectors2     =    0;
    b.DriveNumber       =    0;
    b.Reserved1         =    0;
    b.ExtBootSignature  = 0x29;

    /*
    dt = form_date();
    tm = form_time();
    dt = ((dt & 0xff) << 8) | ((dt & 0xFF00) >> 8);
    tm = ((tm & 0xff) << 8) | ((tm & 0xFF00) >> 8);
    */
    b.VolumeSerial = 0x12345678;
    //b.VolumeSerial = tm << 16 + dt;

    for (uint8_t j=0;j<11;j++)
    {
        b.VolumeLabel[j] = vlab[j];
    }
    b.Reserved2[0] = 'F';
    b.Reserved2[1] = 'A';
    b.Reserved2[2] = 'T';
    b.Reserved2[3] = '1';
    b.Reserved2[4] = '2';
    b.Reserved2[5] = ' ';
    b.Reserved2[6] = ' ';
    b.Reserved2[7] = ' ';


    /// bootsector
    flpydsk_prepare_boot_sector(&b);

    /// prepare FATs
    for (uint16_t j=512;j<9216;j++)
    {
        track0[j] = 0;
    }
    for (uint16_t j=0;j<9216;j++)
    {
        track1[j] = 0;
    }

    a[0]=0xF0; a[1]=0xFF; a[2]=0xFF;
    for (uint8_t j=0;j<3;j++)
    {
        track0[FAT1_SEC*512+j]=a[j]; // FAT1 starts at 0x200  (sector  1)
        track0[FAT2_SEC*512+j]=a[j]; // FAT2 starts at 0x1400 (sector 10)
    }

    /// prepare first root directory entry (volume label)
    for (uint8_t j=0;j<11;j++)
    {
        track1[512+j] = vlab[j];
    }
    track1[512+11] = ATTR_VOLUME_ID | ATTR_ARCHIVE;

    for (uint16_t j=7680;j<9216;j++)
    {
        track1[j] = 0xF6; // format ID of MS Windows
    }

    /// write track 0 & track 1
    printf("writing tracks 1 & 2\n");
    flpydsk_write_ia(0,track0,TRACK);
    flpydsk_write_ia(1,track1,TRACK);
    printf("Quickformat complete.\n\n");

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
