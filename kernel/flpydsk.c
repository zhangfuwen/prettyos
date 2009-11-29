#include "flpydsk.h"
// detailed infos about FDC and FAT12:
// http://www.isdaman.com/alsos/hardware/fdc/floppy.htm
// http://lowlevel.brainsware.org/wiki/index.php/FDC#Data_Address_Mark
// http://www.brokenthorn.com/Resources/OSDev20.html
// http://www.eit.lth.se/fileadmin/eit/courses/eit015/FAT12Description.pdf

/*****************************************************************************
  This module is based on the open source code
  of the OSDEV tutorial series at www.brokenthorn.com
*****************************************************************************/

const int32_t FLPY_SECTORS_PER_TRACK           =   18;   // sectors per track
static uint8_t	_CurrentDrive                  =  false; // current working drive, default: 0
static volatile uint8_t ReceivedFloppyDiskIRQ  =  false; // set when IRQ fires
const int32_t MOTOR_SPIN_UP_TURN_OFF_TIME      =  500;   // waiting time in milliseconds


// IO ports
enum FLPYDSK_IO
{
    FLPYDSK_DOR	        =   0x3f2,
    FLPYDSK_MSR	        =   0x3f4,
    FLPYDSK_FIFO        =   0x3f5,
    FLPYDSK_CTRL        =   0x3f7
};

// Bits 0-4 of command byte
enum FLPYDSK_CMD
{
    FDC_CMD_READ_TRACK	 =    2,
    FDC_CMD_SPECIFY		 =    3,
    FDC_CMD_CHECK_STAT	 =	  4,
    FDC_CMD_WRITE_SECT	 =	  5,
    FDC_CMD_READ_SECT	 =	  6,
    FDC_CMD_CALIBRATE	 =	  7,
    FDC_CMD_CHECK_INT	 =	  8,
    FDC_CMD_FORMAT_TRACK =	0xd,
    FDC_CMD_SEEK		 =	0xf
};

// Additional command masks
enum FLPYDSK_CMD_EXT
{
    FDC_CMD_EXT_SKIP       = 0x20,
    FDC_CMD_EXT_DENSITY	   = 0x40,
    FDC_CMD_EXT_MULTITRACK = 0x80
};

// Digital Output Register (DOR)
enum FLPYDSK_DOR_MASK
{
    FLPYDSK_DOR_MASK_DRIVE0       =   0,
    FLPYDSK_DOR_MASK_DRIVE1       =   1,
    FLPYDSK_DOR_MASK_DRIVE2	      =   2,
    FLPYDSK_DOR_MASK_DRIVE3	      =   3,
    FLPYDSK_DOR_MASK_RESET	      =   4,
    FLPYDSK_DOR_MASK_DMA          =   8,
    FLPYDSK_DOR_MASK_DRIVE0_MOTOR =  16,
    FLPYDSK_DOR_MASK_DRIVE1_MOTOR =  32,
    FLPYDSK_DOR_MASK_DRIVE2_MOTOR =	 64,
    FLPYDSK_DOR_MASK_DRIVE3_MOTOR =	128
};

// Main Status Register
enum FLPYDSK_MSR_MASK
{
	FLPYDSK_MSR_MASK_DRIVE1_POS_MODE	=	  1,
	FLPYDSK_MSR_MASK_DRIVE2_POS_MODE	=	  2,
	FLPYDSK_MSR_MASK_DRIVE3_POS_MODE	=	  4,
	FLPYDSK_MSR_MASK_DRIVE4_POS_MODE	=	  8,
	FLPYDSK_MSR_MASK_BUSY				=	 16,
	FLPYDSK_MSR_MASK_DMA				=	 32,
	FLPYDSK_MSR_MASK_DATAIO				=	 64,
	FLPYDSK_MSR_MASK_DATAREG			=	128
};

// Controller Status Port 0
enum FLPYDSK_ST0_MASK
{
	FLPYDSK_ST0_MASK_DRIVE0		=   0,
	FLPYDSK_ST0_MASK_DRIVE1		=   1,
	FLPYDSK_ST0_MASK_DRIVE2		=   2,
	FLPYDSK_ST0_MASK_DRIVE3		=   3,
	FLPYDSK_ST0_MASK_HEADACTIVE	=   4,
	FLPYDSK_ST0_MASK_NOTREADY	=   8,
	FLPYDSK_ST0_MASK_UNITCHECK	=  16,
	FLPYDSK_ST0_MASK_SEEKEND	=  32,
	FLPYDSK_ST0_MASK_INTCODE	=  64
};

