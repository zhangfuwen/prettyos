/*
*  This code is based on example code for the PIC18F4550 given in the book:
*  Jan Axelson, "USB Mass Storage Device" (2006), web site: www.Lvr.com
*
*  The copyright, page ii, tells:
*  "No part of the book, except the program code, may be reproduced or transmitted in any form by any means
*  without the written permission of the publisher. The program code may be stored and executed in a computer system
*  and may be incorporated into computer programs developed by the reader."
*
*  I am a reader of this helpful book (Erhard Henkes).
*
*  We adapted this sourcecode to the needs of PrettyOS.
*/

#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "usb2_msd.h"
#include "console.h"
#include "fat.h"

// #define ALLOW_WRITES <--- does not yet work!

uint8_t  gFATBuffer[512];                  // The global FAT sector buffer 
bool     gBufferZeroed       = false;      // Global variable indicating that the data buffer contains all zeros
FILEOBJ  gBufferOwner        = NULL;       // Global variable indicating which file is using the data buffer
uint32_t gLastDataSectorRead = 0xFFFFFFFF; // Global variable indicating which data sector was read last
uint32_t gLastFATSectorRead  = 0xFFFFFFFF; // Global variable indicating which FAT sector was read last
bool     gNeedFATWrite       = false;      // Global variable indicating that there is information that needs to be written to the FAT
bool     gNeedDataWrite      = false;      // Global variable indicating that there is data in the buffer that hasn't been written to the device.

// prototypes
static uint32_t fatWrite(PARTITION* volume, uint32_t cls, uint32_t v);

static uint32_t cluster2sector(PARTITION* volume, uint32_t cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cluster2sector <<<<<!");
  #endif

    uint32_t sector = 0;

    switch (volume->type)
    {
    case FAT32:
        /* In FAT32, there is no separate ROOT region. It is as well stored in DATA region */
        if (cluster <= 1) // ??
        {
            sector = volume->data + cluster * volume->SecPerClus;
        }
        else
        {
            sector = volume->data + (cluster-2) * volume->SecPerClus;
        }
        break;

    case FAT12:
    case FAT16:
    default:
        // The root dir takes up cluster 0 and 1
        if (cluster <= 1) // root dir FAT16
        {
            sector = volume->root + (cluster * volume->SecPerClus);
        }
        else // data area
        {
            sector = volume->data + (cluster-2) * volume->SecPerClus;
        }
        break;
    }

  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cluster2sector<<<<<    cluster: %u  sector %u", cluster, sector);
  #endif
    return (sector);
}

uint8_t sectorWrite(uint32_t sector_addr, uint8_t* buffer) // to implement
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorWrite <<<<<!");
  #endif
    textColor(0x0A); printf("\n>>>>> sectorWrite not yet implemented <<<<<!"); textColor(0x0F);
    uint8_t retVal = SUCCESS; // TEST
    return retVal;
}

#ifdef ALLOW_WRITES
static uint8_t flushData()
{
    uint32_t l;
    PARTITION* volume;

    // This will either be the pointer to the last file, or the handle
    FILEOBJ stream = gBufferOwner;

    volume = stream->volume;

    // figure out the lba
    l = cluster2sector(volume,stream->ccls);
    l += (uint16_t)stream->sec; // add the sector number to it

    if(!sectorWrite( l, volume->buffer, false))
    {
        return CE_WRITE_ERROR;
    }

    gNeedDataWrite = false;

    return CE_GOOD;
}
#endif

uint8_t sectorRead(uint32_t sector_addr, uint8_t* buffer) // make it better!
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorRead <<<<<!");
  #endif
    uint8_t retVal = SUCCESS; // TEST
    usbRead(sector_addr, buffer); // until now only realized for USB 2.0 Mass Storage Device
    return retVal;
}

