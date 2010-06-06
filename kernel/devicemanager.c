/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "devicemanager.h"
#include "console.h"
#include "util.h"

disk_t* disks[DISKARRAYSIZE];
port_t* ports[PORTARRAYSIZE];

void deviceManager_install()
{
    for(uint8_t i = 0; i < PORTARRAYSIZE; i++)
    {
        disks[i] = NULL;
    }
    for(uint8_t i = 0; i < DISKARRAYSIZE; i++)
    {
        ports[i] = NULL;
    }
}

void attachPort(port_t* port)
{
	for(uint8_t i = 0; i < PORTARRAYSIZE; i++)
	{
		if(ports[i] != 0)
		{
			ports[i] = port;
			return;
		}
	}
}

void attachDisk(disk_t* disk)
{
	// Later: Searching correct ID in device-File
	for(uint8_t i = 0; i < DISKARRAYSIZE; i++)
	{
		if(disks[i] == 0)
		{
			disks[i] = disk;
			return;
		}
	}
}

void removeDisk(disk_t* disk)
{
	for(uint8_t i = 0; i < DISKARRAYSIZE; i++)
	{
		if(disks[i] == disk)
		{
			disks[i] = 0;
			return;
		}
	}
}

void showPortList()
{
    printf("\n\nAvailable Ports:\nnot implemented. Help us!");
}

void showDiskList()
{
    printf("\n\nAttached Disks:");
    textColor(0x07);
    printf("\n\nType\tNumber\tSerial\tPart.\tSerial");
    printf("\n----------------------------------------------------------------------");
    textColor(0x0F);

    for (uint8_t i=0; i<DISKARRAYSIZE; i++)
    {
        if(disks[i] != NULL)
        {
            switch(disks[i]->type) // Type
            {
                case FLOPPYDISK:
                    printf("\nFDD");
                    break;
                case RAMDISK:
                    printf("\nRAMdisk");
                    break;
                case USB_MSD:
                    printf("\nUSB MSD");
                    break;
            }

            textColor(0x0E); // Number
			printf("\t%u", i);
            textColor(0x0F);

			printf("\t%u", disks[i]->serial); // Serial of disk

            for (uint8_t j = 0; j < PARTITIONARRAYSIZE; j++)
            {
				if(disks[i]->partition[j] == NULL) continue; // Empty

				if(j != 0) printf("\n\t\t\t"); // Not first, indent

				printf("\t%u", j); // Partition number

                switch(disks[i]->type)
                {
                    case FLOPPYDISK: // TODO: floppy disk device: use the current serials of the floppy disks
                        printf("\t%s", disks[i]->partition[j]->serialNumber);
                        break;
                    case RAMDISK:
                        printf("\t%s", disks[i]->partition[j]->serialNumber);
                        break;
                    case USB_MSD:                      
                        printf("\t%s", ((usb2_Device_t*)disks[i]->data)->serialNumber); // serial of device
						break;
                }
            }
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
