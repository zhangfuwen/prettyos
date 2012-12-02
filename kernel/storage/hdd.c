/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

//WARNING: WIP! Do not use any of these functions on a real PC

#include "hdd.h"
#include "ata.h"
#include "util/util.h"
#include "tasking/task.h"
#include "devicemanager.h"
#include "kheap.h"
#include "serial.h"
#include "timer.h"


static inline void wait400NS(uint16_t p) { inportb(p);inportb(p);inportb(p);inportb(p); }

static const uint32_t hddDmaPhysAdress = 0x4000;
static void* hddDmaAdress;

static inline void repinsw(uint16_t port, uint16_t* buf, uint32_t count)
{
    __asm__ volatile ("rep insw" : : "d" (port), "D" ((uint32_t)buf), "c" (count));
}

static FS_ERROR readSectorPIOLBA28(uint32_t sector, void* buf, hdd_t* hd)
{
    //TODO: Use IRQ instead of polling

    uint16_t port = 0;
    bool slave = false;

    switch(hd->channel)
    {
    case ATACHANNEL_FIRST_MASTER:
        port = ATA_PRIMARY_BASEPORT;
        break;
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT;
        slave = true;
        break;
    case ATACHANNEL_SECOND_MASTER:
        port = ATA_SECONDARY_BASEPORT;
        break;
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT;
        slave = true;
        break;
    default:
        return CE_INVALID_ARGUMENT;
    }

    mutex_lock(hd->rwLock);

    //high nibble: 0xE = master, 0xF = slave
    //low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0xF0 : 0xE0) | ((sector >> 24) & 0x0F));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 1); //read 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    outportb(port+ATA_REG_STATUSCMD, 0x20); //Read sector(s)

    wait400NS(port+ATA_REG_STATUSCMD);

    while((!((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ) &&
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_RDY)) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_BAD_SECTOR_READ;
    }

    repinsw(port+ATA_REG_DATA, buf, 256);

    mutex_unlock(hd->rwLock);

    return CE_GOOD;
}

static FS_ERROR writeSectorPIOLBA28(uint32_t sector, void* buf, hdd_t* hd)
{
    //TODO: Use IRQ instead of polling

    uint16_t port = 0;
    bool slave = false;

    switch(hd->channel)
    {
    case ATACHANNEL_FIRST_MASTER:
        port = ATA_PRIMARY_BASEPORT;
        break;
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT;
        slave = true;
        break;
    case ATACHANNEL_SECOND_MASTER:
        port = ATA_SECONDARY_BASEPORT;
        break;
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT;
        slave = true;
        break;
    default:
        return CE_INVALID_ARGUMENT;
    }

    mutex_lock(hd->rwLock);

    //high nibble: 0xE = master, 0xF = slave
    //low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0xF0 : 0xE0) | ((sector >> 24) & 0x0F));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 1); //write 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    outportb(port+ATA_REG_STATUSCMD, 0x30); //Write sector(s)

    wait400NS(port+ATA_REG_STATUSCMD);

    while(!((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ) &&
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_RDY)) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    uint16_t* bufu16 = buf;

    int i;
    for(i = 0; i < 256; ++i)
    {
        nop(); //ATA needs a 'tiny delay of jmp $+2'
        nop(); //I hope 4x nop is enough, too.
        nop();
        nop();

        outportw(port+ATA_REG_DATA, bufu16[i]);
    }

    //Force cache flush
    outportw(port+ATA_REG_STATUSCMD, 0xE7);

    wait400NS(port+ATA_REG_STATUSCMD);

    while((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_BSY) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    mutex_unlock(hd->rwLock);

    return CE_GOOD;
}

static FS_ERROR readSectorPIOLBA48(uint16_t sector_high, uint32_t sector, void* buf, hdd_t* hd)
{
    //TODO: Use IRQ instead of polling

    if (!hd->lba48)
        return CE_INVALID_ARGUMENT;

    uint16_t port = 0;
    bool slave = false;

    switch(hd->channel)
    {
    case ATACHANNEL_FIRST_MASTER:
        port = ATA_PRIMARY_BASEPORT;
        break;
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT;
        slave = true;
        break;
    case ATACHANNEL_SECOND_MASTER:
        port = ATA_SECONDARY_BASEPORT;
        break;
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT;
        slave = true;
        break;
    default:
        return CE_INVALID_ARGUMENT;
    }

    mutex_lock(hd->rwLock);

    //high nibble: 0xE = master, 0xF = slave
    //low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0x50 : 0x40));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 0); //sector count high byte

    outportb(port+ATA_REG_LBALO, (sector >> 24) & 0xFF);
    outportb(port+ATA_REG_LBAMID, sector_high & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector_high >> 8) & 0xFF);

    outportb(port+ATA_REG_SECTORCOUNT, 1); //read 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    outportb(port+ATA_REG_STATUSCMD, 0x24); //Read sector(s)

    wait400NS(port+ATA_REG_STATUSCMD);

    while((!((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ) &&
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_RDY)) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_BAD_SECTOR_READ;
    }

    repinsw(port+ATA_REG_DATA, buf, 256);

    mutex_unlock(hd->rwLock);

    return CE_GOOD;
}

