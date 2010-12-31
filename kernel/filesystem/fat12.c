/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "storage/flpydsk.h"
#include "fat12.h"
#include "video/console.h"
#include "util.h"

int32_t flpydsk_read_directory()
{
    int32_t error = -1; // return value

    static uint8_t track[9216]; // Cache for one track
    memset(track, 0, 9216);
    floppyDrive[0]->drive.insertedDisk->accessRemaining += 18;
    for(int i = 0; i < 18; i++) // Read one track
    {
        flpydsk_readSector(19+i, track+i*0x200, floppyDrive[0]); // start at 0x2600: root directory (14 sectors)
    }
    printf("<Floppy Disk - Root Directory>\n");

    for (uint8_t i=0;i<ROOT_DIR_ENTRIES;++i)       // 224 Entries * 32 Byte
    {
        if (((*((uint8_t*)(track + i*32)))      != 0x00) && // free from here on
            ((*((uint8_t*)(track + i*32)))      != 0xE5) && // 0xE5 deleted = free
            ((*((uint8_t*)(track + i*32 + 11))) != 0x0F))   // 0x0F part of long file name
        {
            error = 0;
            int32_t start = (uintptr_t)track + i*32; // name
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

            start = (uintptr_t)track + i*32 + 8; // extension

            if ((((*((uint8_t*)(track + i*32 + 11))) & 0x08) == 0x08) || // volume label
                 (((uint8_t*)start)[0] == 0x20 &&
                  ((uint8_t*)start)[1] == 0x20 &&
                  ((uint8_t*)start)[2] == 0x20))                          // extension == three 'space'
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
            printf("\t%d byte", *((uint32_t*)(track + i*32 + 28)));

            // attributes
            printf("\t");
            if (*((uint32_t*)(track + i*32 + 28))<100)               printf("\t");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x08) == 0x08) printf(" (vol)");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x10) == 0x10) printf(" (dir)");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x01) == 0x01) printf(" (r/o)");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x02) == 0x02) printf(" (hid)");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x04) == 0x04) printf(" (sys)");
            if (((*((uint8_t*)(track + i*32 + 11))) & 0x20) == 0x20) printf(" (arc)");

            // 1st cluster: physical sector number  =  33  +  FAT entry number  -  2  =  FAT entry number  +  31
            printf("  1st sector: %d", *((uint16_t*)(track + i*32 + 26))+31);
            printf("\n"); // next root directory entry
        }
    }
    printf("\n");
    return error;
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