static uint32_t fatRead (PARTITION* volume, uint32_t ccls)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatRead <<<<<!");
  #endif

    uint8_t q;
    uint32_t p, l;  // "l" is the sector Address
    uint32_t c = 0, d, ClusterFailValue, LastClusterLimit;   // ClusterEntries
    gBufferZeroed = false;

    switch (volume->type)
    {
        case FAT32:
            p = ccls * 4;
            q = 0; // "q" not used for FAT32, only initialized to remove a warning
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            LastClusterLimit = LAST_CLUSTER_FAT32;
            break;
        case FAT12:
            p = (uint32_t) ccls *3;  // Mulby1.5 to find cluster pos in FAT
            q = p&1;
            p >>= 1;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT12;
            break;
        case FAT16:
        default:
            p = (uint32_t)ccls *2;     // Mulby 2 to find cluster pos in FAT
            q = 0; // "q" not used for FAT16, only initialized to remove a warning
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT16;
            break;
    }

    l = volume->fat + (p / volume->sectorSize);  
    
    printf("\nline: %u, l = volume->fat + (p / volume->sectorSize): %u",__LINE__,l); //TEST  
    
    p &= volume->sectorSize - 1;                 // Restrict 'p' within the FATbuffer size

    printf("\nline: %u, p &= volume->sectorSize - 1 : %u",__LINE__,p); //TEST  

    // Check if the appropriate FAT sector is already loaded
    if (gLastFATSectorRead == l)
    {
        printf("\ngLastFATSectorRead == l"); //TEST

        if (volume->type == FAT32)
        {
            c = RAMreadLong (gFATBuffer, p);
            
            printf("\nc = RAMreadLong (gFATBuffer, p) : %u",c); //TEST
        }
        else if(volume->type == FAT16)
        {
            c = RAMreadW (gFATBuffer, p);
        }
        else if(volume->type == FAT12)
        {
            c = RAMread (gFATBuffer, p);
            if (q)
            {
                c >>= 4;
            }
            // Check if the MSB is across the sector boundry
            p = (p +1) & (volume->sectorSize-1);
            if (p == 0)
            {
                // Start by writing the sector we just worked on to the card
                // if we need to
         #ifdef ALLOW_WRITES
                if (gNeedFATWrite)
                    if(fatWrite(volume, 0, 0, true))
                        return ClusterFailValue;
         #endif
                if (sectorRead (l+1, gFATBuffer) != SUCCESS)
                {
                    gLastFATSectorRead = 0xFFFF;
                    return ClusterFailValue;
                }
                else
                {
                    gLastFATSectorRead = l+1;
                }
            }
            d = RAMread (gFATBuffer, p);
            if (q)
            {
                c += (d <<4);
            }
            else
            {
                c += ((d & 0x0F)<<8);
            }
        }
    }
    else
    {
        printf("\nline: %u",__LINE__);

        // If there's a currently open FAT sector,
        // write it back before reading into the buffer
#ifdef ALLOW_WRITES
        if (gNeedFATWrite)
        {
            if(WriteFAT (volume, 0, 0, TRUE))
                return ClusterFailValue;
        }
#endif
        if ( sectorRead (l, gFATBuffer) != SUCCESS)
        {
            printf("\nline: %u",__LINE__);

            gLastFATSectorRead = 0xFFFF;  // Note: It is Sector not Cluster.
            return ClusterFailValue;
        }
        else
        {
            printf("\nline: %u",__LINE__);

            gLastFATSectorRead = l;
            if (volume->type == FAT32)
            {
                c = RAMreadLong (gFATBuffer, p);
                printf("\nline: %u, c = RAMreadLong (gFATBuffer, p) : %u",__LINE__,c); //TEST
            }
            else
            {
                if(volume->type == FAT16)
                {
                    c = RAMreadW (gFATBuffer, p);
                }
                else if (volume->type == FAT12)
                {
                    c = RAMread (gFATBuffer, p);
                    if (q)
                    {
                        c >>= 4;
                    }
                    p = (p +1) & (volume->sectorSize-1);
                    d = RAMread (gFATBuffer, p);
                    if (q)
                    {
                        c += (d <<4);
                    }
                    else
                    {
                        c += ((d & 0x0F)<<8);
                    }
                }
            }
        }
    }

    // Normalize it so 0xFFFF is an error
    if (c >= LastClusterLimit)
    {
        c = LastClusterLimit;
    }
    return c;
}

/*
static uint32_t fatRead(PARTITION* volume, uint32_t ccls)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatRead <<<<<!");
  #endif
    uint32_t l=0;
    if (volume->type == FAT16)
    {
        l = volume->fat + (ccls>>8); // sector contains 256 words
    }
    else if (volume->type == FAT32)
    {
        l = volume->fat + (ccls>>7); // sector contains 128 uint32_t
    }

    if ( sectorRead(l, volume->buffer) != SUCCESS )
    {
        return(CLUSTER_FAIL);
    }

    uint32_t c=0;

    if (volume->type == FAT16)
    {
        // To find the number of the next cluster,
        // read 2 bytes in the buffer of retrieved data
        // beginning at offset = low byte of current cluster's address << 1
        // Shift left 1 (multiply by 2) because each entry is 2 bytes
        c = RAMreadW(volume->buffer, ((ccls&0xFF)<<1));
    }
    else if (volume->type == FAT32)
    {
        c = RAMreadLong(volume->buffer, ((ccls&0x7F)<<2));
    }

    if ((volume->type == FAT16) && (c >= LAST_CLUSTER_FAT16))
    {
        return(LAST_CLUSTER);
    }
    else if (volume->type == FAT32)
    {
        // what to do? 
    }

    return(c);
}
*/

static uint32_t fatWrite(PARTITION* volume, uint32_t cls, uint32_t v)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatWrite <<<<<!");
  #endif
    uint32_t  c;

    uint32_t p = 2*cls;
    uint32_t l = volume->fat + (p>>9);
    p &= 0x1FF;

    if (sectorRead(l,volume->buffer) != SUCCESS) 
    {
        return FAIL;
    }

    RAMwrite(volume->buffer,p,v);
    RAMwrite(volume->buffer,p+1,(v>>8));

    for (uint32_t i=0; i<volume->fatcopy; i++)
    {
        if (sectorWrite(l,volume->buffer) != SUCCESS)
        {
            return FAIL;
        }
    }

    if ((volume->type == FAT16) && (c>= LAST_CLUSTER_FAT16))
    {
        c = LAST_CLUSTER_FAT16; /// Should return directly
    }

    return c; /// Correct? c is uninitialized...
}


