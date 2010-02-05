#include "flpydsk.h"
#include "fat12.h"

/*
Links:
http://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
*/

uint8_t track0[9216];
uint8_t track1[9216];
uint8_t file[51200];

int32_t flpydsk_read_directory()
{
    int32_t error = -1; // return value

	/// TEST
	memset((void*)DMA_BUFFER, 0x0, 0x2400); // 18 sectors: 18 * 512 = 9216 = 0x2400
	/// TEST

	uint8_t* retVal = flpydsk_read_sector(19);   // start at 0x2600: root directory (14 sectors)
	if(retVal != (uint8_t*)DMA_BUFFER)
	{
	    printformat("\nerror: buffer = flpydsk_read_sector(...): %X\n",retVal);
	}
	printformat("<Floppy Disc Directory>\n");

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

            start = DMA_BUFFER + i*32 + 8; // extension

			if(((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x08 ) == 0x08) ||  // volume label
			     ( ( *((uint8_t*) (start))   == 0x20) &&
			       ( *((uint8_t*) (start+1)) == 0x20) &&
			       ( *((uint8_t*) (start+2)) == 0x20) ))                          // extension == three 'space'
			{
			    // do nothing
			}
			else
			{
			    printformat("."); // usual separator between file name and file extension
			}

			count = 3;
			end = (int8_t*)(start+count);
			for(; count!=0; --count)
				printformat("%c",*(end-count));

			// filesize
			printformat("\t%d byte", *((uint32_t*)(DMA_BUFFER + i*32 + 28)));

            // attributes
			printformat("\t");
			if(*((uint32_t*)(DMA_BUFFER + i*32 + 28))<100)                   printformat("\t");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x08 ) == 0x08 ) printformat(" (vol)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x10 ) == 0x10 ) printformat(" (dir)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x01 ) == 0x01 ) printformat(" (r/o)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x02 ) == 0x02 ) printformat(" (hid)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x04 ) == 0x04 ) printformat(" (sys)");
			if((( *((uint8_t*)(DMA_BUFFER + i*32 + 11)) ) & 0x20 ) == 0x20 ) printformat(" (arc)");

			// 1st cluster: physical sector number  =  33  +  FAT entry number  -  2  =  FAT entry number  +  31
            printformat("  1st sector: %d", *((uint16_t*)(DMA_BUFFER + i*32 + 26))+31);
            printformat("\n"); // next root directory entry
		  }//if
	}//for
    printformat("\n");
    return error;
}

int32_t flpydsk_write_sector_ia( int32_t i, void* a)
{
    memcpy((void*)DMA_BUFFER, a  , 0x200);

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;
    while( flpydsk_write_sector_wo_motor(i) != 0 ) // without motor on/off
    {
        retVal = -1;
        timeout--;
        printformat("error write_sector. attempts left: %d\n",timeout);
	    if(timeout<=0)
	    {
	        printformat("timeout\n");
	        break;
	    }
    }
    if(retVal==0)
    {
        // printformat("success write_sector.\n");
    }
    return retVal;
}

int32_t flpydsk_write_track_ia( int32_t track, void* trackbuffer)
{
    memcpy((void*)DMA_BUFFER, trackbuffer, 0x2400);

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;
    while( flpydsk_write_sector_wo_motor(track*18) != 0 ) // without motor on/off
    {
        retVal = -1;
        timeout--;
        printformat("error write_track: %d. left attempts: %d\n",track,timeout);
	    if(timeout<=0)
	    {
	        printformat("timeout\n");
	        break;
	    }
    }
    if(retVal==0)
    {
        // printformat("success write_track: %d.\n",track);
    }
    return retVal;
}