static FS_ERROR writeSectorPIOLBA48(uint16_t sector_high, uint32_t sector, void* buf, hdd_t* hd)
{
    //TODO: Use IRQ instead of polling

    if (!hd->lba48)
        return CE_INVALID_ARGUMENT;

    uint16_t port = 0;
    bool slave = false;

    switch(hd->channel)
    {
    case ATACHANNEL_FIRST_MASTER:
        port = ATA_PRIMARY_BASEPORT;
        break;
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT;
        slave = true;
        break;
    case ATACHANNEL_SECOND_MASTER:
        port = ATA_SECONDARY_BASEPORT;
        break;
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT;
        slave = true;
        break;
    default:
        return CE_INVALID_ARGUMENT;
    }

    mutex_lock(hd->rwLock);

    //high nibble: 0xE = master, 0xF = slave
    //low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0x50 : 0x40));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 0); //sector count high byte

    outportb(port+ATA_REG_LBALO, (sector >> 24) & 0xFF);
    outportb(port+ATA_REG_LBAMID, sector_high & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector_high >> 8) & 0xFF);

    outportb(port+ATA_REG_SECTORCOUNT, 1); //read 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    outportb(port+ATA_REG_STATUSCMD, 0x34); //Write sector(s)

    wait400NS(port+ATA_REG_STATUSCMD);

    while(!((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ) &&
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_RDY)) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    uint16_t* bufu16 = buf;

    int i;
    for(i = 0; i < 256; ++i)
    {
        nop(); //ATA needs a 'tiny delay of jmp $+2'
        nop(); //I hope 4x nop is enough, too.
        nop();
        nop();

        outportw(port+ATA_REG_DATA, bufu16[i]);
    }

    //Force cache flush
    outportw(port+ATA_REG_STATUSCMD, 0xE7);

    wait400NS(port+ATA_REG_STATUSCMD);

    while((inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_BSY) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR) ||
        (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF))
        switch_context();

    if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR || inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DF)
    {
        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    mutex_unlock(hd->rwLock);

    return CE_GOOD;
}

static bool hdd_ATAIdentify(HDD_ATACHANNEL channel, uint16_t *output)
{
    uint16_t port = 0;
    bool slave = false;

    switch(channel)
    {
    case ATACHANNEL_FIRST_MASTER:
        port = ATA_PRIMARY_BASEPORT;
        break;
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT;
        slave = true;
        break;
    case ATACHANNEL_SECOND_MASTER:
        port = ATA_SECONDARY_BASEPORT;
        break;
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT;
        slave = true;
        break;
    default:
        return false;
    }

    if (inportb(port+ATA_REG_STATUSCMD) == 0xFF) //Floating port
    {
        serial_log(SER_LOG_HRDDSK, "\t[hdd_ATAIdentify] floating port: %d \n", (int32_t)channel);
        return false;
    }

    outportb(port+ATA_REG_DRIVE, slave ? 0xB0 : 0xA0);
    outportb(port+ATA_REG_SECTORCOUNT, 0x00);
    outportb(port+ATA_REG_LBALO, 0x00);
    outportb(port+ATA_REG_LBAMID, 0x00);
    outportb(port+ATA_REG_LBAHI, 0x00);
    outportb(port+ATA_REG_STATUSCMD, 0xEC); //IDENTIFY

    uint8_t tmp;

    wait400NS(port+ATA_REG_STATUSCMD);

    if ((tmp=inportb(port+ATA_REG_STATUSCMD)) == 0) //Drive does not exist
    {
        serial_log(SER_LOG_HRDDSK, "\t[hdd_ATAIdentify] drive does not exist (returned %y from port %x): %d \n",
            tmp, (uint16_t)(port+ATA_REG_STATUSCMD), (int32_t)channel);
        return false;
    }

    while(inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_BSY || !(inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ))
    {
        if (inportb(port+ATA_REG_LBAMID) || inportb(port+ATA_REG_LBAHI)) //Nonstandard device --> not supported
        {
            serial_log(SER_LOG_HRDDSK, "\t[hdd_ATAIdentify] nonstandard device: %d \n", (int32_t)channel);
            return false;
        }
        if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR)
        {
            serial_log(SER_LOG_HRDDSK, "\t[hdd_ATAIdentify] error bit in status reg was set: %d \n", (int32_t)channel);
            return false;
        }
    }

    repinsw(port+ATA_REG_DATA, output, 256);

    return true;
}