static uint8_t fileSearchNextCluster(FILEOBJ fo, uint32_t n)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileSearchNextCluster <<<<<!");
  #endif
    PARTITION* volume = fo->volume;

    do
    {
        uint32_t c2 = fo->ccls;
		uint32_t c = fatRead(volume,c2);
        if (c==FAIL)
        {
            return  CE_BAD_SECTOR_READ;
        }
        else
        {
            if (c>=volume->maxcls)
            {
                return  CE_INVALID_CLUSTER;
            }
            if (volume->type == FAT16)
            {
                c2 = LAST_CLUSTER_FAT16;
                if (c>=c2)
                {
                    return  CE_FAT_EOF;
                }
            }
        }
        fo->ccls = c;
    }
    while (--n > 0);

    return CE_GOOD;
}

/*
static uint32_t fatFindEmptyCluster(FILEOBJ fo)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatFindEmptyCluster <<<<<!");
  #endif
    uint32_t value = 0x0;

    PARTITION* volume = fo->volume;
    uint32_t c      = fo->ccls;

    if (c<2)
    {
        c=2;
    }

    uint32_t curcls = c;

    fatRead(volume,c);

    while (c)
    {
        c++;

        if ( ((volume->type == FAT16) && (value == END_CLUSTER)) || (c >= volume->maxcls))
        {
            c=2;
        }

        if (c == curcls)
        {
            return 0;
        }

        if ((value = fatReadQueued(volume,c)) == CLUSTER_FAIL_FAT16)
        {
            return 0;
        }

        if (value == CLUSTER_EMPTY)
        {
            break;
        }
    }
    return c;
}
*/


uint32_t checksum(char* ShortFileName)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> checksum <<<<<!");
  #endif
    uint32_t Bit7, Checksum=0;
    for (uint32_t Character=0; Character<11; ++Character)
    {
        if (Checksum & 1)
        {
            Bit7 = 0x80;
        }
        else
        {
            Bit7 = 0x0;
        }

        Checksum = Checksum >> 1;
        Checksum |= Bit7;
        Checksum += ShortFileName[Character];
        Checksum &= 0xFF;
    }
    return Checksum;
}



///////////////
// directory //
///////////////

/*
  Input:
    fo -         File information
    curEntry -   Offset of the directory entry to load
    ForceRead -  Forces loading of a new sector of the directory
  Return:
    DIRENTRY - Pointer to the directory entry that was loaded
*/

static DIRENTRY cacheFileEntry(FILEOBJ fo, uint32_t* curEntry, bool ForceRead) 
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %u ForceRead: %u", *curEntry, ForceRead);
  #endif
    PARTITION* volume     = fo->volume;    
    uint32_t   LastClusterLimit, numofclus = 0;
    uint32_t   ccls       = fo->dirccls; // directory's cluster to begin looking in // unused if ForceRead is true
	uint32_t   cluster    = fo->dirclus; // first cluster of the file's directory
    uint32_t   DirectoriesPerSector = volume->sectorSize/NUMBER_OF_BYTES_IN_DIR_ENTRY;

    // Get the number of the entry's sector within the root dir.
    uint32_t offset2 = (*curEntry)/DirectoriesPerSector;                          //////////////////////////
    
    switch (volume->type)
    {
        case FAT32:
            // the ROOT is always cluster based in FAT32
            /* In FAT32: There is no ROOT region. Root etries are made in DATA region only.
            Every cluster of DATA which is accupied by ROOT is tracked by FAT table/entry so the ROOT can grow
            to an amount which is restricted only by available free DATA region. */
            offset2  = offset2 % (volume->SecPerClus);   // figure out the offset
            LastClusterLimit = LAST_CLUSTER_FAT32;
            break;

        case FAT12:
        case FAT16:
        default:
            // if its the root its not cluster based
            if(cluster != 0)
            {
                offset2  = offset2 % (volume->SecPerClus);   // figure out the offset
            }
            LastClusterLimit = LAST_CLUSTER_FAT16;
            break;
    }

    // check if a new sector of the root must be loaded
    if ( ForceRead || ((*curEntry & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0) ) // only 16 entries per sector
    {
        // see if we have to load a new cluster
        if(((offset2 == 0) && ((*curEntry) >= DirectoriesPerSector)) || ForceRead)               //////////////////////////
        {
            if (cluster == 0 )
            {
                ccls = 0;
            }
            else
            {
                if (ForceRead) // If ForceRead, read the number of sectors from 0
                {
                    numofclus = (*curEntry)/(DirectoriesPerSector * volume->SecPerClus);           //////////////////////////
                }
                else // Otherwise just read the next sector
                {
                    numofclus = 1;
                }

                while (numofclus) // move to the correct cluster
                {
                    ccls = fatRead(volume,ccls);

                    if (ccls >= LastClusterLimit)
                    {
                        break;
                    }
                    else
                    {
                        numofclus--;
                    }
                }
            }
        }
        
        if(ccls < LastClusterLimit) // do we have a valid cluster number?
        {
            textColor(0x0E);
            // printf("\ncacheFileEntry - current cluster: %u", ccls); /// TEST
            textColor(0x0F);

            fo->dirccls = ccls; // write it back

            uint32_t sector = cluster2sector(volume,ccls);

            /* see if we are root and about to go pass our boundaries
            FAT32 stores the root directory in the Data Region along with files and other directories,
            allowing it to grow without such a restraint */
            if( (ccls == volume->FatRootDirCluster) && (((sector + offset2) >= volume->data)) && ((volume->type != FAT32)) )
            {
                return ((DIRENTRY)NULL);   // reached the end of the root
            }
            else
            {
                textColor(0x0E);
                // printf("\ncacheFileEntry - sectorRead, sector: %u offset2: %u", sector, offset2); /// TEST
                textColor(0x0F);

              #ifdef ALLOW_WRITES
                if (gNeedDataWrite)
                    if (flushData())
                        return NULL;
              #endif
                gBufferOwner  = NULL;
                gBufferZeroed = false;

                uint8_t retVal = sectorRead( sector + offset2, volume->buffer);

                if ( retVal != SUCCESS ) // if FAIL: sector could not be read.
                {
                    return ((DIRENTRY)NULL);
                }
                else // Sector has been read properly, Copy the root entry info of the file searched.
                {
                    if(ForceRead)    // Buffer holds all 16 root entry info. Point to the one required.
                    {
                        return (DIRENTRY)((DIRENTRY)volume->buffer) + ((*curEntry)%DirectoriesPerSector);   //////////////////////////
                    }
                    else
                    {
                        return (DIRENTRY)volume->buffer;
                    }
                    gLastDataSectorRead = 0xFFFFFFFF;
                }
            } // END: read an entry
        } // END: a valid cluster is found
        else // invalid cluster number
        {
            //nextClusterIsLast = TRUE;
            return((DIRENTRY)NULL);
        }

    }
    else
    {
        return (DIRENTRY)(((DIRENTRY)volume->buffer) + ((*curEntry)%DirectoriesPerSector));  //////////////////////////
    }
} // Cache_File_Entry