int32_t flpydsk_read_sector_ia( int32_t i, void* a)
{
    uint8_t* retVal = flpydsk_read_sector_wo_motor(i); // retVal should be DMA_BUFFER
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

int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* file)
{
    uint8_t a[512];
    uint32_t sectornumber;
    uint32_t i, pos;  // i for FAT-index, pos for data position
    const uint32_t ADD = 31;

    // copy first cluster
    sectornumber = firstCluster+ADD;
    printformat("\n\n1st sector: %d\n",sectornumber);
    flpydsk_read_sector_ia(sectornumber,a); // read
    memcpy( file, (void*)a, 512);

    // // find second cluster and chain in fat
    pos=0;
    i = firstCluster;
    while(fatEntry[i]!=0xFFF)
    {
        printformat("\ni: %d FAT-entry: %x\n",i,fatEntry[i]);

        // copy data from chain
        pos++;
        sectornumber = fatEntry[i]+ADD;
        printformat("\nsector: %d\n",sectornumber);
        flpydsk_read_sector_ia(sectornumber,a); // read
        memcpy( (void*)(file+pos*512), (void*)a, 512);

        // search next cluster of the file
        i = fatEntry[i];
    }
    return 0;
}

/*****************************************************************************
  The following functions are derived from source code of the dynacube team.
  which was published in the year 2004 at http://www.dynacube.net

  This functions are free software and can be redistributed/modified
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License,
  or any later version.
*****************************************************************************/

int32_t flpydsk_prepare_boot_sector(struct boot_sector *bs)
{
    int32_t i,j,k;
    uint8_t a[512];

    flpydsk_read_sector_ia( BOOT_SEC, a ); ///TEST

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


    // flpydsk_control_motor(true); printformat("write_boot_sector.motor_on\n");
    // return flpydsk_write_sector_ia( BOOT_SEC, a );
    /// prepare sector 0 of track 0
    for(k=0;k<511;k++)
    {
        track0[k] = a[k];
    }
    return 0;
}



int32_t flpydsk_format(char* vlab) //VolumeLabel
{
    struct boot_sector b;
    uint8_t a[512];
    int32_t i,j;
    /*
       int32_t dt, tm; // for VolumeSerial
    */
    flpydsk_control_motor(true);

    ///TEST TODO: auslagern in eigene Funktion
    printformat("Search for a file\n");

    struct file f;
    uint32_t firstCluster = search_file_first_cluster("BOOT2   ","BIN", &f);
    printformat("FirstCluster (retVal): %d\n",firstCluster);
    printformat("FileSize: %d FirstCluster: %d\n",f.size, f.firstCluster);

    printformat("\nFAT1 parsed 12-bit-wise: ab cd ef --> dab efc\n");
    int32_t fat_entry[FATMAXINDEX];
    for(i=0;i<FATMAXINDEX;i++)
    {
        read_fat(&fat_entry[i], i, FAT1_SEC);
    }


    file_ia(fat_entry,firstCluster,file);
    printformat("\nFile content: ");
    printformat("\n1st sector:\n");
    for(i=0;i<20;i++)
    {
        printformat("%x ",file[i]);
    }
    printformat("\n2nd sector:\n");
    for(i=512;i<532;i++)
    {
        printformat("%x ",file[i]);
    }
    printformat("\n3rd sector:\n");
    for(i=1024;i<1044;i++)
    {
        printformat("%x ",file[i]);
    }
    ///TEST



    printformat("\n\nFormat process started.\n");

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

    b.SysName[0] = 'M';
    b.SysName[1] = 'S';
    b.SysName[2] = 'W';
    b.SysName[3] = 'I';
    b.SysName[4] = 'N';
    b.SysName[5] = '4';
    b.SysName[6] = '.';
    b.SysName[7] = '1';

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
    b.VolumeSerial = 0x12345678;
    /* b.VolumeSerial = tm << 16 + dt; */

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


    /// bootsector
    flpydsk_prepare_boot_sector(&b);
    // printformat("bootsector prepared.\n");

    /// prepare FATs
    for(i=512;i<9216;i++)
    {
        track0[i] = 0;
    }
    for(i=0;i<9216;i++)
    {
        track1[i] = 0;
    }

    a[0]=0xF0; a[1]=0xFF; a[2]=0xFF;
    for(i=0;i<3;i++)
    {
        track0[FAT1_SEC*512+i]=a[i]; // FAT1 starts at 0x200 (sector 1)
        track0[FAT2_SEC*512+i]=a[i]; // FAT2 starts at 0x1400 (sector 10)
    }

    /// prepare first root directory entry (volume label)
    for(i=0;i<11;i++)
    {
        track1[512+i] = vlab[i];
    }
    track1[512+11] = ATTR_VOLUME_ID | ATTR_ARCHIVE;

    for(i=7680;i<9216;i++)
    {
        track1[i] = 0xF6; // format ID of MS Windows
    }

    /// write track 0 & track 1
    flpydsk_control_motor(true); printformat("writing tracks 1 & 2\n");
    flpydsk_write_track_ia( 0, track0);
    flpydsk_write_track_ia( 1, track1);
    printformat("Quickformat complete.\n\n");

    ///TEST
    printformat("Content of Disc:\n");
    struct dir_entry entry;
    for(i=0;i<224;i++)
    {
        read_dir(&entry, i, 19, false); // testweise unterdrückt
        if(strcmp((&entry)->Filename,"")==0)
        {
            break;
        }
    }
    printformat("\n");
    ///TEST

    return 0;
}