// LPYDSK_ST0_MASK_INTCODE types
enum FLPYDSK_ST0_INTCODE_TYP
{
	FLPYDSK_ST0_TYP_NORMAL		 =	0,
	FLPYDSK_ST0_TYP_ABNORMAL_ERR =	1,
	FLPYDSK_ST0_TYP_INVALID_ERR	 =	2,
	FLPYDSK_ST0_TYP_NOTREADY	 =	3
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

	FLPYDSK_SECTOR_DTL_128	=	0,
	FLPYDSK_SECTOR_DTL_256	=	1,
	FLPYDSK_SECTOR_DTL_512	=	2,
	FLPYDSK_SECTOR_DTL_1024	=	4
};

/**
**	DMA Routines.
**	The DMA (Direct Memory Access) controller allows the FDC to send data to the DMA, which can put the data in memory.
**  While the FDC can be programmed to not use DMA, it is not very well supported on emulators or virtual machines.
**  Because of this, the DMA is used for data transfers.
**/

// initialize DMA to use physical address 84k-128k
void flpydsk_initialize_dma()
{
	outportb(0x0a, 0x06);	// mask dma channel 2
	outportb(0xd8, 0xFF);	// reset master flip-flop
	outportb(0x04, 0x00);   // DMA buffer address 0x1000
	outportb(0x04, 0x10);
	outportb(0xd8, 0xFF);   // reset master flip-flop
	outportb(0x05, 0xFF);   // count to 0x23FF (number of bytes in a 3.5" floppy disk track: 18*512)
	outportb(0x05, 0x23);

	outportb(0x81, 0x00);   // external page register = 0
	outportb(0x0a, 0x02);   // unmask dma channel 2
}

// prepare the DMA for read transfer
void flpydsk_dma_read()
{
	outportb(0x0a, 0x06); // mask dma channel 2
	outportb(0x0b, 0x56); // single transfer, address increment, autoinit, read, channel 2
	outportb(0x0a, 0x02); // unmask dma channel 2
}

// prepare the DMA for write transfer
void flpydsk_dma_write()
{
	outportb(0x0a, 0x06); // mask dma channel 2
	outportb(0x0b, 0x5A); // single transfer, address increment, autoinit, write, channel 2
	outportb(0x0a, 0x02); // unmask dma channel 2
}

/**
*	Basic Controller In/Out Routines
*/

// return fdc status
uint8_t flpydsk_read_status ()
{
	return inportb(FLPYDSK_MSR); // just return main status register
}

// write to the fdc dor
void flpydsk_write_dor( uint8_t val )
{
	outportb(FLPYDSK_DOR, val); // write the digital output register
}

// send command byte to fdc
void flpydsk_send_command (uint8_t cmd)
{
    // wait until data register is ready. We send commands to the data register
	int32_t i;
	for(i=0; i<500; ++i)
		if( flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG )
			return outportb(FLPYDSK_FIFO, cmd);
}

// get data from fdc
uint8_t flpydsk_read_data()
{
	// same as above function but returns data register for reading
	int32_t i;
	for(i=0; i<500; ++i)
		if( flpydsk_read_status() & FLPYDSK_MSR_MASK_DATAREG )
			return inportb(FLPYDSK_FIFO);
	return 0;
}

// write to the configuation control register
void flpydsk_write_ccr(uint8_t val)
{
	outportb(FLPYDSK_CTRL, val); // write the configuation control
}

/**
*	Interrupt Handling Routines
*/

// wait for irq
void flpydsk_wait_irq()
{
    uint32_t timeout = getCurrentSeconds()+2;
    while( ReceivedFloppyDiskIRQ == false) // wait for irq to fire
    {
	    if( (timeout-getCurrentSeconds()) <= 0 )
	    {
	        // printformat("\ntimeout: IRQ not received!\n");
	        break;
	    }
    }
    ReceivedFloppyDiskIRQ = false;
}

//	floppy disk irq handler
void i86_flpy_irq(struct regs* r)
{
	ReceivedFloppyDiskIRQ = true; // irq fired. Set flag!
}