static DIRENTRY loadDirectoryAttribute(FILEOBJ fo, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> loadDirectoryAttribute <<<<<!");
  #endif    
    // printf("\nline: %u",__LINE__); 
    DIRENTRY dir = cacheFileEntry(fo,fHandle,true);
    uint8_t  a   = dir->DIR_Name[0];
    if (a == DIR_EMPTY)
    {
        dir = ((DIRENTRY)NULL);
    }
    if (dir != (DIRENTRY)NULL)
    {
        if (a == DIR_DEL)
        {
            dir = ((DIRENTRY)NULL);
        }
        else
        {
            a = dir->DIR_Attr;
            while(a==ATTR_LONG_NAME)
            {
                (*fHandle)++;
                // printf("\nline: %u",__LINE__); 
                dir = cacheFileEntry(fo,fHandle,false);
                a = dir->DIR_Attr;
            }
        }
    }
    return dir;
}

static uint8_t writeFileEntry(FILEOBJ fo, uint32_t* curEntry)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> writeFileEntry <<<<<!");
  #endif
    PARTITION* volume  = fo->volume;
    uint32_t   ccls    = fo->dirccls;
    uint32_t    offset2 = (*curEntry>>4);

    if (ccls != 0)
    {
        offset2 %= volume->SecPerClus;
    }

    uint32_t sector = cluster2sector(volume,ccls);

    return (sectorWrite(sector+offset2,volume->buffer) == SUCCESS);
}

static void updateTimeStamp(DIRENTRY dir)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> updateTimeStamp not yet implemented, does nothing <<<<<!");
  #endif
    // TODO
}

///////////////////////////
////  File Operations  ////
///////////////////////////

/*
static uint8_t eraseCluster(PARTITION* volume, uint32_t cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> eraseCluster <<<<<!");
  #endif
    uint32_t SectorAddress = cluster2sector(volume, cluster);
    memset(volume->buffer, 0x00, SDC_SECTOR_SIZE);
    for (uint32_t i=0; i<volume->SecPerClus; i++)
    {
        if (sectorWrite(SectorAddress++,volume->buffer) != SUCCESS)
        {
            return CE_WRITE_ERROR;
        }
    }
    return CE_GOOD;
}
*/


/*
static uint8_t fileCreateHeadCluster(FILEOBJ fo, uint32_t* cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileCreateHeadCluster <<<<<!");
  #endif
    PARTITION* volume = fo->volume;
    *cluster = fatFindEmptyCluster(fo);

    if (*cluster == 0)
    {
        return CE_DISK_FULL;
    }
    else
    {
        if (fatWrite(volume, *cluster, LAST_CLUSTER_FAT16)==FAIL) // FAT16 // TODO: FAT32
        {
            return CE_WRITE_ERROR;
        }
        return eraseCluster(volume,*cluster);
    }
}
*/

/*
static uint8_t createFirstCluster(FILEOBJ fo)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> createFirstCluster <<<<<!");
  #endif
    uint8_t  error;
    uint32_t cluster;

    if ((error = fileCreateHeadCluster(fo,&cluster))==CE_GOOD)
    {
		uint32_t fHandle = fo->entry;
        DIRENTRY dir = loadDirectoryAttribute(fo, &fHandle);
        dir->DIR_FstClusLO = cluster; // FAT16 // TODO: FAT32

        if (writeFileEntry(fo,&fHandle) != true)
        {
            return CE_WRITE_ERROR;
        }
    }
    return error;
}
*/

/*
static uint8_t fileAllocateNewCluster(FILEOBJ fo)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileAllocateNewCluster <<<<<!");
  #endif
    PARTITION* volume  = fo->volume;
    uint32_t c         = fatFindEmptyCluster(fo);

    if (c==0)
    {
        return CE_DISK_FULL;
    }

    fatWrite(volume,c,LAST_CLUSTER_FAT16); // FAT16 // TODO: FAT32
    uint32_t curcls = fo->ccls;
    fatWrite(volume,curcls,c);
    fo->ccls = c;
    return CE_GOOD;
}
*/

