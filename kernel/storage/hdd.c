/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// WARNING: WIP! Do not use any of these functions on a real PC

#include "hdd.h"
#include "ata.h"
#include "util/util.h"
#include "tasking/task.h"
#include "devicemanager.h"
#include "kheap.h"
#include "serial.h"
#include "timer.h"
#include "irq.h"


static inline void wait400NS(uint16_t p) { inportb(p);inportb(p);inportb(p);inportb(p); }

static const uint32_t hddDmaPhysAdress = 0x4000;
static void* hddDmaAdress;

static inline void repinsw(uint16_t port, uint16_t* buf, uint32_t count)
{
    __asm__ volatile ("rep insw" : : "d" (port), "D" ((uint32_t)buf), "c" (count));
}

static const uint32_t ataTimeout = 30000; // Technically we have to wait 30 sec for drive spinning up

static const uint32_t ataIRQCheckInterval = 200;
static const uint32_t ataIRQRetries = 30000 / 200;

static const uint32_t ataPollInterval = 5;
static const uint32_t ataPollRetries = 30000 / 5;

// TODO: Merge the r/w-functions and make lba48 (currently removed) part of it to avoid code duplication
// Caller has to call irq_resetCounter before calling this function
static inline bool ataWaitIRQ(uint16_t channel, uint8_t errMask, uint8_t* status)
{
    IRQ_NUM_t irq;
    // Use the alternate status registers when waiting for an IRQ
    // http://wiki.osdev.org/ATA_PIO_Mode#IRQs
    uint16_t alternateStatus;

    switch(channel)
    {
    case ATACHANNEL_FIRST_MASTER:
    case ATACHANNEL_FIRST_SLAVE:
        irq = IRQ_ATA_PRIMARY;
        alternateStatus = ATA_REG_PRIMARY_DEVCONTROL;
        break;
    case ATACHANNEL_SECOND_MASTER:
    case ATACHANNEL_SECOND_SLAVE:
        irq = IRQ_ATA_SECONDARY;
        alternateStatus = ATA_REG_SECONDARY_DEVCONTROL;
        break;
    default:
        if(status)
            *status = 0xEE;
        return false;
    }

    for(int i = 0; i < ataIRQRetries; ++i)
    {
        if(!waitForIRQ(irq, ataIRQCheckInterval))
        {
            wait400NS(alternateStatus);
            uint8_t driveStatus = inportb(alternateStatus);

            if(driveStatus & errMask)
            {
                if(status)
                    *status = driveStatus;
                return false;
            }
        }
        else
        {
            if(status)
            {
                wait400NS(alternateStatus);
                uint8_t driveStatus = inportb(alternateStatus);
                *status = driveStatus;
            }
            return true;
        }
    }

    if(status)
        *status = 0xFF; // Indicate timeout

    return false;
}

static inline bool ataPoll(HDD_ATACHANNEL channel, uint8_t errMask, uint8_t goodMask, uint8_t *status)
{
    uint16_t port;

    switch(channel)
    {
    case ATACHANNEL_FIRST_MASTER:
    case ATACHANNEL_FIRST_SLAVE:
        port = ATA_PRIMARY_BASEPORT+ATA_REG_STATUSCMD;
        break;
    case ATACHANNEL_SECOND_MASTER:
    case ATACHANNEL_SECOND_SLAVE:
        port = ATA_SECONDARY_BASEPORT+ATA_REG_STATUSCMD;
        break;
    default:
        if(status)
            *status = 0xEE;
        return false;
    }

    wait400NS(port);

    uint8_t portval = inportb(port);

    for(int i = 0; i < ataPollRetries; ++i)
    {
        if(!(portval & ATA_STATUS_BSY))
        {
            if(port & goodMask)
            {
               if(status)
                    *status = portval;
               return true;
            }

            if(port & errMask)
            {
               if(status)
                   *status = portval;
               return false;
            }
        }

        sleepMilliSeconds(ataPollInterval);
        portval = inportb(port);
    }

    if(status)
        *status = 0xFF;
    return false;
}

