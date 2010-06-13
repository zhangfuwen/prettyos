/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "flpydsk.h"
#include "util.h"
#include "console.h"
#include "cmos.h"
#include "timer.h"
#include "irq.h"
#include "kheap.h"
#include "fat.h"


// detailed infos about FDC and FAT12:
// http://www.isdaman.com/alsos/hardware/fdc/floppy.htm
// http://lowlevel.brainsware.org/wiki/index.php/FDC#Data_Address_Mark
// http://www.brokenthorn.com/Resources/OSDev20.html
// http://www.eit.lth.se/fileadmin/eit/courses/eit015/FAT12Description.pdf

/*****************************************************************************
  This module is based on the open source code
  of the OSDEV tutorial series at www.brokenthorn.com
*****************************************************************************/

static int32_t flpydsk_calibrate(floppy_t* drive);

floppy_t* floppyDrive[2];

const int32_t FLPY_SECTORS_PER_TRACK       =    18; // sectors per track
static floppy_t* CurrentDrive              =     0; // current working drive
static volatile bool ReceivedFloppyDiskIRQ = false; // set when IRQ fires
const int32_t MOTOR_SPIN_UP_TURN_OFF_TIME  =   350; // waiting time in milliseconds (motor spin up)
const int32_t WAITING_TIME                 =    10; // waiting time in milliseconds (for dynamic processes)

// IO ports
enum FLPYDSK_IO
{
    FLPYDSK_DOR  = 0x3f2,
    FLPYDSK_MSR  = 0x3f4,
    FLPYDSK_FIFO = 0x3f5,
    FLPYDSK_CTRL = 0x3f7
};

// Bits 0-4 of command byte
enum FLPYDSK_CMD
{
    FDC_CMD_READ_TRACK   =   2,
    FDC_CMD_SPECIFY      =   3,
    FDC_CMD_CHECK_STAT   =   4,
    FDC_CMD_WRITE_SECT   =   5,
    FDC_CMD_READ_SECT    =   6,
    FDC_CMD_CALIBRATE    =   7,
    FDC_CMD_CHECK_INT    =   8,
    FDC_CMD_FORMAT_TRACK = 0xD,
    FDC_CMD_SEEK         = 0xF
};

// Additional command masks
enum FLPYDSK_CMD_EXT
{
    FDC_CMD_EXT_SKIP       = 0x20,
    FDC_CMD_EXT_DENSITY    = 0x40,
    FDC_CMD_EXT_MULTITRACK = 0x80
};

// Digital Output Register (DOR)
enum FLPYDSK_DOR_MASK
{
    FLPYDSK_DOR_MASK_DRIVE0       =   0,
    FLPYDSK_DOR_MASK_DRIVE1       =   1,
    FLPYDSK_DOR_MASK_DRIVE2       =   2,
    FLPYDSK_DOR_MASK_DRIVE3       =   3,
    FLPYDSK_DOR_MASK_RESET        =   4,
    FLPYDSK_DOR_MASK_DMA          =   8,
    FLPYDSK_DOR_MASK_DRIVE0_MOTOR =  16,
    FLPYDSK_DOR_MASK_DRIVE1_MOTOR =  32,
    FLPYDSK_DOR_MASK_DRIVE2_MOTOR =  64,
    FLPYDSK_DOR_MASK_DRIVE3_MOTOR = 128
};

// Main Status Register
enum FLPYDSK_MSR_MASK
{
    FLPYDSK_MSR_MASK_DRIVE1_POS_MODE =   1,
    FLPYDSK_MSR_MASK_DRIVE2_POS_MODE =   2,
    FLPYDSK_MSR_MASK_DRIVE3_POS_MODE =   4,
    FLPYDSK_MSR_MASK_DRIVE4_POS_MODE =   8,
    FLPYDSK_MSR_MASK_BUSY            =  16,
    FLPYDSK_MSR_MASK_DMA             =  32,
    FLPYDSK_MSR_MASK_DATAIO          =  64,
    FLPYDSK_MSR_MASK_DATAREG         = 128
};