void parse_dir(uint8_t* a, int32_t in, struct dir_entry* rs)
{
   int32_t i,j;
   i = (in %DIR_ENTRIES) * 32;

   for(j=0;j<8;j++,i++)
   {
       rs->Filename[j] = a[i];
   }

   for(j=0;j<3;j++,i++)
   {
       rs->Extension[j] = a[i];
   }

   rs->Attributes   = a[i++];
   rs->NTRes        = a[i++];
   rs->CrtTimeTenth = a[i++];
   rs->CrtTime      = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->CrtDate      = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->LstAccDate   = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->FstClusHI    = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->WrtTime      = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->WrtDate      = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->FstClusLO    = FORM_INT(a[i],a[i+1]);                  i+=2;
   rs->FileSize     = FORM_LONG(a[i],a[i+1],a[i+2],a[i+3]);   i+=4;
}

void print_dir(struct dir_entry* rs)
{
    int32_t j;

    if(strcmp(rs->Filename,"")!=0)
    {
        printformat("File Name : ");
        for(j=0;j<8;j++)
        {
            printformat("%c",rs->Filename[j]);
        }
        printformat("\n");
        printformat("Extension : ");
        for(j=0;j<3;j++)
        {
            printformat("%c",rs->Extension[j]);
        }
        printformat("\n");
        printformat("Attributes   = %d\t %x\n",rs->Attributes,   rs->Attributes      );
        printformat("NTRes        = %d\t %x\n",rs->NTRes,        rs->NTRes           );
        printformat("CrtTimeTenth = %d\t %x\n",rs->CrtTimeTenth, rs->CrtTimeTenth    );
        printformat("CrtTime      = %d\t %x\n",rs->CrtTime,      rs->CrtTime         );
        printformat("CrtDate      = %d\t %x\n",rs->CrtDate,      rs->CrtDate         );
        printformat("LstAccDate   = %d\t %x\n",rs->LstAccDate,   rs->LstAccDate      );
        printformat("FstClusHI    = %d\t %x\n",rs->FstClusHI,    rs->FstClusHI       );
        printformat("WrtTime      = %d\t %x\n",rs->WrtTime,      rs->WrtTime         );
        printformat("WrtDate      = %d\t %x\n",rs->WrtDate,      rs->WrtDate         );
        printformat("FstClusLO    = %d\t %x\n",rs->FstClusLO,    rs->FstClusLO       );
        printformat("FileSize     = %d\t %x\n",rs->FileSize,     rs->FileSize        );
        printformat("\n");
    }
}

// Disk --sector_read--> a[...] --parse_dir--> rs --print_dir--> ScreenOutput

