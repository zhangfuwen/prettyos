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

// detailed infos about FDC and FAT12:
// http://www.isdaman.com/alsos/hardware/fdc/floppy.htm
// http://lowlevel.brainsware.org/wiki/index.php/FDC#Data_Address_Mark
// http://www.brokenthorn.com/Resources/OSDev20.html
// http://www.eit.lth.se/fileadmin/eit/courses/eit015/FAT12Description.pdf

/*****************************************************************************
  This module is based on the open source code
  of the OSDEV tutorial series at www.brokenthorn.com
*****************************************************************************/

const int32_t FLPY_SECTORS_PER_TRACK           =    18; // sectors per track
static uint8_t    _CurrentDrive                =     0; // current working drive, default: 0
static volatile uint8_t ReceivedFloppyDiskIRQ  = false; // set when IRQ fires
const int32_t MOTOR_SPIN_UP_TURN_OFF_TIME      =   350; // waiting time in milliseconds (motor spin up)
const int32_t WAITING_TIME                     =    10; // waiting time in milliseconds (for dynamic processes)

// IO ports
enum FLPYDSK_IO
{
    FLPYDSK_DOR          =  0x3f2,
    FLPYDSK_MSR          =  0x3f4,
    FLPYDSK_FIFO         =  0x3f5,
    FLPYDSK_CTRL         =  0x3f7
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
    FDC_CMD_FORMAT_TRACK = 0xd,
    FDC_CMD_SEEK         = 0xf
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
    FLPYDSK_DOR_MASK_DRIVE0        =   0,
    FLPYDSK_DOR_MASK_DRIVE1        =   1,
    FLPYDSK_DOR_MASK_DRIVE2        =   2,
    FLPYDSK_DOR_MASK_DRIVE3        =   3,
    FLPYDSK_DOR_MASK_RESET         =   4,
    FLPYDSK_DOR_MASK_DMA           =   8,
    FLPYDSK_DOR_MASK_DRIVE0_MOTOR  =  16,
    FLPYDSK_DOR_MASK_DRIVE1_MOTOR  =  32,
    FLPYDSK_DOR_MASK_DRIVE2_MOTOR  =  64,
    FLPYDSK_DOR_MASK_DRIVE3_MOTOR  = 128
};

// Main Status Register
enum FLPYDSK_MSR_MASK
{
    FLPYDSK_MSR_MASK_DRIVE1_POS_MODE  =   1,
    FLPYDSK_MSR_MASK_DRIVE2_POS_MODE  =   2,
    FLPYDSK_MSR_MASK_DRIVE3_POS_MODE  =   4,
    FLPYDSK_MSR_MASK_DRIVE4_POS_MODE  =   8,
    FLPYDSK_MSR_MASK_BUSY             =  16,
    FLPYDSK_MSR_MASK_DMA              =  32,
    FLPYDSK_MSR_MASK_DATAIO           =  64,
    FLPYDSK_MSR_MASK_DATAREG          = 128
};

// Controller Status Port 0
enum FLPYDSK_ST0_MASK
{
    FLPYDSK_ST0_MASK_DRIVE0      =   0,
    FLPYDSK_ST0_MASK_DRIVE1      =   1,
    FLPYDSK_ST0_MASK_DRIVE2      =   2,
    FLPYDSK_ST0_MASK_DRIVE3      =   3,
    FLPYDSK_ST0_MASK_HEADACTIVE  =   4,
    FLPYDSK_ST0_MASK_NOTREADY    =   8,
    FLPYDSK_ST0_MASK_UNITCHECK   =  16,
    FLPYDSK_ST0_MASK_SEEKEND     =  32,
    FLPYDSK_ST0_MASK_INTCODE     =  64
};

// LPYDSK_ST0_MASK_INTCODE types
enum FLPYDSK_ST0_INTCODE_TYP
{
    FLPYDSK_ST0_TYP_NORMAL        =  0,
    FLPYDSK_ST0_TYP_ABNORMAL_ERR  =  1,
    FLPYDSK_ST0_TYP_INVALID_ERR   =  2,
    FLPYDSK_ST0_TYP_NOTREADY      =  3
};

// GAP 3 sizes
enum FLPYDSK_GAP3_LENGTH
{
    FLPYDSK_GAP3_LENGTH_STD   =  42,
    FLPYDSK_GAP3_LENGTH_5_14  =  32,
    FLPYDSK_GAP3_LENGTH_3_5   =  27
};

// Formula: 2^sector_number * 128
enum FLPYDSK_SECTOR_DTL
{

    FLPYDSK_SECTOR_DTL_128   =  0,
    FLPYDSK_SECTOR_DTL_256   =  1,
    FLPYDSK_SECTOR_DTL_512   =  2,
    FLPYDSK_SECTOR_DTL_1024  =  4
};