static FS_ERROR readSectorPIOLBA28(uint32_t sector, void* buf, hdd_t* hd)
{
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

    // high nibble: 0xE = master, 0xF = slave
    // low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0xF0 : 0xE0) | ((sector >> 24) & 0x0F));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 1); // read 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    // Moved from ataWaitIRQ because it caused problems on fast emulators
    irq_resetCounter(port == ATA_PRIMARY_BASEPORT ? IRQ_ATA_PRIMARY : IRQ_ATA_SECONDARY);

    outportb(port+ATA_REG_STATUSCMD, 0x20); // Read sector(s)

    uint8_t stat = 0;

    if(!ataWaitIRQ(hd->channel, ATA_STATUS_ERR | ATA_STATUS_DF, &stat))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Read] Failed to read: %y\n", stat);
        mutex_unlock(hd->rwLock);

        // TODO: Reset drive

        return CE_BAD_SECTOR_READ;
    }
    else
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Read] Got IRQ, now checking the status: %y\n", stat);
    }

    if(!(stat & ATA_STATUS_RDY && stat & ATA_STATUS_DRQ))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO-Read28] IRQ generated with no error, but either RDY or DRQ is 0: %y\n", stat);
        mutex_unlock(hd->rwLock);

        return CE_BAD_SECTOR_READ;
    }

    repinsw(port+ATA_REG_DATA, buf, 256);

    mutex_unlock(hd->rwLock);

    return CE_GOOD;
}