// Controller Status Port 0
enum FLPYDSK_ST0_MASK
{
    FLPYDSK_ST0_MASK_DRIVE0     =   0,
    FLPYDSK_ST0_MASK_DRIVE1     =   1,
    FLPYDSK_ST0_MASK_DRIVE2     =   2,
    FLPYDSK_ST0_MASK_DRIVE3     =   3,
    FLPYDSK_ST0_MASK_HEADACTIVE =   4,
    FLPYDSK_ST0_MASK_NOTREADY   =   8,
    FLPYDSK_ST0_MASK_UNITCHECK  =  16,
    FLPYDSK_ST0_MASK_SEEKEND    =  32,
    FLPYDSK_ST0_MASK_INTCODE    =  64
};

// LPYDSK_ST0_MASK_INTCODE types
enum FLPYDSK_ST0_INTCODE_TYP
{
    FLPYDSK_ST0_TYP_NORMAL       = 0,
    FLPYDSK_ST0_TYP_ABNORMAL_ERR = 1,
    FLPYDSK_ST0_TYP_INVALID_ERR  = 2,
    FLPYDSK_ST0_TYP_NOTREADY     = 3
};

// GAP 3 sizes
enum FLPYDSK_GAP3_LENGTH
{
    FLPYDSK_GAP3_LENGTH_STD  = 42,
    FLPYDSK_GAP3_LENGTH_5_14 = 32,
    FLPYDSK_GAP3_LENGTH_3_5  = 27
};

// Formula: 2^sector_number * 128
enum FLPYDSK_SECTOR_DTL
{

    FLPYDSK_SECTOR_DTL_128  = 0,
    FLPYDSK_SECTOR_DTL_256  = 1,
    FLPYDSK_SECTOR_DTL_512  = 2,
    FLPYDSK_SECTOR_DTL_1024 = 4
};

static floppy_t* createFloppy(uint8_t ID)
{
    floppy_t* fdd = malloc(sizeof(floppy_t), 0);
    fdd->ID       = ID;
    fdd->motor    = false; // floppy motor is off
    fdd->RW_Lock  = false; // floppy is not blocked
	fdd->accessRemaining = 0;

    fdd->drive.insertedDisk = malloc(sizeof(disk_t), 0);
    fdd->drive.insertedDisk->type = &FLOPPYDISK;
    fdd->drive.insertedDisk->partition[0] = malloc(sizeof(partition_t), 0);
    fdd->drive.insertedDisk->partition[1] = 0;
    fdd->drive.insertedDisk->partition[2] = 0;
    fdd->drive.insertedDisk->partition[3] = 0;
    fdd->drive.insertedDisk->partition[0]->buffer = malloc(512, 0);
    fdd->drive.insertedDisk->partition[0]->disk = fdd->drive.insertedDisk;

    fdd->drive.type = FDD;
    fdd->drive.data = (void*)(ID+1);

    attachDisk(fdd->drive.insertedDisk); // disk == floppy disk
    attachPort(&fdd->drive);

    CurrentDrive = fdd;
	while(flpydsk_read_sector(0, true));
    analyzeBootSector((void*)DMA_BUFFER, fdd->drive.insertedDisk->partition[0]);

    return(fdd);
}

void floppy_install()
{
    if ((cmos_read(0x10)>>4) == 4) // 1st floppy 1,44 MB: 0100....b
    {
        printf("1.44 MB FDD first device found\n");
        floppyDrive[0] = createFloppy(0);
        strncpy(floppyDrive[0]->drive.name, "Floppy Dev 1", 12);
        floppyDrive[0]->drive.name[12]=0; // terminate string

        if ((cmos_read(0x10) & 0xF) == 4) // 2nd floppy 1,44 MB: 0100....b
        {
            printf("1.44 MB FDD second device found\n");
            floppyDrive[1] = createFloppy(1);
            strncpy(floppyDrive[1]->drive.name, "Floppy Dev 2", 12);
            floppyDrive[1]->drive.name[12]=0; // terminate string
        }
        else
        {
            floppyDrive[1] = NULL;
        }

        flpydsk_install(32+6);                  // floppy disk uses IRQ 6 // 32+6
        memset((void*)DMA_BUFFER, 0x0, 0x2400); // set DMA memory buffer to zero

        CurrentDrive = floppyDrive[0];
    }
    else
    {
        printf("\n1.44 MB 1st floppy not shown by CMOS\n");
        floppyDrive[0] = 0;
    }
    printf("\n");
}