void floppy_install() {
    if ((cmos_read(0x10)>>4) == 4)         // 1st floppy 1,44 MB: 0100....b
    {
        printf("1.44 MB FDD device 0\n\n");
        ODA.flpy_motor[0] = false;            // first floppy motor is off
        flpydsk_set_working_drive(0);           // set drive 0 as current drive
        flpydsk_install(32+6);                  // floppy disk uses IRQ 6 // 32+6
        memset((void*)DMA_BUFFER, 0x0, 0x2400); // set DMA memory buffer to zero
        if ((cmos_read(0x10) & 0xF) == 4) // 2nd floppy 1,44 MB: 0100....b
        {
            printf("1.44 MB FDD device 1\n\n");
            ODA.flpy_motor[1] = false;        // second floppy motor is off
        }
    }
    else
    {
        printf("\n1.44 MB 1st floppy not shown by CMOS\n\n");
    }
}

/**
**    DMA Routines.
**    The DMA (Direct Memory Access) controller allows the FDC to send data to the DMA, which can put the data in memory.
**  While the FDC can be programmed to not use DMA, it is not very well supported on emulators or virtual machines.
**  Because of this, the DMA is used for data transfers.
**/

// initialize DMA to use physical address 84k-128k
void flpydsk_initialize_dma()
{
    outportb(0x0a, 0x06);    // mask dma channel 2
    outportb(0xd8, 0xFF);    // reset master flip-flop
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
void flpydsk_dma_read()
{
    outportb(0x0a, 0x06); // mask dma channel 2
    outportb(0x0b, 0x46); // single transfer, address increment, read, channel 2 // without autoinit
    outportb(0x0a, 0x02); // unmask dma channel 2
}

// prepare the DMA for write transfer
void flpydsk_dma_write()
{
    outportb(0x0a, 0x06); // mask dma channel 2
    outportb(0x0b, 0x4A); // single transfer, address increment, write, channel 2 // without autoinit
    outportb(0x0a, 0x02); // unmask dma channel 2
}

/**
*    Basic Controller In/Out Routines
*/

// return fdc status
uint8_t flpydsk_read_status ()
{
    return inportb(FLPYDSK_MSR); // just return main status register
}

// write to the fdc dor
void flpydsk_write_dor(uint8_t val)
{
    outportb(FLPYDSK_DOR, val); // write the digital output register
}

// send command byte to fdc
void flpydsk_send_command (uint8_t cmd)
{
    // wait until data register is ready. We send commands to the data register
    for (int32_t i=0; i<500; ++i)
        if (flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG)
            return outportb(FLPYDSK_FIFO, cmd);
}

// get data from fdc
uint8_t flpydsk_read_data()
{
    // same as above function but returns data register for reading
    for (int32_t i=0; i<500; ++i)
        if (flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG)
            return inportb(FLPYDSK_FIFO);
    return 0;
}

// write to the configuation control register
void flpydsk_write_ccr(uint8_t val)
{
    outportb(FLPYDSK_CTRL, val); // write the configuation control
}

/**
*    Interrupt Handling Routines
*/

// wait for irq
void flpydsk_wait_irq()
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

/**
*    Controller Command Routines
*/

// check interrupt status command
void flpydsk_check_int(uint32_t* st0, uint32_t* cyl)
{
    flpydsk_send_command(FDC_CMD_CHECK_INT);
    *st0 = flpydsk_read_data();
    *cyl = flpydsk_read_data();
}

// turns the current floppy drives motor on/off
void flpydsk_control_motor(bool b)
{
    uint32_t motor = 0;
    switch (_CurrentDrive) // select the correct mask based on current drive
    {
        case 0: motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR; break;
        case 1:    motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR; break;
        case 2: motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR; break;
        case 3:    motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR; break;
    }

    // turn on or off the motor of the current drive
    if ((b==true) && (ODA.flpy_motor[_CurrentDrive]==false))
    {
        flpydsk_write_dor(_CurrentDrive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA); // motor on
        ODA.flpy_motor[_CurrentDrive]=true;
        /*
        settextcolor(14,0);
        printf("floppy motor on\n");
        settextcolor(2,0);
        */
    }
    else if ((b==false) && (ODA.flpy_motor[_CurrentDrive]==true))
    {
        flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET); // motor off
        ODA.flpy_motor[_CurrentDrive]=false;

        /*
        settextcolor(14,0);
        printf("floppy motor off\n");
        settextcolor(2,0);
        */
    }
    sti(); // important!
    if ((b==true) && (ODA.flpy_motor[_CurrentDrive]==false))///TEST
    {
        sleepMilliSeconds(MOTOR_SPIN_UP_TURN_OFF_TIME); // wait for the motor to spin up/turn off
    }
    else
    {
        sleepMilliSeconds(WAITING_TIME);
    }
}

