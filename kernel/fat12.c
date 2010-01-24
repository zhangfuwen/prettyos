#include "flpydsk.h"
#include "fat12.h"

int32_t flpydsk_read_directory()
{
    int32_t error = -1; // return value

	/// TEST
	memset((void*)DMA_BUFFER, 0x0, 0x2400);
	/// TEST

	uint8_t* retVal = flpydsk_read_sector(19);   // start at 0x2600: root directory (14 sectors)
	if(retVal != (uint8_t*)DMA_BUFFER)
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

			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x08 ) != 0x08 ) // no volume label
			{
			     printformat("."); // usual separator between file name and file extension
			}

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

int32_t flpydsk_write_sector_ia( int32_t i, void* a)
{
    memset((void*)DMA_BUFFER, 0x0, 0x200);
    memcpy((void*)DMA_BUFFER, a  , 0x200);
    int retVal = flpydsk_write_sector(i);
    return retVal;
}

int32_t flpydsk_read_sector_ia( int32_t i, void* a)
{
    uint8_t* retVal = flpydsk_read_sector(i); // retVal should be DMA_BUFFER
    memcpy( a, (void*)DMA_BUFFER, 0x200);
    if(retVal == (uint8_t*)DMA_BUFFER)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*****************************************************************************
  The following functions are derived from source code of the dynacube team.
  which was published in the year 2004 at http://www.dynacube.net

  This functions are free software and can be redistributed/modified
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License,
  or any later version.
*****************************************************************************/

int32_t flpydsk_write_boot_sector(struct boot_sector *bs)
{
    int32_t i,j;
    uint8_t a[512];

    flpydsk_read_sector_ia( BOOT_SEC, a); ///TEST

    i=0;
    for(j=0;j<3;j++)
    {
        a[j]=bs->jumpBoot[j];
    }

    i+= 3;
    for(j=0;j<8;j++,i++)
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

    for(j=0;j<11;j++,i++)
    {
        a[i] = bs->VolumeLabel[j];
    }

    for(j=0;j<8;j++,i++)
    {
        a[i] = bs->Reserved2[j];
    }

    // boot signature
    a[510]= 0x55; a[511]= 0xAA;

    return flpydsk_write_sector_ia(BOOT_SEC,a);
}

int32_t flpydsk_write_dir(struct dir_entry* rs, int32_t in, int32_t st_sec)
{
    uint8_t a[512];
    int32_t i,j;

    st_sec = st_sec + in/DIR_ENTRIES; //??

    flpydsk_read_sector_ia( st_sec, a); ///TEST

    /*
    if(flpydsk_read_sector(st_sec) != 0)        /// read sector !!!
    {
        return E_DISK;
    }
    */

    printformat("\nwriting directory to sector %d in %d\n", st_sec, in );
    i = (in % DIR_ENTRIES) * 32;

    for(j=0;j<8;j++,i++)
    {
        a[i] = rs->Filename[j];
    }

    for(j=0;j<3;j++,i++)
    {
        a[i] = rs->Extension[j];
    }

    a[i++] = rs->Attributes;
    a[i++] = rs->NTRes;
    a[i++] = rs->CrtTimeTenth;

    a[i]   = BYTE1(rs->CrtTime);
    a[i+1] = BYTE2(rs->CrtTime);
    i+= 2;

    a[i]   = BYTE1(rs->CrtDate);
    a[i+1] = BYTE2(rs->CrtDate);
    i+= 2;

    a[i]   = BYTE1(rs->LstAccDate);
    a[i+1] = BYTE2(rs->LstAccDate);
    i+=2;

    a[i]   = BYTE1(rs->FstClusHI);
    a[i+1] = BYTE2(rs->FstClusHI);
    i+=2;

    a[i]   = BYTE1(rs->WrtTime);
    a[i+1] = BYTE2(rs->WrtTime);
    i+=2;

    a[i]   = BYTE1(rs->WrtDate);
    a[i+1] = BYTE2(rs->WrtDate);
    i+=2;

    a[i]   = BYTE1(rs->FstClusLO);
    a[i+1] = BYTE2(rs->FstClusLO);
    i+=2;

    a[i]   = BYTE1(rs->FileSize);
    a[i+1] = BYTE2(rs->FileSize);
    a[i+2] = BYTE3(rs->FileSize);
    a[i+3] = BYTE4(rs->FileSize);
    i+=4;

    return flpydsk_write_sector_ia(st_sec,a);
}

int32_t flpydsk_format(char* vlab) //VolumeLabel
{
    int32_t retVal;
    struct  boot_sector b;
    struct  dir_entry   d;
    uint8_t a[512];
    int32_t i, j;

    /*
       int32_t dt, tm; // for VolumeSerial
    */

    for(i=0;i<11;i++)
    {
        if(vlab[i] == '\0')
        {
            break;
        }
    }

    for(j=i;j<11;j++)
    {
        vlab[j] = ' ';
    }

      b.jumpBoot[0] = 0xeb;
      b.jumpBoot[1] = 0x3c;
      b.jumpBoot[2] = 0x90;

      b.SysName[0] = 'P';
      b.SysName[1] = 'R';
      b.SysName[2] = 'E';
      b.SysName[3] = 'T';
      b.SysName[4] = 'T';
      b.SysName[5] = 'Y';
      b.SysName[6] = 'O';
      b.SysName[7] = 'S';

      b.charsPerSector    =  512;
      b.SectorsPerCluster =    1;
      b.ReservedSectors   =    1;
      b.FATcount          =    2;
      b.MaxRootEntries    =  224;
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
      b.VolumeSerial = /* tm << 16 + dt; */ 12345678;

      for(i=0;i<11;i++)
      {
          b.VolumeLabel[i] = vlab[i];
      }
      b.Reserved2[0] = 'F';
      b.Reserved2[1] = 'A';
      b.Reserved2[2] = 'T';
      b.Reserved2[3] = '1';
      b.Reserved2[4] = '2';
      b.Reserved2[5] = ' ';
      b.Reserved2[6] = ' ';
      b.Reserved2[7] = ' ';

      /// write bootsector
      retVal = flpydsk_write_boot_sector(&b);
      if(retVal != 0)
      {
          return E_DISK;
      }

      for(i=0;i<512;i++)
      {
          a[i] = 0;
      }
      printformat("Format Completed (in percent):\n");

      /// write!
      for(i=1;i<33;i++)
      {
          retVal = flpydsk_write_sector_ia(i,a);
          if(retVal != 0)
          {
              return E_DISK;
          }
          printformat("%d  ",(int32_t)i*100/32);
      }

      /// write two reserved FAT entries and others as 0
      //FAT1
      flpydsk_read_sector_ia( FAT1_SEC,a); ///TEST
      a[0]=0xF0; a[1]=0xFF; a[2]=0xFF;
      for(i=3;i<512;i++)
      {
          a[i]=0;
      }
      flpydsk_write_sector_ia(FAT1_SEC,a);

      //FAT2
      flpydsk_read_sector_ia( FAT2_SEC,a); ///TEST
      a[0]=0xF0; a[1]=0xFF; a[2]=0xFF;
      for(i=3;i<512;i++)
      {
          a[i]=0;
      }
      flpydsk_write_sector_ia(FAT2_SEC,a);

      /*************************************/

      a[0]=0; a[1]=0; a[2]=0;

      /// write!
      for(i=FAT1_SEC+1;i<FAT2_SEC;i++)
      {
          retVal = flpydsk_write_sector_ia(i,a);
	      if(retVal != 0)
	      {
		      return E_DISK;
	      }

          retVal = flpydsk_write_sector_ia(i+9,a);
	      if(retVal != 0)
	      {
		      return E_DISK;
	      }
      }

      /*************************************/

      //form volume root dir entry
      for(i=0;i<8;i++)
      {
          d.Filename[i] = vlab[i];
      }
      for(i=0;i<3;i++)
      {
          d.Extension[i] = vlab[8+i];
      }
      d.Attributes   = ATTR_VOLUME_ID | ATTR_ARCHIVE;
      d.NTRes        = 0;
      d.CrtTimeTenth = 0;
      d.CrtTime      = 0;
      d.CrtDate      = 0;
      d.LstAccDate   = 0;
      d.FstClusHI    = 0;
      d.WrtTime      = 0; /* form_time(); */
      d.WrtDate      = 0; /* form_date(); */
      d.FstClusLO    = 0;
      d.FileSize     = 0;

     /// write directory
     retVal = flpydsk_write_dir( &d, 0, ROOT_SEC );
     if(retVal != 0)
     {
         return E_DISK;
     }

     printformat("\nFormat Complete.\n");

     return 0;
}