/// DMA Routines
/* The DMA (Direct Memory Access) controller allows the FDC to send data to the DMA, which can put the data in memory.
   While the FDC can be programmed to not use DMA, it is not very well supported on emulators or virtual machines.
   Because of this, the DMA is used for data transfers. */

// initialize DMA to use physical address 84k-128k
static void flpydsk_initialize_dma()
{
    outportb(0x0a, 0x06);   // mask dma channel 2
    outportb(0xd8, 0xFF);   // reset master flip-flop
    outportb(0x04, 0x00);   // DMA buffer address 0x1000
    outportb(0x04, 0x10);
    outportb(0xd8, 0xFF);   // reset master flip-flop
    outportb(0x05, 0xFF);   // count to 0x23FF (number of bytes in a 3.5" floppy disk track: 18*512)
    outportb(0x05, 0x23);

    outportb(0x81, 0x00);   // external page register = 0
    outportb(0x0a, 0x02);   // unmask dma channel 2
}

/// autoinit (2^4 = 16 = 0x10) creates problems with MS Virtual PC and on real hardware!
/// hence, it is not used here, but reinitialization is used before read/write
// prepare the DMA for read transfer
static void flpydsk_dma_read()
{
    outportb(0x0a, 0x06); // mask dma channel 2
    outportb(0x0b, 0x46); // single transfer, address increment, read, channel 2 // without autoinit
    outportb(0x0a, 0x02); // unmask dma channel 2
}

// prepare the DMA for write transfer
static void flpydsk_dma_write()
{
    outportb(0x0a, 0x06); // mask dma channel 2
    outportb(0x0b, 0x4A); // single transfer, address increment, write, channel 2 // without autoinit
    outportb(0x0a, 0x02); // unmask dma channel 2
}


/// Basic Controller In/Out Routines

// return fdc status
static uint8_t flpydsk_read_status ()
{
    return inportb(FLPYDSK_MSR); // just return main status register
}

// write to the fdc dor
static void flpydsk_write_dor(uint8_t val)
{
    outportb(FLPYDSK_DOR, val); // write the digital output register
}

// send command byte to fdc
static void flpydsk_send_command (uint8_t cmd)
{
    // wait until data register is ready. We send commands to the data register
    for (uint16_t i = 0; i < 500; ++i)
    {
        if (flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG)
        {
            outportb(FLPYDSK_FIFO, cmd);
            return;
        }
    }
}

// get data from fdc
static uint8_t flpydsk_read_data()
{
    // same as above function but returns data register for reading
    for (uint16_t i = 0; i < 500; ++i)
        if (flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG)
            return inportb(FLPYDSK_FIFO);
    return 0;
}

// write to the configuation control register
static void flpydsk_write_ccr(uint8_t val)
{
    outportb(FLPYDSK_CTRL, val); // write the configuation control
}


/// Interrupt Handling Routines

// wait for irq
static void flpydsk_wait_irq()
{
    uint32_t timeout = getCurrentSeconds()+2;
    while (ReceivedFloppyDiskIRQ == false) // wait for irq to fire
    {
        if ((timeout-getCurrentSeconds()) <= 0)
        {
            // printf("\ntimeout: IRQ not received!\n");
            break;
        }
    }
    ReceivedFloppyDiskIRQ = false;
}

//    floppy disk irq handler
void i86_flpy_irq(registers_t* r)
{
    ReceivedFloppyDiskIRQ = true; // irq fired. Set flag!
}


/// Controller Command Routines

// check interrupt status command
static void flpydsk_check_int(uint32_t* st0, uint32_t* cyl)
{
    flpydsk_send_command(FDC_CMD_CHECK_INT);
    *st0 = flpydsk_read_data();
    *cyl = flpydsk_read_data();
}

// turns the current floppy drives motor on/off
void flpydsk_control_motor(bool b)
{
    if(b == CurrentDrive->motor) return; // everything is already fine

    // turn on or off the motor of the current drive
    sti(); // important!
    if (b)
    {
        uint32_t motor = 0;
        switch (CurrentDrive->ID) // select the correct mask based on current drive
        {
            case 0: motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR; break;
            case 1: motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR; break;
            case 2: motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR; break;
            case 3: motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR; break;
        }
        flpydsk_write_dor(CurrentDrive->ID | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA); // motor on
        CurrentDrive->motor = true;
        sleepMilliSeconds(MOTOR_SPIN_UP_TURN_OFF_TIME); // wait for the motor to spin up/turn off
    }
    else if(CurrentDrive->drive.insertedDisk->accessRemaining == 0 && CurrentDrive->accessRemaining == 0)
    {
        flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET); // motor off
        CurrentDrive->motor = false;
        sleepMilliSeconds(WAITING_TIME);
    }
}