// configure drive
void flpydsk_drive_data(uint32_t stepr, uint32_t loadt, uint32_t unloadt, int32_t dma)
{
    flpydsk_send_command(FDC_CMD_SPECIFY);
    flpydsk_send_command(((stepr & 0xF) << 4) | (unloadt & 0xF));
    flpydsk_send_command((loadt << 1)        | (dma==false) ? 0 : 1);
}



// disable controller
void flpydsk_disable_controller()
{
    flpydsk_write_dor(0);
}

// enable controller
void flpydsk_enable_controller()
{
    flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}

// reset controller
void flpydsk_reset()
{
    uint32_t st0, cyl;
    flpydsk_disable_controller();
    flpydsk_enable_controller();
    flpydsk_wait_irq();

    // send CHECK_INT/SENSE INTERRUPT command to all drives
    for (int32_t i=0; i<4; ++i)
    {
        flpydsk_check_int(&st0,&cyl);
    }
    flpydsk_write_ccr(0);              // transfer speed 500kb/s
    flpydsk_drive_data(3,16,0xF,true); // pass mechanical drive info: steprate=3ms, load time=16ms, unload time=240ms (0xF bei 500K)

    flpydsk_control_motor(true);       //printf("reset.motor_on\n");
    flpydsk_calibrate(_CurrentDrive);  // calibrate the disk
    flpydsk_control_motor(false);      // printf("reset.motor_off\n");
}

/*
http://en.wikipedia.org/wiki/CHS_conversion#From_LBA_to_CHS
CYL = LBA / (HPC * SPT)
TEMP = LBA % (HPC * SPT)
HEAD = TEMP / SPT
SECT = TEMP % SPT + 1
*/