static uint8_t fillFileObject(FILEOBJ fo, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fillFileObject <<<<<!");
  #endif

    // Get the entry
    DIRENTRY dir;

    if (((*fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0) && (*fHandle != 0)) // 4-bit mask because 16 root entries max per sector
    {
        fo->dirccls = fo->dirclus;
        // printf("\nline: %u",__LINE__); 
        dir = cacheFileEntry(fo, fHandle, true);
    }
    else
    {
        // printf("\nline: %u",__LINE__); 
        dir = cacheFileEntry (fo, fHandle, false);
    }

    if ((dir==(DIRENTRY)NULL) )
    {
        return NO_MORE;
    }
    else
    {
        // read first character of file name from the entry
        uint8_t a = dir->DIR_Name[0];

        if (a == DIR_DEL)
        {
            return NOT_FOUND;
        }
		else if ( a == DIR_EMPTY)
		{
			return NO_MORE;
		}
        else
        {
			uint8_t character, test=0;

            for (uint8_t i=0; i<DIR_NAMESIZE; i++)
            {
                character = dir->DIR_Name[i];
                fo->name[test++] = character;
            }

            character = dir->DIR_Extension[0];

            if(character != ' ')
            {
                for (uint8_t i=0; i<DIR_EXTENSION; i++)
                {
                    character = dir->DIR_Extension[i];
                    fo->name[test++]=character;
                }
            }
            fo->entry = *fHandle;
            fo->size  = (dir->DIR_FileSize);

            fo->cluster = ((dir->DIR_FstClusHI)<<16) | dir->DIR_FstClusLO;

            fo->time = (dir->DIR_WrtTime);
            fo->date = (dir->DIR_WrtDate);

            a = dir->DIR_Attr;
            fo->attributes = a;
            return FOUND;
        } // END: the entry is not deleted
    } // END: an entry exists
}

uint8_t fileFind( FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd, uint8_t mode) 
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileFind <<<<<!");
  #endif

    for (uint8_t i=0; i<DIR_NAMECOMP; i++)
    {
        foDest->name[i] = 0x20; // set name and extension to spaces
    }

    uint32_t DirectoriesPerCluster = foDest->volume->sectorSize * foDest->volume->SecPerClus / NUMBER_OF_BYTES_IN_DIR_ENTRY;
	char character, test;
    uint8_t statusB        = CE_FILE_NOT_FOUND;
    
    foDest->dirccls        = foDest->dirclus; // reset the cluster
    uint16_t compareAttrib = 0xFFFF ^ foCompareTo->attributes;  

    // uint32_t fHandle = foDest->entry; // current entry counter (code in original)
    uint32_t fHandle = 0; 
    bool read = true;

label1: // TEST
    printf("\nfHandle: %d",fHandle);  

    if (fHandle == 0)
    {
        // printf("\nline: %u",__LINE__); 
        if (cacheFileEntry(foDest, &fHandle, read) == NULL)
        {
            statusB = CE_BADCACHEREAD;
        }
    }
    else
    {
        if ((fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) != 0) // Maximum 16 entries possible
        {
            // printf("\nline: %u",__LINE__); 
            if (cacheFileEntry (foDest, &fHandle, read) == NULL)
            {
                statusB = CE_BADCACHEREAD;
            }
        }
    }

    if (statusB != CE_BADCACHEREAD)
    {
		uint8_t  state=0;
		uint32_t attrib;
        while(true) // Loop until you reach the end or find the file
        {
          #ifdef _FAT_DIAGNOSIS_
            textColor(0x0E); printf("\n\nfHandle %u\n",fHandle); textColor(0x0F);
          #endif

            if (statusB != CE_GOOD) //First time entry always here
            {
                state = fillFileObject(foDest, &fHandle);
                if (state == NO_MORE) // Reached the end of available files. Comparision over and file not found so quit.
                {
                    break;
                }
            }
            else // statusB == CE_GOOD then exit
            {
                break;
            }

            if (state == FOUND)
            {
              #ifdef _FAT_DIAGNOSIS_
                textColor(0x0A);printf("\n\nstate == FOUND");textColor(0x0F);
              #endif
                attrib =  foDest->attributes;
                attrib &= ATTR_MASK;

                switch (mode)
                {
                case 0:
                    if ((attrib != ATTR_VOLUME) && ((attrib & ATTR_HIDDEN) != ATTR_HIDDEN))
                    {
                      #ifdef _FAT_DIAGNOSIS_
                        textColor(0x0A);printf("\n\nAn entry is found. Attributes OK for search");textColor(0x0F); /// TEST
                      #endif
                        statusB = CE_GOOD;
                        for (uint8_t i = 0; i < DIR_NAMECOMP; i++)
                        {
                            character = foDest->name[i];
                          #ifdef _FAT_DIAGNOSIS_
                            printf("\ni: %u character: %c test: %c", i, character, foCompareTo->name[i]); textColor(0x0F); /// TEST
                          #endif
                            if (toLower(character) != toLower(foCompareTo->name[i]))
                            {
                                statusB = CE_FILE_NOT_FOUND;
                              #ifdef _FAT_DIAGNOSIS_
                                textColor(0x0C); printf("\n\n %c <--- not equal", character); textColor(0x0F);
                              #endif
							    break; // finish for loop
                            }
                        } // for loop
                    } 
                    break; // end of case
                
                case 1:
                    // Check for attribute match
                    if (((attrib & compareAttrib) == 0) && (attrib != ATTR_LONG_NAME))
                    {
                        statusB = CE_GOOD;                 // Indicate the already filled file data is correct and go back
                        character = (char)'m';             // random value
                        if (foCompareTo->name[0] != '*')   // If "*" is passed for comparion as 1st char then don't proceed. Go back, file alreay found.
                        {
                            for (uint16_t index = 0; index < DIR_NAMESIZE; index++)
                            {
                                
                                character = foDest->name[index]; 
                                test = foCompareTo->name[index]; 
                                if (test == '*')
                                    break;
                                if (test != '?')
                                {
                                    if(toLower(character) != toLower(test))
                                    {
                                        statusB = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }
                                }
                            }
                        }

                        // Before calling this "FILEfind" fn, "formatfilename" must be called. Hence, extn always starts from position "8".
                        if ((foCompareTo->name[8] != '*') && (statusB == CE_GOOD))
                        {
                            for (uint16_t index = 8; index < DIR_NAMECOMP; index++)
                            {
                                // Get the source character
                                character = foDest->name[index];
                                // Get the destination character
                                test = foCompareTo->name[index];
                                if (test == '*')
                                    break;
                                if (test != '?')
                                {
                                    if(toLower(character) != toLower(test))
                                    {
                                        statusB = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }
                                }
                            }
                        }
                    } // Attribute match
                    break; // end of case
                } // while
            } // not found // end of if (statusB != CE_BADCACHEREAD)
            else
            {
                // looking for an empty/re-usable entry 
                if ( cmd == LOOK_FOR_EMPTY_ENTRY)
                {
                    statusB = CE_GOOD;
                }
            } // found or not
            // increment it no matter what happened
            fHandle++;

            // with FAT32 we have to find the next cluster of the root dir, if necessary  
            if (foDest->volume->type == FAT32)
            {
                uint32_t oldCluster = foDest->dirccls;
                if (fHandle > 0)
                {
                    foDest->dirccls = foDest->dirccls + (fHandle-1)/DirectoriesPerCluster; // 128 --> 0, 129 --> 1    //////////////////////////
                    read = false;
                }                
                if (foDest->dirccls > oldCluster)
                {
                    foDest->dirccls = fatRead(foDest->volume, oldCluster);
                    
                    printf("\nold  foDest->dirccls in cluster chain: %u == %X", oldCluster, oldCluster);
                    printf("\nnext foDest->dirccls in cluster chain: %u == %X", foDest->dirccls, foDest->dirccls);
                    read = true;
                    
                    //fHandle = 1; // TEST ???
                    
                    waitForKeyStroke();
                    if (foDest->dirccls == CLUSTER_FAIL_FAT32)
                    {
                        textColor(0x0C);
                        printf("\nEnd of root dir cluster chain!");
                        textColor(0x0F);
                        statusB = CE_FILE_NOT_FOUND;
                        waitForKeyStroke();
                        break;
                    }
                }
                if (fHandle > 0)
                {
                    goto label1;
                }
            }
        }// while
    }
    return(statusB);
}