/**
*	Controller Command Routines
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
	switch(_CurrentDrive) // select the correct mask based on current drive
	{
		case 0: motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR; break;
		case 1:	motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR; break;
		case 2: motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR; break;
		case 3:	motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR; break;
	}

    // turn on or off the motor of that drive
	if(b)
	{
        flpydsk_write_dor(_CurrentDrive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
	}
	else
	{
        flpydsk_write_dor(FLPYDSK_DOR_MASK_RESET);
	}
	sti(); // important!
	sleepMilliSeconds(MOTOR_SPIN_UP_TURN_OFF_TIME); // wait for the motor to spin up/turn off
}

// configure drive
void flpydsk_drive_data(uint32_t stepr, uint32_t loadt, uint32_t unloadt, int32_t dma )
{
	flpydsk_send_command(   FDC_CMD_SPECIFY                           );
	flpydsk_send_command( ((stepr & 0xF) << 4) | (unloadt & 0xF)      );
	flpydsk_send_command( ( loadt << 1)        | (dma==false) ? 0 : 1 );
}

// calibrate the drive
int32_t flpydsk_calibrate(uint32_t drive)
{
	uint32_t st0, cyl;
	if (drive >= 4)	return -2;
	flpydsk_control_motor(true); // turn on the motor
	int32_t i;
	for(i=0; i<10; ++i)
	{
		// send command
		flpydsk_send_command( FDC_CMD_CALIBRATE );
		flpydsk_send_command( drive );
		flpydsk_wait_irq();
		flpydsk_check_int(&st0, &cyl);

		if(!cyl) // did we find cylinder 0? if so, we are done
		{
			flpydsk_control_motor(false);
			return 0;
		}
	}
	flpydsk_control_motor(false);
	return -1;
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
	int32_t i;
	for(i=0; i<4; ++i)
	{
		flpydsk_check_int(&st0,&cyl);
	}
	flpydsk_write_ccr(0);              // transfer speed 500kb/s
	flpydsk_drive_data(3,16,0xF,true); // pass mechanical drive info: steprate=3ms, load time=16ms, unload time=240ms (0xF bei 500K)
	flpydsk_calibrate(_CurrentDrive);  // calibrate the disk
}

// seek to given track/cylinder
int32_t flpydsk_seek( uint32_t cyl, uint32_t head )
{
	uint32_t st0, cyl0;
	if (_CurrentDrive >= 4)	return -1;
	int32_t i;
	for(i=0; i<10; ++i)
	{
		// send the command
        flpydsk_send_command (FDC_CMD_SEEK);
        flpydsk_send_command ( (head) << 2 | _CurrentDrive);
        flpydsk_send_command (cyl);

        // printformat("i=%d ", i);

        flpydsk_wait_irq();
		flpydsk_check_int(&st0,&cyl0);

        if ( cyl0 == cyl) // found the cylinder?
        {
		    return 0;
        }
    }
	return -1;
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
   *track  =   lba / ( FLPY_SECTORS_PER_TRACK * 2 );
   *head   = ( lba % ( FLPY_SECTORS_PER_TRACK * 2 ) ) / ( FLPY_SECTORS_PER_TRACK );
   *sector = ( lba % ( FLPY_SECTORS_PER_TRACK * 2 ) ) % FLPY_SECTORS_PER_TRACK + 1;
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

// read or write a sector // http://www.isdaman.com/alsos/hardware/fdc/floppy_files/wrsec.gif
// read: operation = 0; write: operation = 1
int32_t flpydsk_transfer_sector(uint8_t head, uint8_t track, uint8_t sector, uint8_t operation)
{
    uint32_t st0, cyl;
    if(operation == 0) // read a sector
    {
        flpydsk_dma_read();
        flpydsk_send_command( FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
    }
    if(operation == 1) // write a sector
	{
        flpydsk_dma_write();
        flpydsk_send_command( FDC_CMD_WRITE_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_DENSITY );
    }

    /// Delay
    sleepMilliSeconds(50); // what is Floppy Disk head settle time?
    /// Delay

    flpydsk_send_command( head << 2 | _CurrentDrive );
    flpydsk_send_command( track);
    flpydsk_send_command( head);
    flpydsk_send_command( sector);
    flpydsk_send_command( FLPYDSK_SECTOR_DTL_512 );
    flpydsk_send_command( FLPY_SECTORS_PER_TRACK );
    flpydsk_send_command( FLPYDSK_GAP3_LENGTH_3_5 );
    flpydsk_send_command( 0xFF );
    flpydsk_wait_irq();
    // printformat("ST0 ST1 ST2 C H S Size(2: 512 Byte):\n");
    int32_t j,retVal;
    for(j=0; j<7; ++j)
    {
        int32_t val = flpydsk_read_data(); // read status info: ST0 ST1 ST2 C H S Size(2: 512 Byte)
        // printformat("%d  ",val);
        if((j==6) && (val==2))
        {
            retVal = 0;
        }
        else
        {
            retVal = -1;
        }
    }
    // printformat("\n\n");
    flpydsk_check_int(&st0,&cyl);    // inform FDC that we handled interrupt
    return retVal;
}

// read a sector
uint8_t* flpydsk_read_sector(int32_t sectorLBA)
{
    if(_CurrentDrive >= 4) return 0; // floppies 0-3 possible

	int32_t head=0, track=0, sector=1;
	flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);

	// turn motor on and seek to track
	flpydsk_control_motor(true);
	if(flpydsk_seek (track, head))
	{
	    return 0;
	}

	// read sector, turn motor off, return DMA buffer
	uint32_t timeout = getCurrentSeconds()+2;
	while( flpydsk_transfer_sector(head, track, sector, 0) == -1 )
    {
	    if( (timeout-getCurrentSeconds()) <= 0 )
	    {
	        printformat("\ntimeout: read/write error!\n");
	        break;
	    }
    }
	flpydsk_control_motor(false);
	return (uint8_t*)DMA_BUFFER;
}

// write a sector
int32_t flpydsk_write_sector(int32_t sectorLBA)
{
	if (_CurrentDrive >= 4) return -1;
	// convert LBA sector to CHS
	int32_t head=0, track=0, sector=1;
	flpydsk_lba_to_chs(sectorLBA, &head, &track, &sector);
	// turn motor on and seek to track
	flpydsk_control_motor(true);
	if(flpydsk_seek (track, head)) return -2;
	// write sector and turn motor off
	flpydsk_transfer_sector(head, track, sector, 1);
	flpydsk_control_motor(false);
	return 0;
}


/// only for tests in flpydsk.c, later: file data system

int32_t flpydsk_read_directory()
{
    int32_t error = -1; // return value

	/// TEST
	k_memset((void*)DMA_BUFFER, 0x0, 0x2400);
	/// TEST

	uint8_t* retVal = flpydsk_read_sector(19);   // start at 0x2600: root directory (14 sectors)
	if(retVal != (uint8_t*)0x1000)
	{
	    printformat("\nerror: buffer = flpydsk_read_sector(...): %X\n",retVal);
	}
	printformat("<Floppy Disc Root Dir>\n");

	uint32_t i;
	for(i=0;i<224;++i)       // 224 Entries * 32 Byte
	{
        if(
			(( *((uint8_t*)(DMA_BUFFER + i*32)) )      != 0x00 ) && /* free from here on           */
			(( *((uint8_t*)(DMA_BUFFER + i*32)) )      != 0xE5 ) && /* 0xE5 deleted = free         */
			(( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) != 0x0F )    /* 0x0F part of long file name */
		  )
		  {
		    error = 0;
			int32_t start = DMA_BUFFER + i*32; // name
			int32_t count = 8;
			int8_t* end = (int8_t*)(start+count);
			for(; count != 0; --count)
			{
			    if( *(end-count) != 0x20 ) /* empty space in file name */
				    printformat("%c",*(end-count));
			}

			if(i!=0) printformat("."); // usual separator between file name and file extension

			start = DMA_BUFFER + i*32 + 8; // extension
			count = 3;
			end = (int8_t*)(start+count);
			for(; count != 0; --count)
				printformat("%c",*(end-count));

			// filesize
			printformat("\t%d byte", *((uint32_t*)(DMA_BUFFER + i*32 + 28)));

            // attributes
			printformat("\t");
			if(*((uint32_t*)(DMA_BUFFER + i*32 + 28))<100) printformat("\t");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x08 ) == 0x08 ) printformat(" (vol)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x10 ) == 0x10 ) printformat(" (dir)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x01 ) == 0x01 ) printformat(" (r/o)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x02 ) == 0x02 ) printformat(" (hid)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x04 ) == 0x04 ) printformat(" (sys)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x20 ) == 0x20 ) printformat(" (arc)");

			// 1st cluster: physical sector number  =  33  +  FAT entry number  -  2  =  FAT entry number  +  31
            printformat("  first sector: %d", *((uint16_t*)(DMA_BUFFER + i*32 + 26))+31);
            printformat("\n"); // next root directory entry
		  }//if
	}//for
    printformat("\n");
    return error;
}