// convert LBA to CHS
void flpydsk_lba_to_chs(int32_t lba, int32_t* head, int32_t* track, int32_t* sector)
{
   *track  =   lba / (FLPY_SECTORS_PER_TRACK * 2);
   *head   = (lba % (FLPY_SECTORS_PER_TRACK * 2)) / (FLPY_SECTORS_PER_TRACK);
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

// set current working drive
void flpydsk_set_working_drive(uint8_t drive){ if (drive < 4) _CurrentDrive = drive; }

// get current working drive
uint8_t flpydsk_get_working_drive(){ return _CurrentDrive; }

// calibrate the drive
int32_t flpydsk_calibrate(uint32_t drive)
{
    int32_t  retVal;
    uint32_t st0, cyl;
    if (drive >= 4)
    {
        return -2;
    }

    for (uint8_t i=0; i<10; ++i)
    {
        // send command
        flpydsk_send_command(FDC_CMD_CALIBRATE);
        flpydsk_send_command(drive);
        flpydsk_wait_irq();
        flpydsk_check_int(&st0, &cyl);

        if (!cyl) // did we find cylinder 0? if yes, calibration is correct
        {
            retVal = 0;
            break;
        }
        else
        {
            retVal = -1;
        }
    }

    if (retVal==0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

// seek to given track/cylinder
int32_t flpydsk_seek(uint32_t cyl, uint32_t head)
{
    int32_t  retVal;
    uint32_t st0, cyl0;
    if (_CurrentDrive >= 4)
    {
        return -2;
    }

    /// TEST
    flpydsk_calibrate(_CurrentDrive);  // calibrate the disk ==> cyl. 0
    /// TEST

    for (uint8_t i=0; i<10; ++i)
    {
        // send the command
        flpydsk_send_command (FDC_CMD_SEEK);
        flpydsk_send_command ((head) << 2 | _CurrentDrive);
        flpydsk_send_command (cyl);
        // printf("i=%d ", i);
        flpydsk_wait_irq();
        flpydsk_check_int(&st0,&cyl0);
        if (cyl0 == cyl) // found the cylinder?
        {
            retVal = 0;
            break;
        }
           else
        {
            retVal = -1;
        }
    }
    if (retVal==0)
    {
        // printf("cyl. found\t");
        // printf("cyl0: %d cyl: %d \n",cyl0,cyl);
        return 0;
    }
    else
    {
        // printf("cyl. not found\t");
        // printf("cyl0: %d cyl: %d \n",cyl0,cyl);
        return -1;
    }
}


// read or write a sector // http://www.isdaman.com/alsos/hardware/fdc/floppy_files/wrsec.gif
// read: operation = 0; write: operation = 1
int32_t flpydsk_transfer_sector(uint8_t head, uint8_t track, uint8_t sector, uint8_t operation)
{
    uint32_t st0, cyl;

    /// TEST
    while ( ODA.flpy_ReadWriteFlag[_CurrentDrive] == true )
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

        for(uint32_t t=0;t<60;t++)
        {
             delay(1000000);
             printf(".");
        }
        printf("\n");
        break;
    }

    ODA.flpy_ReadWriteFlag[_CurrentDrive] = true; // busy
    /// TEST

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

    flpydsk_send_command(head << 2 | _CurrentDrive);
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
    flpydsk_check_int(&st0,&cyl);    // inform FDC that we handled interrupt

    /// TEST
    ODA.flpy_ReadWriteFlag[_CurrentDrive] = false; // ready
    /// TEST

    return retVal;
}

// read a sector
int32_t flpydsk_read_sector(int32_t sectorLBA, int8_t motor)
{
    int32_t retVal=0;

    if (_CurrentDrive >= 4)
    {
        return -3; // floppies 0-3 possible
    }

    if (motor)
    {
        flpydsk_control_motor(true);
        // printf("read_sector.motor_on\n");
    }

    int32_t head=0, track=0, sector=1;
    flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

    if (flpydsk_seek (track, head) !=0)
    {
        printf("flpydsk_seek not ok.\n");
        retVal=-2;
    }

    uint32_t timeout = 2; // limit
    while (flpydsk_transfer_sector(head, track, sector, 0) == -1)
    {
        timeout--;
        printf("error read_sector. left: %d\n",timeout);
        if (timeout<= 0)
        {
            printf("\nread_sector timeout: read error!\n");
            retVal=-1;
            break;
        }
    }

    if (motor)
    {
        flpydsk_control_motor(false); // printf("read_sector.motor_off\n");
    }

    return retVal;
}

// write a sector
int32_t flpydsk_write_sector(int32_t sectorLBA)
{
    if (_CurrentDrive >= 4) return -1;

    // convert LBA sector to CHS
    int32_t head=0, track=0, sector=1;
    flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

    // turn motor on and seek to track
    flpydsk_control_motor(true);      // printf("write_sector.motor_on\n");
    if (flpydsk_seek (track, head)!=0)
    {
        printf("flpydsk_seek not ok. sector not written.\n");
        flpydsk_control_motor(false); // printf("write_sector.motor_off\n");
        return -2;
    }
    else
    {
        // printf("flpydsk_seek ok\n");

        flpydsk_transfer_sector(head, track, sector, 1);
        flpydsk_control_motor(false); // printf("write_sector.motor_off\n");
        return 0;
    }
}

// write a sector
int32_t flpydsk_write_sector_wo_motor(int32_t sectorLBA)
{
    if (_CurrentDrive >= 4) return -1;

    // convert LBA sector to CHS
    int32_t head=0, track=0, sector=1;
    flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

    if (flpydsk_seek (track, head)!=0)
    {
        printf("flpydsk_seek not ok. sector not written.\n");
        return -2;
    }
    else
    {
        // printf("flpydsk_seek ok\n");
        flpydsk_transfer_sector(head, track, sector, 1);
        return 0;
    }
}


///******************* block write and read *****************************///

int32_t flpydsk_write_ia(int32_t i, void* a, int8_t option) /// floppy
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

    while (flpydsk_write_sector_wo_motor(val) != 0) // without motor on/off
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


int32_t flpydsk_read_ia(int32_t i, void* a, int8_t option) /// floppy
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
    flpydsk_control_motor(true);

    int32_t retVal;
    for (uint8_t n=0;n<MAX_ATTEMPTS_FLOPPY_DMA_BUFFER;n++)
    {
        retVal = flpydsk_read_sector(val,0);
        if (retVal!=0)
        {
            printf("\nread error: %d\n",retVal);
        }
        if ((*(uint8_t*)(DMA_BUFFER+ 0)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 1)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 2)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 3)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 4)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 5)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 6)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 7)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+ 8)==0x41) && (*(uint8_t*)(DMA_BUFFER+ 9)==0x41) &&
            (*(uint8_t*)(DMA_BUFFER+10)==0x41) && (*(uint8_t*)(DMA_BUFFER+11)==0x41)
)
          {memset((void*)DMA_BUFFER, 0x41, 0x2400); // 0x41 is in ASCII the 'A'
              settextcolor(4,0);
              printf("Floppy ---> DMA attempt no. %d failed.\n",n+1);
              if (n>=MAX_ATTEMPTS_FLOPPY_DMA_BUFFER-1)
              {
                  printf("Floppy ---> DMA error.\n");
              }
              settextcolor(2,0);
              continue;
          }
          else
          {
              /*
              settextcolor(3,0);
              printf("Floppy ---> DMA success.\n");
              settextcolor(2,0);
              */
              break;
          }
    }

    if (option == SECTOR)
    {
        memcpy((void*)a, (void*)DMA_BUFFER, 0x200);
    }
    else if (option == TRACK)
    {
        memcpy((void*)a, (void*)DMA_BUFFER, 0x2400);
    }
    return retVal;
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