int32_t read_dir(struct dir_entry* rs, int32_t in, int32_t st_sec, bool flag)
{
   uint8_t a[512];
   st_sec = st_sec + in/DIR_ENTRIES;

   if(flpydsk_read_sector_ia(st_sec,a) != 0)
   {
       return E_DISK;
   }
   parse_dir(a,in,rs);
   if(flag==true)
   {
       print_dir(rs);
   }
   return 0;
}

uint32_t search_file_first_cluster(char* name, char* ext, struct file* f)
{
   struct dir_entry entry;
   int32_t i,j;
   char buf1[10];
   char buf2[5];

   for(i=0;i<224;i++)
   {
       read_dir(&entry, i, 19, false);
       // printformat("Filename: %s\n",(&entry)->Filename);
       // printformat("Fileext : %s\n",(&entry)->Extension);
       for(j=0;j<3;j++)
       {
           buf1[j] = (&entry)->Filename[j];
           buf2[j] = (&entry)->Extension[j];
       }
       for(j=3;j<8;j++)
       {
           buf1[j] = (&entry)->Filename[j];
       }
       buf1[8]=0; //string termination Filename
       buf2[3]=0; //string termination Extension

       if((strcmp(buf1,name)==0) && (strcmp(buf2,ext)==0))
       {
           break;
       }
    }
    f->size = (&entry)->FileSize;
    f->firstCluster = FORM_INT((&entry)->FstClusLO,(&entry)->FstClusHI);

    return f->firstCluster;
}

void parse_fat(int32_t* fat_entry, int32_t fat1, int32_t fat2, int32_t in)
{
    int32_t fat;
    if(in%2 == 0)
    {
        fat = ((fat2 & 0x0f) << 8) | fat1;
    }
    else
    {
        fat = (fat2 << 4) | ((fat1 &0x0f0) >> 4);
    }
    fat = fat & 0xFFF;
    *fat_entry = fat;
    printformat("%x ", fat);
}

int32_t read_fat(int32_t* fat_entry, int32_t in, int32_t st_sec)
{
    int32_t fat_in, res;
    uint8_t a[512];
    int32_t fat1, fat2;
    fat_in = (in*3)/2;
    st_sec = st_sec + fat_in/512;
    if(fat_in % 512 == 511)
    {
        //spans sectors
        res = flpydsk_read_sector_ia(st_sec,a);
        if(res != 0)
        {
            return E_DISK;
        }
        fat1 = a[511];
        res = flpydsk_read_sector_ia(st_sec+1,a);
        fat2 = a[0];
    }
    else
    {
        fat_in = fat_in%512;
        res = flpydsk_read_sector_ia(st_sec,a);
        if(res != 0)
        {
            return E_DISK;
        }
        fat1 = a[fat_in] & 0xff;
        fat2 = a[fat_in+1] & 0xff;
     }
     parse_fat(fat_entry,fat1,fat2,in);
     //Even And Odd logical clusters

     return 0;
}

/*

Clusters are numbered from a cluster offset as defined above (?) and the FilestartCluster is in 0x1a.
This would mean the first data segment X can be calculated using the Boot Sector fields:

FileStartSector =
ReservedSectors(0x0e)
+ (NumofFAT(0x10) * Sectors2FAT(0x16))
+ (MaxRootEntry(0x11) * 32 / BytesPerSector(0x0b))
+ ((X − 2) * SectorsPerCluster(0x0d))

*/


/*
int32_t flpydsk_write_dir(struct dir_entry* rs, int32_t in, int32_t st_sec)
{
    uint8_t a[512];
    int32_t i,j;

    st_sec = st_sec + in/DIR_ENTRIES; // ??

    flpydsk_read_sector_ia( st_sec, a );

    printformat("\nwriting directory to sector %d\n", st_sec);
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

    ///TODO: do not use sector-wise writing!
    flpydsk_control_motor(true); printformat("write_dir.motor_on\n");
    return flpydsk_write_sector_ia(st_sec,a);
}
*/