void hdd_install(void)
{
    outportb(ATA_REG_PRIMARY_DEVCONTROL, 0x00);
    outportb(ATA_REG_SECONDARY_DEVCONTROL, 0x00);

    hddDmaAdress = paging_acquirePciMemory(hddDmaPhysAdress, 1); //Reserved for later use

    uint16_t buf[256];

    mutex_t* ataPrimaryChannelLock = 0;
    mutex_t* ataSecondaryChannelLock = 0;

    for(int i = ATACHANNEL_FIRST_MASTER; i <= ATACHANNEL_SECOND_SLAVE; ++i)
    {
        if (hdd_ATAIdentify((HDD_ATACHANNEL)i, buf))
        {
            serial_log(SER_LOG_HRDDSK, "\t[hdd_install]Disk connected in ATA-Channel %d\n", i);
            hdd_t* hd = malloc(sizeof(hdd_t), 0, "hdd-HDD");

            hd->channel = (HDD_ATACHANNEL)i;

            hd->dma = false; //reserved
            hd->lba48 = false; //reserved

            if (i == ATACHANNEL_FIRST_MASTER || i == ATACHANNEL_FIRST_SLAVE)
            {
              if (!ataPrimaryChannelLock)
                  ataPrimaryChannelLock = mutex_create();
              hd->rwLock = ataPrimaryChannelLock;
            }
            else if (i == ATACHANNEL_SECOND_MASTER || i == ATACHANNEL_SECOND_SLAVE)
            {
                if (!ataSecondaryChannelLock)
                    ataSecondaryChannelLock = mutex_create();
                hd->rwLock = ataSecondaryChannelLock;
            }

            hd->drive = malloc(sizeof(port_t), 0, "harddsk-Port");

            hd->drive->type         = &HDD;
            hd->drive->data         = hd;
            hd->drive->insertedDisk = malloc(sizeof(disk_t), 0, "hdd-Disk");

            hd->drive->insertedDisk->type            = &HDDPIODISK;
            hd->drive->insertedDisk->data            = hd;
            hd->drive->insertedDisk->port            = hd->drive;
            // buf[60] and buf[61] give the total size of the lba28 adressable sectors
            hd->drive->insertedDisk->size            = (*(uint32_t*)&buf[60])*512;
            hd->drive->insertedDisk->headCount       = 0;
            hd->drive->insertedDisk->secPerTrack     = 0;
            hd->drive->insertedDisk->sectorSize      = 512;
            hd->drive->insertedDisk->accessRemaining = 0;

            for(int j = 0; j < PARTITIONARRAYSIZE; ++j)
                hd->drive->insertedDisk->partition[i] = 0;

            serial_log(SER_LOG_HRDDSK, "\t[hdd_install] Size of disk at channel %d is %d\n", i, hd->drive->insertedDisk->size);

            attachDisk(hd->drive->insertedDisk); // disk == hard disk
            attachPort(hd->drive);
        }
    }
}

FS_ERROR hdd_writeSectorPIO(uint32_t sector, void* buf, disk_t* device)
{
    if (sector < device->size / 512)
    {
        if (sector > 0x0FFFFFFF)
            return writeSectorPIOLBA48(0, sector, buf, device->data);;

        return writeSectorPIOLBA28(sector, buf, device->data);
    }
    else return CE_INVALID_ARGUMENT;
}

FS_ERROR hdd_readSectorPIO(uint32_t sector, void* buf, disk_t* device)
{
    if (sector < device->size / 512)
    {
        if (sector > 0x0FFFFFFF)
            return readSectorPIOLBA48(0, sector, buf, device->data);

        return readSectorPIOLBA28(sector, buf, device->data);
    }
    else return CE_INVALID_ARGUMENT;
}


/*
* Copyright (c) 2012 The PrettyOS Project. All rights reserved.
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