/*
static uint8_t populateEntries(FILEOBJ fo, char* name, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> populateEntries <<<<<!");
  #endif

    DIRENTRY dir = cacheFileEntry(fo, fHandle, true);
    strncpy(dir->DIR_Name,name,DIR_NAMECOMP);   /// TODO: check!
    dir->DIR_Attr          = ATTR_ARCHIVE;
    dir->DIR_NTRes         = 0x00;
    dir->DIR_CrtTimeTenth  = 0x64;
    dir->DIR_CrtTime       = 0x43C5;
    dir->DIR_CrtDate       = 0x34B0;
    dir->DIR_LstAccDate    = 0x34B0;
    dir->DIR_FstClusHI     = 0x0000;
    dir->DIR_WrtTime       = 0x43C6;
    dir->DIR_WrtDate       = 0x34B0;
    dir->DIR_FstClusLO     = 0x0000;
    dir->DIR_FileSize      = 0x0000;

    fo->size        = dir->DIR_FileSize;
    fo->time        = dir->DIR_CrtTime;
    fo->date        = dir->DIR_CrtDate;
    fo->attributes  = dir->DIR_Attr;
    fo->entry       = *fHandle;

    writeFileEntry(fo,fHandle);
    return CE_GOOD;
}
*/

/*
static uint8_t findEmptyEntry(FILEOBJ fo, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> findEmptyEntry <<<<<!");
  #endif
    uint8_t  a, status = NOT_FOUND;
    uint32_t bHandle;
    DIRENTRY dir = cacheFileEntry(fo, fHandle, true);

    if (dir==NULL)
    {
        status = CE_BADCACHEREAD;
    }
    else
    {
        while (status == NOT_FOUND)
        {
            uint32_t amountfound=0;
            bHandle = *fHandle;
            do
            {
                dir = cacheFileEntry(fo, fHandle, false);
                a = dir->DIR_Name[0];
                (*fHandle)++;
            }
            while( (a==DIR_DEL || a==DIR_EMPTY) && (dir!=(DIRENTRY)NULL) && (++amountfound<1));

            if (dir==NULL)
            {
                a=fo->dirccls;
                if (a==0)
                {
                    status = NO_MORE;
                }
                else
                {
                    fo->ccls = a;
                    if (fileAllocateNewCluster(fo)==CE_DISK_FULL)
                    {
                        status = NO_MORE;
                    }
                    else
                    {
                        status = FOUND;
                    }
                }
            } // END: it is the cluster's last entry
            else
            {
                if(amountfound==1)
                {
                    status = FOUND;
                }
            }
        } // END: while (status == NOT_FOUND)

        *fHandle = bHandle;

    } // END: search for an entrys

    return(status==FOUND);
}
*/