// configure drive
static void flpydsk_drive_data(uint32_t stepr, uint32_t loadt, uint32_t unloadt, int32_t dma)
{
    flpydsk_send_command(FDC_CMD_SPECIFY);
    flpydsk_send_command(((stepr & 0xF) << 4) | (unloadt & 0xF));
    flpydsk_send_command((loadt << 1)         | (dma==false) ? 0 : 1);
}



// disable controller
static void flpydsk_disable_controller()
{
    flpydsk_write_dor(0);
}

// enable controller
static void flpydsk_enable_controller()
{
    flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}

// reset controller
static void flpydsk_reset()
{
    flpydsk_disable_controller();
    flpydsk_enable_controller();
    flpydsk_wait_irq();

    uint32_t st0, cyl;
    // send CHECK_INT/SENSE INTERRUPT command to all drives
    for (uint8_t i=0; i<4; ++i)
    {
        flpydsk_check_int(&st0,&cyl);
    }
    flpydsk_write_ccr(0);              // transfer speed 500kb/s
    flpydsk_drive_data(3,16,0xF,true); // pass mechanical drive info: steprate=3ms, load time=16ms, unload time=240ms (0xF bei 500K)

	CurrentDrive->accessRemaining++;
    flpydsk_calibrate(CurrentDrive);   // calibrate the disk
}

/*
http://en.wikipedia.org/wiki/CHS_conversion#From_LBA_to_CHS
CYL = LBA / (HPC * SPT)
TEMP = LBA % (HPC * SPT)
HEAD = TEMP / SPT
SECT = TEMP % SPT + 1
*/

// convert LBA to CHS
static void flpydsk_lba_to_chs(int32_t lba, int32_t* head, int32_t* track, int32_t* sector)
{
   *track  =  lba / (FLPY_SECTORS_PER_TRACK * 2);
   *head   = (lba % (FLPY_SECTORS_PER_TRACK * 2)) / FLPY_SECTORS_PER_TRACK;
   *sector = (lba % (FLPY_SECTORS_PER_TRACK * 2)) % FLPY_SECTORS_PER_TRACK + 1;
}

// install floppy driver
void flpydsk_install(int32_t irq)
{
    irq_install_handler(irq, i86_flpy_irq);
    flpydsk_initialize_dma();
    flpydsk_reset();
    flpydsk_drive_data(13, 1, 0xF, true);
}

// calibrate the drive
static int32_t flpydsk_calibrate(floppy_t* drive)
{
    if (drive == 0)
    {
		CurrentDrive->accessRemaining--;
        return -2;
    }

    flpydsk_control_motor(true);

    uint32_t st0, cyl;
    for (uint8_t i = 0; i < 10; ++i)
    {
        // send command
        flpydsk_send_command(FDC_CMD_CALIBRATE);
        flpydsk_send_command(drive->ID);
        flpydsk_wait_irq();
        flpydsk_check_int(&st0, &cyl);

        if (!cyl) // did we find cylinder 0? if yes, calibration is correct
        {
			CurrentDrive->accessRemaining--;
            return(0);
        }
    }
	
	CurrentDrive->accessRemaining--;
    return(-1);
}

// seek to given track/cylinder
static int32_t flpydsk_seek(uint32_t cyl, uint32_t head)
{
    if (CurrentDrive == 0)
    {
		CurrentDrive->accessRemaining--;
        return -2;
    }

	CurrentDrive->accessRemaining++;
    flpydsk_calibrate(CurrentDrive);  // calibrate the disk ==> cyl. 0

    flpydsk_control_motor(true);

    uint32_t st0, cyl0;
    for (uint8_t i=0; i<10; ++i)
    {
        // send the command
        flpydsk_send_command (FDC_CMD_SEEK);
        flpydsk_send_command ((head) << 2 | CurrentDrive->ID);
        flpydsk_send_command (cyl);
        // printf("i=%d ", i);
        flpydsk_wait_irq();
        flpydsk_check_int(&st0,&cyl0);
        if (cyl0 == cyl) // found the cylinder?
        {
			CurrentDrive->accessRemaining--;
            return(0);
        }
    }
	
	CurrentDrive->accessRemaining--;
    return(-1);
}