static FS_ERROR writeSectorPIOLBA28(uint32_t sector, void* buf, hdd_t* hd)
{
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

    uint8_t portval;

    if(!ataPoll(hd->channel, ATA_STATUS_ERR | ATA_STATUS_DF, ATA_STATUS_RDY, &portval))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Write] Drive was not ready in 30 sec: %y\n", portval);

        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    // high nibble: 0xE = master, 0xF = slave
    // low nibble: highest 4 bit of the 28 bit lba
    outportb(port+ATA_REG_DRIVE, (slave? 0xF0 : 0xE0) | ((sector >> 24) & 0x0F));

    //outportb(port+ATA_REG_FEATURE, 0x00);

    outportb(port+ATA_REG_SECTORCOUNT, 1); // write 1 sector

    outportb(port+ATA_REG_LBALO, sector & 0xFF);
    outportb(port+ATA_REG_LBAMID, (sector >> 8) & 0xFF);
    outportb(port+ATA_REG_LBAHI, (sector >> 16) & 0xFF);

    // When writing the IRQ will fire AFTER we transmitted the data, so we'll have to poll
    outportb(port+ATA_REG_STATUSCMD, 0x30); // Write sector(s)

    irq_resetCounter(port == ATA_PRIMARY_BASEPORT ? IRQ_ATA_PRIMARY : IRQ_ATA_SECONDARY);

    if(!ataPoll(hd->channel, ATA_STATUS_ERR | ATA_STATUS_DF, ATA_STATUS_RDY | ATA_STATUS_DRQ, &portval))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Write] Failed to write (pre data send): %y\n", portval);

        mutex_unlock(hd->rwLock);
        return CE_WRITE_ERROR;
    }

    uint16_t* bufu16 = buf;

    int i;
    for(i = 0; i < 256; ++i)
    {
        __asm__ volatile("jmp .+2"); // ATA needs a 'tiny delay of jmp $+2'

        outportw(port+ATA_REG_DATA, bufu16[i]);
    }

    if(!ataWaitIRQ(hd->channel, ATA_STATUS_DF | ATA_STATUS_ERR, &portval))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Write] Failed to write (post data send): %y\n", portval);
        mutex_unlock(hd->rwLock);

        // TODO: Reset drive

        return CE_WRITE_ERROR;
    }

    // Force cache flush
    outportw(port+ATA_REG_STATUSCMD, 0xE7);

    if(!ataPoll(hd->channel, ATA_STATUS_ERR | ATA_STATUS_ERR, ATA_STATUS_RDY, &portval))
    {
        serial_log(SER_LOG_HRDDSK, "[ATA-PIO28-Write] Error during cache flush: %y\n", portval);

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

    if (inportb(port+ATA_REG_STATUSCMD) == 0xFF) // Floating port
    {
        serial_log(SER_LOG_HRDDSK, "[hdd_ATAIdentify] floating port: %d \n", (int32_t)channel);
        return false;
    }

    outportb(port+ATA_REG_DRIVE, slave ? 0xB0 : 0xA0);
    outportb(port+ATA_REG_SECTORCOUNT, 0x00);
    outportb(port+ATA_REG_LBALO, 0x00);
    outportb(port+ATA_REG_LBAMID, 0x00);
    outportb(port+ATA_REG_LBAHI, 0x00);
    outportb(port+ATA_REG_STATUSCMD, 0xEC); // IDENTIFY

    uint8_t tmp;

    wait400NS(port+ATA_REG_STATUSCMD);

    if ((tmp=inportb(port+ATA_REG_STATUSCMD)) == 0) // Drive does not exist
    {
        serial_log(SER_LOG_HRDDSK, "[hdd_ATAIdentify] drive does not exist (returned %y from port %x): %d \n",
            tmp, (uint16_t)(port+ATA_REG_STATUSCMD), (int32_t)channel);
        return false;
    }

    while(inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_BSY || !(inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_DRQ))
    {
        if (inportb(port+ATA_REG_LBAMID) || inportb(port+ATA_REG_LBAHI)) // Nonstandard device --> not supported
        {
            serial_log(SER_LOG_HRDDSK, "[hdd_ATAIdentify] nonstandard device: %d \n", (int32_t)channel);
            return false;
        }
        if (inportb(port+ATA_REG_STATUSCMD) & ATA_STATUS_ERR)
        {
            serial_log(SER_LOG_HRDDSK, "[hdd_ATAIdentify] error bit in status reg was set: %d \n", (int32_t)channel);
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

    hddDmaAdress = paging_acquirePciMemory(hddDmaPhysAdress, 1); // Reserved for later use

    uint16_t buf[256];

    mutex_t* ataPrimaryChannelLock = 0;
    mutex_t* ataSecondaryChannelLock = 0;

    for(int i = ATACHANNEL_FIRST_MASTER; i <= ATACHANNEL_SECOND_SLAVE; ++i)
    {
        if (hdd_ATAIdentify((HDD_ATACHANNEL)i, buf))
        {
            serial_log(SER_LOG_HRDDSK, "[hdd_install]Disk connected in ATA-Channel %d\n", i);
            hdd_t* hd = malloc(sizeof(hdd_t), 0, "hdd-HDD");

            hd->channel = (HDD_ATACHANNEL)i;

            hd->dma = false; // reserved
            hd->lba48 = false; // reserved

            if (i == ATACHANNEL_FIRST_MASTER || i == ATACHANNEL_FIRST_SLAVE)
            {
              if (!ataPrimaryChannelLock)
                  ataPrimaryChannelLock = mutex_create();
              hd->rwLock = ataPrimaryChannelLock;

              outportb(ATA_REG_PRIMARY_DEVCONTROL, 0x00);
            }
            else if (i == ATACHANNEL_SECOND_MASTER || i == ATACHANNEL_SECOND_SLAVE)
            {
                if (!ataSecondaryChannelLock)
                    ataSecondaryChannelLock = mutex_create();
                hd->rwLock = ataSecondaryChannelLock;

                outportb(ATA_REG_SECONDARY_DEVCONTROL, 0x00);
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

            serial_log(SER_LOG_HRDDSK, "[hdd_install] Size of disk at channel %d is %d\n", i, hd->drive->insertedDisk->size);

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
            return CE_INVALID_ARGUMENT;

        return writeSectorPIOLBA28(sector, buf, device->data);
    }
    else return CE_INVALID_ARGUMENT;
}

FS_ERROR hdd_readSectorPIO(uint32_t sector, void* buf, disk_t* device)
{
    if (sector < device->size / 512)
    {
        if (sector > 0x0FFFFFFF)
            return CE_INVALID_ARGUMENT;

        return readSectorPIOLBA28(sector, buf, device->data);
    }
    else return CE_INVALID_ARGUMENT;
}


/*
* Copyright (c) 2012-2013 The PrettyOS Project. All rights reserved.
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