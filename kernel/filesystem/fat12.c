/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "fat12.h"
#include "storage/flpydsk.h"
#include "video/console.h"
#include "util.h"
#include "fat.h"


int32_t flpydsk_read_directory()
{
    // Read track
    static uint8_t track[9216]; // Cache for one track
    memset(track, 0, 9216);
    floppyDrive[0]->drive.insertedDisk->accessRemaining += 18;
    for (int i = 0; i < 18; i++) // Read one track
    {
        flpydsk_readSector(19+i, track+i*0x200, floppyDrive[0]); // start at 0x2600: root directory (14 sectors)
    }

    textColor(HEADLINE);
    puts("\n<Floppy Disk - Root Directory>");
    textColor(TABLE_HEADING);
    puts("\nFile\t\tSize (Bytes)\tAttributes\n");
    textColor(TEXT);

    // Analyze root dir
    FAT_dirEntry_t* dirEntry = (void*)track;
    for (uint8_t i = 0; i < 224; i++) // 224 root directory entries in FAT12
    {
        if (dirEntry[i].Name[0] == DIR_EMPTY) break; // free from here on

        if ((uint8_t)dirEntry[i].Name[0] != DIR_DEL &&            // Entry is not deleted
           (dirEntry[i].Attr & ATTR_LONG_NAME) != ATTR_LONG_NAME) // Entry is not part of long file name (VFAT)
        {
            // Filename
            size_t letters = 0;
            for (uint8_t j = 0; j < 8; j++)
            {
                if (dirEntry[i].Name[j] != 0x20) // Empty space
                {
                    putch(dirEntry[i].Name[j]);
                    letters++;
                }
            }
            if (!(dirEntry[i].Attr & ATTR_VOLUME) && // No volume label
                 dirEntry[i].Extension[0] != 0x20 && dirEntry[i].Extension[1] != 0x20 && dirEntry[i].Extension[2] != 0x20) // Has extension
            {
                putch('.');
            }

            for (uint8_t j = 0; j < 3; j++)
            {
                if (dirEntry[i].Extension[j] != 0x20) // Empty space
                {
                    putch(dirEntry[i].Extension[j]);
                    letters++;
                }
            }

            if (letters < 7) putch('\t');

            // Filesize
            if (!(dirEntry[i].Attr & ATTR_VOLUME))
                printf("\t%d\t\t", dirEntry[i].FileSize);
            else
                puts("\t\t\t");

            // Attributes
            if (dirEntry[i].Attr & ATTR_VOLUME)    puts("(vol) ");
            if (dirEntry[i].Attr & ATTR_DIRECTORY) puts("(dir) ");
            if (dirEntry[i].Attr & ATTR_READ_ONLY) puts("(r/o) ");
            if (dirEntry[i].Attr & ATTR_HIDDEN)    puts("(hid) ");
            if (dirEntry[i].Attr & ATTR_SYSTEM)    puts("(sys) ");
            if (dirEntry[i].Attr & ATTR_ARCHIVE)   puts("(arc)" );

            putch('\n');
        }
    }
    putch('\n');
    return 0;
}


/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