static uint8_t fatEraseClusterChain(uint32_t cluster, PARTITION* volume)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatEraseClusterChain <<<<<!");
  #endif
    uint32_t c, c2;
    enum _status{Good,Fail,Exit} status = Good;

    if (cluster <= 1)
    {
        status = Exit;
    }
    else
    {
        while(status == Good)
        {
            if ((c = fatRead(volume,cluster)) == FAIL)
            {
                status = Fail;
            }
            else
            {
                if (c <= 1)
                {
                    status = Exit;
                }
                else
                {
                    if (volume->type == FAT16)
                    {
                        c2 = LAST_CLUSTER_FAT16;
                        if (c>=c2)
                        {
                            status = Exit;
                        }
                    }
                    if (fatWrite(volume,cluster, CLUSTER_EMPTY)==FAIL)
                    {
                        status = Fail;
                    }
                    cluster = c;
                }
            }
        } // END: while not the end of the chain and no error
    }
    return(status == Exit);
}

/*
uint8_t createFileEntry(FILEOBJ fo, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> createFileEntry <<<<<!");
  #endif
    char name[DIR_NAMECOMP];

    for (uint8_t i=0; i<FILE_NAME_SIZE; i++)
    {
        name[i] = fo->name[i];
    }

    *fHandle = 0;
    if (findEmptyEntry(fo,fHandle))
    {
        if (populateEntries(fo, name, fHandle) == CE_GOOD)
        {
            return createFirstCluster(fo);
        }
    }
    else
    {
        return CE_DIR_FULL;
    }
    return CE_GOOD;
}
*/

uint8_t fileDelete(FILEOBJ fo, uint32_t* fHandle, uint8_t EraseClusters)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileDelete <<<<<!");
  #endif
    uint8_t  status = CE_GOOD;
    uint32_t clus   = fo->dirclus;
    PARTITION* volume = fo->volume;

    fo->dirccls = clus;

    // printf("\nline: %u",__LINE__); 
    DIRENTRY dir = cacheFileEntry(fo, fHandle, true);
    uint8_t a = dir->DIR_Name[0];
    if (dir == (DIRENTRY)NULL || a==DIR_EMPTY)
    {
        return CE_FILE_NOT_FOUND;
    }
    else
    {
        if (a==DIR_DEL)
        {
            return CE_FILE_NOT_FOUND;
        }
        else
        {
            a = dir->DIR_Attr;
            dir->DIR_Name[0] = DIR_DEL;
            clus = dir->DIR_FstClusLO;
            if (status != CE_GOOD || !(writeFileEntry(fo,fHandle)))
            {
                return CE_ERASE_FAIL;
            }
            else
            {
                if (EraseClusters)
                {
                    return((fatEraseClusterChain(clus,volume)) ? CE_GOOD : CE_ERASE_FAIL);
                }
            }
        } // END: a not empty, not deleted entry was returned
    } // END: a not empty entry was returned
    return status;
}

uint8_t fopen(FILEOBJ fo, uint32_t* fHandle, char type)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fopen <<<<<!");
  #endif
    PARTITION* volume = (PARTITION*)(fo->volume);
    uint8_t error = CE_GOOD;

    if (volume->mount == false)
    {
        textColor(0x0C); printf("\nError: CE_NOT_INIT!"); textColor(0x0F); /// MESSAGE
        return CE_NOT_INIT;
    }
    else
    {
        // printf("\nline: %u",__LINE__); 
        cacheFileEntry(fo, fHandle, true);
        uint8_t r = fillFileObject(fo, fHandle);
        if (r != FOUND)
        {
            return CE_FILE_NOT_FOUND;
        }
        else // a file was found
        {
            fo->seek = 0;
            fo->ccls = fo->cluster;
            fo->sec  = 0;
            fo->pos  = 0;
            uint32_t l = cluster2sector(volume,fo->ccls); // determine LBA of the file's current cluster
            if (sectorRead(l,volume->buffer)!=SUCCESS)
            {
                error = CE_BAD_SECTOR_READ;
            }
            fo->Flags.FileWriteEOF = false;
            if (type == 'w' || type == 'a')
            {
                fo->Flags.write = true;
            }
            else
            {
                fo->Flags.write = 0;
            }
        } // END: a file was found
    } // END: the media is available
    return error;
}