// read or write a sector // http://www.isdaman.com/alsos/hardware/fdc/floppy_files/wrsec.gif
// read: operation = 0; write: operation = 1
static int32_t flpydsk_transfer_sector(uint8_t head, uint8_t track, uint8_t sector, uint8_t operation)
{
    uint32_t st0, cyl;

    flpydsk_control_motor(true);

    while (CurrentDrive->RW_Lock == true)
    {
        printf("waiting for Floppy Disk ");
        if (operation == 0)
        {
            printf("read ");
        }
        else if (operation == 1)
        {
            printf("write ");
        }

        for(uint8_t t = 0; t < 60; t++)
        {
             delay(1000000);
             printf(".");
        }
        printf("\n");
        break;
    }

    CurrentDrive->RW_Lock = true; // busy

    flpydsk_initialize_dma();

    if (operation == 0) // read a sector
    {
        flpydsk_dma_read();
        flpydsk_send_command(FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
    }
    if (operation == 1) // write a sector
    {
        flpydsk_dma_write();
        flpydsk_send_command(FDC_CMD_WRITE_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_DENSITY);
    }

    flpydsk_send_command(head << 2 | CurrentDrive->ID);
    flpydsk_send_command(track);
    flpydsk_send_command(head);
    flpydsk_send_command(sector);
    flpydsk_send_command(FLPYDSK_SECTOR_DTL_512);
    flpydsk_send_command(FLPY_SECTORS_PER_TRACK);
    flpydsk_send_command(FLPYDSK_GAP3_LENGTH_3_5);
    flpydsk_send_command(0xFF);
    flpydsk_wait_irq();
    // printf("ST0 ST1 ST2 C H S Size(2: 512 Byte):\n");
    int32_t retVal;
    for (uint8_t j=0; j<7; ++j)
    {
        int32_t val = flpydsk_read_data(); // read status info: ST0 ST1 ST2 C H S Size(2: 512 Byte)
        // printf("%d  ",val);
        if ((j==6) && (val==2)) // value 2 means 512 Byte
        {
            retVal = 0;
        }
        else
        {
            retVal = -1;
        }
    }
    // printf("\n\n");
    flpydsk_check_int(&st0,&cyl); // inform FDC that we handled interrupt

	CurrentDrive->accessRemaining--;

    CurrentDrive->RW_Lock = false; // ready

    return retVal;
}

int32_t flpydsk_readSector(uint32_t sector, uint8_t* buffer)
{
    int32_t retVal = flpydsk_read_sector(sector, false);
    memcpy(buffer, (void*)DMA_BUFFER, 512);
    return(retVal);
}

// read a sector
int32_t flpydsk_read_sector(uint32_t sectorLBA, bool single)
{
    if (CurrentDrive == 0)
    {
        return -3; // floppies 0-3 possible
    }

    if(single) CurrentDrive->drive.insertedDisk->accessRemaining++;

    int32_t head=0, track=0, sector=1;
    flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

    int32_t retVal=0;
	CurrentDrive->accessRemaining+=2;
    if (flpydsk_seek (track, head) !=0)
    {
        retVal=-2;
    }

    uint32_t timeout = 2; // limit
    while (flpydsk_transfer_sector(head, track, sector, 0) == -1)
    {
        timeout--;
        // printf("error read_sector. left: %d\n",timeout);
        if (timeout<= 0)
        {
            printf("\nread_sector timeout: read error!\n");
            retVal=-1;
            break;
        }
		CurrentDrive->accessRemaining++;
    }
    CurrentDrive->drive.insertedDisk->accessRemaining--;

    return retVal;
}

// write a sector
int32_t flpydsk_write_sector(int32_t sectorLBA, bool single)
{
    if (CurrentDrive == 0)
	{
		return -1;
	}

    // convert LBA sector to CHS
    int32_t head=0, track=0, sector=1;
    flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

    // turn motor on and seek to track
    if(single) CurrentDrive->drive.insertedDisk->accessRemaining++;

	CurrentDrive->accessRemaining+=2;
    if (flpydsk_seek (track, head)!=0)
    {
		CurrentDrive->accessRemaining--;
        printf("flpydsk_seek not ok. sector not written.\n");
        CurrentDrive->drive.insertedDisk->accessRemaining--;
        return -2;
    }
    else
    {
        flpydsk_transfer_sector(head, track, sector, 1);
        CurrentDrive->drive.insertedDisk->accessRemaining--;
        return 0;
    }
}


///******************* block write and read *****************************///

int32_t flpydsk_write_ia(int32_t i, void* a, int8_t option)
{
    int32_t val=0;

    if (option == SECTOR)
    {
        memcpy((void*)DMA_BUFFER, a, 0x200);
        val = i;
    }
    else if (option == TRACK)
    {
        memcpy((void*)DMA_BUFFER, a, 0x2400);
        val = i*18;
    }

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;

    while (flpydsk_write_sector(val, true) != 0) // without motor on/off
    {
        retVal = -1;
        timeout--;
        printf("write error: attempts left: %d\n",timeout);
        if (timeout<=0)
        {
            printf("timeout\n");
            break;
        }
    }
    if (retVal==0)
    {
        // printf("success write_sector.\n");
    }
    return retVal;
}


int32_t flpydsk_read_ia(int32_t i, void* a, int8_t option)
{
    /// TEST: change DMA before write/read
    /// printf("DMA manipulation\n");

    int32_t val=0;

    if (option == SECTOR)
    {
        memset((void*)DMA_BUFFER, 0x41, 0x200); // 0x41 is in ASCII the 'A'
        val = i;
    }
    else if (option == TRACK)
    {
        memset((void*)DMA_BUFFER, 0x41, 0x2400); // 0x41 is in ASCII the 'A'
        val = i*18;
    }

    //flpydsk_initialize_dma(); // important, if you do not use the unreliable autoinit bit of DMA

    int32_t retVal;
    for (uint8_t n = 0; n < MAX_ATTEMPTS_FLOPPY_DMA_BUFFER; n++)
    {
        retVal = flpydsk_read_sector(val, true);
        if (retVal!=0)
        {
            printf("\nread error: %d\n", retVal);
        }
        if ((*(uint8_t*)(DMA_BUFFER+ 0)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 1)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 2)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 3)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 4)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 5)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 6)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 7)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 8)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 9)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+10)==0x41) && (*(uint8_t*)(DMA_BUFFER+11)==0x41))
          {
              memset((void*)DMA_BUFFER, 0x41, 0x2400); // 0x41 is in ASCII the 'A'
              textColor(0x04);
              printf("Floppy ---> DMA attempt no. %d failed.\n",n+1);
              if (n>=MAX_ATTEMPTS_FLOPPY_DMA_BUFFER-1)
              {
                  printf("Floppy ---> DMA error.\n");
              }
              textColor(0x02);
              continue;
          }
          else
          {
              break;
          }
    }

    if (option == SECTOR)
    {
        memcpy(a, (void*)DMA_BUFFER, 0x200);
    }
    else if (option == TRACK)
    {
        memcpy(a, (void*)DMA_BUFFER, 0x2400);
    }
    return retVal;
}

void flpydsk_refreshVolumeNames()
{
    floppy_t* currentDrive = CurrentDrive;

    for(uint8_t i = 0; i < 2; i++)
    {
        if(floppyDrive[i] == 0) continue;

        CurrentDrive = floppyDrive[i];

        memset((void*)DMA_BUFFER, 0x0, 0x2400); // 18 sectors: 18 * 512 = 9216 = 0x2400

        flpydsk_initialize_dma(); // important, if you do not use the unreliable autoinit bit of DMA. TODO: Do we need it here?

        /// TODO: change to read_ia(...)!

        while(flpydsk_read_sector(19, true) != 0); // start at 0x2600: root directory (14 sectors) 

        strncpy(CurrentDrive->drive.insertedDisk->name, (char*)DMA_BUFFER, 11);
        CurrentDrive->drive.insertedDisk->name[11] = 0; // end of string
    }
    CurrentDrive = currentDrive;
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
