/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "devicemanager.h"
#include "console.h"
#include "util.h"

MSD_t*   MSD_Array[MSDARRAYSIZE];
uint8_t  globalMSD = 0;

void MSDmanager_install()
{
    printf("\nMass Storage Device Manager installed.\n");
    for(uint8_t i=0; i<MSDARRAYSIZE; i++)
    {
        MSD_Array[i] = NULL;
    }
}

void addToMSDmanager(MSD_t* msd)
{
    MSD_Array[globalMSD] = msd;
    msd->globalMSD = globalMSD;
    globalMSD++;
}

void deleteFromMSDmanager(MSD_t* msd)
{
    MSD_Array[msd->globalMSD] = NULL;
}

uint32_t getMSDVolumeNumber()
{
	// Should recognize MSDs. So it has to be "rewritten" later.
    static uint32_t globalMSDVolume = 0;
    return ++globalMSDVolume; // volume 0 is reserved for the PrettyOS media
}

void showMSDAttached()
{
    printf("\n\nAttached Mass Storage Devices:");
    textColor(0x07);
    printf("\n\nType\tDrive\tPart.\tSerial(device)\tPort");
    printf("\n----------------------------------------------------------------------");
    textColor(0x0F);

    for (uint8_t i=0; i<globalMSD; i++)
    {
        if(MSD_Array[i] != NULL)
        {
            if ( MSD_Array[i]->connected )
            {
                switch( MSD_Array[i]->type )
                {
                case FLOPPYDISK:
                    printf("\nFDD");
                    break;
                case RAMDISK:
                    printf("\nRAMdisk");
                    break;
                case USBMSD:
                    printf("\nUSB MSD");
                    break;
                }

                for (uint8_t j = 0; j < MSD_Array[i]->numberOfPartitions; j++)
                {
                    textColor(0x0E);
                    printf("\t%u", MSD_Array[i]->Partition[j]->volumeNumber);
                    textColor(0x0F);

                    switch( MSD_Array[i]->type )
                    {
                    case FLOPPYDISK: // TODO: floppy disk device: use the current serials of the floppy disks
                        printf("\t%d\t%s", j, (MSD_Array[i]->Partition[j])->serialNumber);
                        break;
                    case RAMDISK:
                        printf("\t%d\t%s", j, (MSD_Array[i]->Partition[j])->serialNumber);
                        break;
                    case USBMSD:                        
                        printf("\t%d\t%s", j, MSD_Array[i]->usb2Device->serialNumber); // serial of device
                        if (strlen(MSD_Array[i]->usb2Device->serialNumber) < 8) printf("\t");
                        break;
                    }
                }

                if ( (MSD_Array[i]->portNumber) == 255 )
                {
                    // do nothing
                }
                else
                {
                    printf("\t%u", MSD_Array[i]->portNumber+1);
                }
            }
        }
        else
        {
            textColor(0x07);
            printf("\ndeleted"); // TEST
            textColor(0x0F);
        }
    }
    textColor(0x07);
    printf("\n----------------------------------------------------------------------\n");
    textColor(0x0F);
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