uint8_t fclose(FILEOBJ fo)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fclose <<<<<!");
  #endif
    uint8_t error = CE_GOOD;
    uint32_t fHandle;
    DIRENTRY dir;

    fHandle = fo->entry;
    if (fo->Flags.write)
    {
        dir = loadDirectoryAttribute(fo, &fHandle);
        updateTimeStamp(dir);
        dir->DIR_FileSize = fo->size;
        if (writeFileEntry(fo,&fHandle))
        {
            error = CE_GOOD;
        }
        else
        {
            error = CE_WRITE_ERROR;
        }
        fo->Flags.write = false;
    }
    return error;
}

uint8_t fread(FILEOBJ fo, void* dest, uint32_t count)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fread <<<<<!");
  #endif
    PARTITION* volume;
    uint8_t  error = CE_GOOD;
    uint32_t pos;
    uint32_t l, seek, size, temp;

    volume  = (PARTITION*)fo->volume;
    temp = count;
    pos  = fo->pos;
    seek = fo->seek;
    size = fo->size;

    l = cluster2sector(volume,fo->ccls);
    l += (uint32_t)fo->sec;
    if (sectorRead(l,volume->buffer) != SUCCESS)
    {
        error = CE_BAD_SECTOR_READ;
    }
    while ((error == CE_GOOD) && (temp>0))
    {
        if (seek == size)
        {
            error = CE_EOF;
        }
        else
        {
            if (pos == SDC_SECTOR_SIZE)
            {
                pos = 0;
                fo->sec++;
                if (fo->sec == volume->SecPerClus)
                {
                    fo->sec = 0;
                    error = fileSearchNextCluster(fo,1);
                }
                if (error == CE_GOOD)
                {
                    l = cluster2sector(volume,fo->ccls);
                    l += (uint32_t)fo->sec;
                    if (sectorRead(l,volume->buffer) != SUCCESS)
                    {
                        error = CE_BAD_SECTOR_READ;
                    }
                }
            } // END: load new sector

            if (error == CE_GOOD)
            {
                *(uint8_t*)dest = RAMread(volume->buffer,pos++);
                dest += 1;
                seek++;
                (temp)--;
            }
        } // END: if not EOF
    } // while no error and more bytes to copy
    fo->pos  = pos;
    fo->seek = seek;
    return error;
}

/*
uint8_t fwrite(FILEOBJ fo, void* src, uint32_t count)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fwrite <<<<<!");
  #endif
    PARTITION*    volume;
    bool     sectorloaded = false;
    uint8_t  error = CE_GOOD;
    uint32_t pos, tempo;
    uint32_t l, seek, size;
    bool     IsWriteProtected = false;

    if (fo->Flags.write)
    {
        if (!IsWriteProtected)
        {
            tempo = count;
            volume   = fo->volume;
            pos   = fo->pos;
            seek  = fo->seek;

            l = cluster2sector(volume,fo->ccls);
            l += (uint32_t) fo->sec;
            if (sectorRead(l,volume->buffer) != SUCCESS)
            {
                error = CE_BAD_SECTOR_READ;
            }
            sectorloaded = true;
            size = fo->size;
            while ((error == CE_GOOD) && (tempo>0))
            {
                if (seek == size)
                {
                    fo->Flags.FileWriteEOF = true;
                }
                if (pos == SDC_SECTOR_SIZE)
                {
                    if (sectorloaded)
                    {
                        if (sectorWrite(l,volume->buffer)!=SUCCESS)
                        {
                            error = CE_WRITE_ERROR;
                        }
                    }
                    pos = 0;
                    fo->sec++;

                    if (fo->sec == volume->SecPerClus)
                    {
                        fo->sec = 0;
                        if (fo->Flags.FileWriteEOF)
                        {
                            error = fileAllocateNewCluster(fo);
                        }
                        else
                        {
                            error = fileSearchNextCluster(fo,1);
                        }
                    }

                    if (error == CE_DISK_FULL)
                    {
                        return error;
                    }

                    if (error == CE_GOOD)
                    {
                        l = cluster2sector(volume,fo->ccls);
                        l += (uint32_t) fo->sec;

                        if (sectorRead(l,volume->buffer) != SUCCESS)
                        {
                            error = CE_BAD_SECTOR_READ;
                        }
                        sectorloaded = true;
                    }
                } // END: write a sector and read the next sector

                if (error == CE_GOOD)
                {
                    RAMwrite(volume->buffer, pos++, *(uint8_t*)src);
                    src += 1;
                    seek++;
                    tempo--;
                }
                if (fo->Flags.FileWriteEOF)
                {
                    size++;
                }
            } // END: write to the file (except for the last sector)
            if (error == CE_GOOD)
            {
                l = cluster2sector(volume,fo->ccls);
                l += (uint32_t)fo->sec;
                if (sectorWrite(l,volume->buffer) != SUCCESS)
                {
                    error = CE_WRITE_ERROR;
                }
            }
            fo->pos  = pos;
            fo->seek = seek;
            fo->size = size;
        }
        else
        {
            return CE_WRITE_PROTECTED;
        }
    }
    else
    {
        return CE_WRITE_ERROR;
    }
    return error;
}
*/

void showDirectoryEntry(DIRENTRY dir)
{
    printf("\nname.ext: %s.%s",     dir->DIR_Name,dir->DIR_Extension                );
    printf("\nattrib.:  %y",        dir->DIR_Attr                                   );
    printf("\ncluster:  %u",        dir->DIR_FstClusLO + 0x10000*dir->DIR_FstClusHI );
    printf("\nfilesize: %u byte",   dir->DIR_FileSize                               );
}


