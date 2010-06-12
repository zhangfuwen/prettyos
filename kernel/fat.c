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
*  We are adapting this sourcecode to the needs of PrettyOS.
*/

#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "usb2_msd.h"
#include "console.h"
#include "fat.h"

// #define WRITE_IS_APPROVED <--- does not yet work!

uint8_t  globalBufferFATSector[512];                  // The global FAT sector buffer
bool     globalBufferMemSet0      = false;      // Global variable indicating that the data buffer contains all zeros
FILEPTR  globalBufferUsedByFILEPTR        = NULL;       // Global variable indicating which file is using the data buffer
uint32_t globalLastDataSectorRead = 0xFFFFFFFF; // Global variable indicating which data sector was read last
uint32_t globalLastFATSectorRead  = 0xFFFFFFFF; // Global variable indicating which FAT sector was read last
bool     globalFATWriteNecessary       = false;      // Global variable indicating that there is information that needs to be written to the FAT
bool     globalDataWriteNecessary      = false;      // Global variable indicating that there is data in the buffer that hasn't been written to the device.

// prototypes
// static uint32_t fatWrite(partition_t* volume, uint32_t cls, uint32_t v);

static uint32_t cluster2sector(partition_t* volume, uint32_t cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cluster2sector <<<<<!");
  #endif

    uint32_t sector = 0;

    switch (volume->type)
    {
        case FAT32: // In FAT32, there is no separate ROOT region. It is as well stored in DATA region 
            if (cluster <= 1) 
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

FS_ERROR sectorWrite(uint32_t sector_addr, uint8_t* buffer) // to implement
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorWrite <<<<<!");
  #endif
    textColor(0x0A); printf("\n>>>>> sectorWrite not yet implemented <<<<<!"); textColor(0x0F);
    return SUCCESS;
}

#ifdef WRITE_IS_APPROVED
static FS_ERROR flushData()
{
    partition_t* volume;
    FILEPTR fileptr = globalBufferUsedByFILEPTR;
    uint32_t sector;
    volume = fileptr->volume;
    sector = cluster2sector(volume,fileptr->ccls);
    sector += (uint16_t)fileptr->sec;
    if(sectorWrite( sector, volume->buffer, false) != SUCCESS)
    { 
        return CE_WRITE_ERROR; 
    }
    else
    {
        globalDataWriteNecessary = false;
        return CE_GOOD;
    }
}
#endif

FS_ERROR sectorRead(uint32_t sector_addr, uint8_t* buffer) // make it better!
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorRead <<<<<!");
  #endif
    FS_ERROR retVal = SUCCESS; // TEST
    usbRead(sector_addr, buffer); // until now only realized for USB 2.0 Mass Storage Device
    return retVal;
}

static uint32_t fatRead (partition_t* volume, uint32_t ccls)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatRead <<<<<!");
  #endif

    uint8_t  q;
    uint32_t posFAT; // position (byte) in FAT
    uint32_t c = 0;
    uint32_t d;
    uint32_t sector;
    uint32_t ClusterFailValue;
    uint32_t LastClusterLimit;
    globalBufferMemSet0= false;

    switch (volume->type)
    {
        case FAT32:
            posFAT = ccls * 4;
            q = 0; 
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            LastClusterLimit = LAST_CLUSTER_FAT32;
            break;
        case FAT12:
            posFAT = (uint32_t) ccls * 3;
            q = posFAT & 1;
            posFAT >>= 1;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT12;
            break;
        case FAT16:
        default:
            posFAT = (uint32_t)ccls *2;
            q = 0; 
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT16;
            break;
    }

    sector = volume->fat + (posFAT / volume->sectorSize);
    posFAT &= volume->sectorSize - 1;

    if (globalLastFATSectorRead == sector)
    {
        switch(volume->type)
        {
            case FAT32:
                c = MemoryReadLong (globalBufferFATSector, posFAT);
                break;
            case FAT16:
                c = MemoryReadWord    (globalBufferFATSector, posFAT);
                break;
            case FAT12:
                c = MemoryReadByte (globalBufferFATSector, posFAT);
                if (q)
                {
                    c >>= 4;
                }
                posFAT = (posFAT +1) & (volume->sectorSize-1);
                if (posFAT == 0)
                {
                  #ifdef WRITE_IS_APPROVED 
                    if (globalFATWriteNecessary)
                    {
                        if(fatWrite(volume, 0, 0, true))
                        {
                            return ClusterFailValue;
                        }
                    }
                  #endif
                    if (sectorRead (sector+1, globalBufferFATSector) != SUCCESS)
                    {
                        globalLastFATSectorRead = 0xFFFF;
                        return ClusterFailValue;
                    }
                    else
                    {
                        globalLastFATSectorRead = sector+1;
                    }
                }
                d = MemoryReadByte (globalBufferFATSector, posFAT);
                if (q)
                {
                    c += (d <<4);
                }
                else
                {
                    c += ((d & 0x0F)<<8);
                }
                break;
        }
    }
    else
    {
#ifdef WRITE_IS_APPROVED
        if (globalFATWriteNecessary)
        {
            if(WriteFAT (volume, 0, 0, TRUE))
                return ClusterFailValue;
        }
#endif
        if ( sectorRead (sector, globalBufferFATSector) != SUCCESS)
        {
            globalLastFATSectorRead = 0xFFFF;
            return ClusterFailValue;
        }
        else
        {
            globalLastFATSectorRead = sector;
            switch(volume->type)
            {
                case FAT32:
                    c = MemoryReadLong (globalBufferFATSector, posFAT);
                    break;
                case FAT16:
                    c = MemoryReadWord (globalBufferFATSector, posFAT);
                    break;
                case FAT12:
                    c = MemoryReadByte (globalBufferFATSector, posFAT);
                    if (q) { c >>= 4; }
                    posFAT = (posFAT +1) & (volume->sectorSize-1);
                    d = MemoryReadByte (globalBufferFATSector, posFAT);
                    if (q) { c += (d <<4); }
                    else   { c += ((d & 0x0F)<<8); }
                    break;
            }
        }
    }
    if (c >= LastClusterLimit){c = LastClusterLimit;}
    return c;
}

static FS_ERROR fileSearchNextCluster(FILEPTR fileptr, uint32_t n)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileSearchNextCluster <<<<<!");
  #endif
    partition_t* volume = fileptr->volume;

    do
    {
        uint32_t c2 = fileptr->ccls;
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
        fileptr->ccls = c;
    }
    while (--n > 0);

    return CE_GOOD;
}


///////////////
// directory //
///////////////

static FILEROOTDIRECTORYENTRY cacheFileEntry(FILEPTR fileptr, uint32_t* curEntry, bool ForceRead) 
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %u ForceRead: %u", *curEntry, ForceRead);
  #endif
    partition_t* volume     = fileptr->volume;
    uint32_t   LastClusterLimit, numofclus = 0;
    uint32_t   ccls       = fileptr->dirccls;
    uint32_t   cluster    = fileptr->dirclus;
    uint32_t   DirectoriesPerSector = volume->sectorSize/NUMBER_OF_BYTES_IN_DIR_ENTRY;
    uint32_t   offset2 = (*curEntry)/DirectoriesPerSector;

    switch (volume->type)
    {
        case FAT32:
            offset2  = offset2 % (volume->SecPerClus);
            LastClusterLimit = LAST_CLUSTER_FAT32;
            break;

        case FAT12:
        case FAT16:
        default:
            if(cluster != 0)
            {
                offset2  = offset2 % (volume->SecPerClus);
            }
            LastClusterLimit = LAST_CLUSTER_FAT16;
            break;
    }

    if ( ForceRead || ((*curEntry & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0) ) 
    {
        // do we need to load the next cluster?
        if(((offset2 == 0) && ((*curEntry) >= DirectoriesPerSector)) || ForceRead)                
        {
            if (cluster == 0 )
            {
                ccls = 0;
            }
            else
            {
                if (ForceRead) 
                {
                    numofclus = (*curEntry)/(DirectoriesPerSector * volume->SecPerClus);            
                }
                else 
                {
                    numofclus = 1;
                }

                while (numofclus) 
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
        
        if(ccls < LastClusterLimit) 
        {
            fileptr->dirccls = ccls; 
            uint32_t sector = cluster2sector(volume,ccls);

            if( (ccls == volume->FatRootDirCluster) && (((sector + offset2) >= volume->data)) && ((volume->type != FAT32)) )
            {
                return ((FILEROOTDIRECTORYENTRY)NULL);
            }
            else
            {
              #ifdef WRITE_IS_APPROVED
                if (globalDataWriteNecessary) {if (flushData()) {return NULL;}}
              #endif
                globalBufferUsedByFILEPTR  = NULL;
                globalBufferMemSet0= false;

                uint8_t retVal = sectorRead( sector + offset2, volume->buffer );

                if ( retVal != SUCCESS )
                {
                    return ((FILEROOTDIRECTORYENTRY)NULL);
                }
                else 
                {
                    if(ForceRead)
                    {
                        return (FILEROOTDIRECTORYENTRY)((FILEROOTDIRECTORYENTRY)volume->buffer) + ((*curEntry)%DirectoriesPerSector);    
                    }
                    else
                    {
                        return (FILEROOTDIRECTORYENTRY)volume->buffer;
                    }
                    globalLastDataSectorRead = 0xFFFFFFFF;
                }
            } // END: read an entry
        } // END: a valid cluster is found
        else // invalid cluster number
        {            
            return((FILEROOTDIRECTORYENTRY)NULL);
        }
    }
    else
    {
        return (FILEROOTDIRECTORYENTRY)(((FILEROOTDIRECTORYENTRY)volume->buffer) + ((*curEntry)%DirectoriesPerSector));   
    }
    textColor(0x0C); printf("UNCLEAN !!! END of cacheFileEntry"); textColor(0x0F); 
} 

static FILEROOTDIRECTORYENTRY getFileAttribute(FILEPTR fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> getFileAttribute <<<<<!");
  #endif
    FILEROOTDIRECTORYENTRY dir = cacheFileEntry(fileptr,fHandle,true);
    uint8_t  a   = dir->DIR_Name[0];
    if (a == DIR_EMPTY)
    {
        dir = ((FILEROOTDIRECTORYENTRY)NULL);
    }
    if (dir != (FILEROOTDIRECTORYENTRY)NULL)
    {
        if (a == DIR_DEL)
        {
            dir = ((FILEROOTDIRECTORYENTRY)NULL);
        }
        else
        {
            a = dir->DIR_Attr;
            while(a==ATTR_LONG_NAME)
            {
                (*fHandle)++;
                dir = cacheFileEntry(fileptr,fHandle,false);
                a = dir->DIR_Attr;
            }
        }
    }
    return dir;
}

static void updateTimeStamp(FILEROOTDIRECTORYENTRY dir)
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
 This function will cache the sector of directory entries in the directory 
 pointed to by the dirclus value in the FILEPTR 'fileptr' 
 that contains the entry that corresponds to the fHandle offset.  
 It will then copy the file information for that entry into the 'fo' FILE object.
*/

static uint8_t fillFILEPTR(FILEPTR fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fillFILEPTR <<<<<!");
  #endif

    FILEROOTDIRECTORYENTRY dir;

    if (((*fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0) && (*fHandle != 0)) // 4-bit mask because 16 root entries max per sector
    {
        fileptr->dirccls = fileptr->dirclus;
        dir = cacheFileEntry(fileptr, fHandle, true);
        showDirectoryEntry(dir);
    }
    else { dir = cacheFileEntry (fileptr, fHandle, false); }

    if ((dir == NULL) ) { return NO_MORE; }
    else
    {
        uint8_t a = dir->DIR_Name[0];
        if      ( a == DIR_DEL)  { return NOT_FOUND; }
        else if ( a == DIR_EMPTY){ return NO_MORE;   }
        else
        {
            uint8_t character, test=0;
            for (uint8_t i=0; i<DIR_NAMESIZE; i++)
            {
                character = dir->DIR_Name[i];
                fileptr->name[test++] = character;
            }
            character = dir->DIR_Extension[0];
            if(character != ' ')
            {
                for (uint8_t i=0; i<DIR_EXTENSION; i++)
                {
                    character = dir->DIR_Extension[i];
                    fileptr->name[test++]=character;
                }
            }
            fileptr->entry   = *fHandle;
            fileptr->size    = ( dir->DIR_FileSize);
            fileptr->cluster = ((dir->DIR_FstClusHI)<<16) | dir->DIR_FstClusLO;
            fileptr->time    = ( dir->DIR_WrtTime);
            fileptr->date    = ( dir->DIR_WrtDate);
            a = dir->DIR_Attr;
            fileptr->attributes = a;
            return FOUND;
        } // END: the entry is not deleted
    } // END: an entry exists
}

FS_ERROR searchFile( FILEPTR fileptrDest, FILEPTR fileptrTest, uint8_t cmd, uint8_t mode) 
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> searchFile <<<<<!");
  #endif

    char character, test;
    FS_ERROR statusB       = CE_FILE_NOT_FOUND;
    fileptrDest->dirccls   = fileptrDest->dirclus;
    uint16_t compareAttrib = 0xFFFF ^ fileptrTest->attributes;
    uint32_t fHandle = fileptrDest->entry;
    bool read = true;

    for (uint8_t i=0; i<DIR_NAMECOMP; i++){fileptrDest->name[i] = 0x20;}
    textColor(0x0E); printf("\nfHandle (searchFile): %d",fHandle); textColor(0x0F);  
    if (fHandle == 0)
    {
        if (cacheFileEntry(fileptrDest, &fHandle, read) == NULL){ statusB = CE_BADCACHEREAD; }
    }
    else
    {
        if ((fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) != 0) // Maximum 16 entries possible
        {
            if (cacheFileEntry (fileptrDest, &fHandle, read) == NULL){ statusB = CE_BADCACHEREAD; }
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
                state = fillFILEPTR(fileptrDest, &fHandle);
                if (state == NO_MORE) { break; }
            }
            else { break; }

            if (state == FOUND)
            {
              #ifdef _FAT_DIAGNOSIS_
                textColor(0x0A);printf("\n\nstate == FOUND");textColor(0x0F);
              #endif
                attrib =  fileptrDest->attributes;
                attrib &= ATTR_MASK;

                /*
                mode 0 : attributes are irrelevant.
                mode 1 : attributes of the fileptrDest entry must match the attributes of fileptrTest
                Partial string search characters may bypass portions of the comparison.
                */
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
                                character = fileptrDest->name[i];
                              #ifdef _FAT_DIAGNOSIS_
                                printf("\ni: %u character: %c test: %c", i, character, fileptrTest->name[i]); textColor(0x0F); /// TEST
                              #endif
                                if (toLower(character) != toLower(fileptrTest->name[i]))
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
                            if (fileptrTest->name[0] != '*')   // If "*" is passed for comparion as 1st char then don't proceed. Go back, file alreay found.
                            {
                                for (uint16_t index = 0; index < DIR_NAMESIZE; index++)
                                {
                                    character = fileptrDest->name[index]; 
                                    test = fileptrTest->name[index]; 
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

                            // Before calling this "searchFile" fn, "formatfilename" must be called. Hence, extn always starts from position "8".
                            if ((fileptrTest->name[8] != '*') && (statusB == CE_GOOD))
                            {
                                for (uint16_t index = 8; index < DIR_NAMECOMP; index++)
                                {
                                    
                                    character = fileptrDest->name[index]; // Get the source character
                                    test = fileptrTest->name[index]; // Get the destination character
                                    if (test == '*')
                                    {
                                        break;
                                    }
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
        }// while
    }
    return(statusB);
}

FS_ERROR fopen(FILEPTR fileptr, uint32_t* fHandle, char type)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fopen <<<<<!");
  #endif
    partition_t* volume = (partition_t*)(fileptr->volume);
    FS_ERROR error = CE_GOOD;

    if (volume->mount == false)
    {
        textColor(0x0C); printf("\nError: CE_NOT_INIT!"); textColor(0x0F); /// MESSAGE
        return CE_NOT_INIT;
    }
    else
    {
        cacheFileEntry(fileptr, fHandle, true);
        uint8_t r = fillFILEPTR(fileptr, fHandle);
        if (r != FOUND)
        {
            return CE_FILE_NOT_FOUND;
        }
        else // a file was found
        {
            fileptr->seek = 0;
            fileptr->ccls = fileptr->cluster;
            fileptr->sec  = 0;
            fileptr->pos  = 0;
            uint32_t l = cluster2sector(volume,fileptr->ccls); // determine LBA of the file's current cluster
            if (sectorRead(l,volume->buffer)!=SUCCESS)
            {
                error = CE_BAD_SECTOR_READ;
            }
            fileptr->Flags.FileWriteEOF = false;
            if (type == 'w' || type == 'a')
            {
                fileptr->Flags.write = true;
            }
            else
            {
                fileptr->Flags.write = 0;
            }
        } // END: a file was found
    } // END: the media is available
    return error;
}

FS_ERROR fclose(FILEPTR fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fclose <<<<<!");
  #endif
    FS_ERROR error = CE_GOOD;
    uint32_t fHandle;
    FILEROOTDIRECTORYENTRY dir;

    fHandle = fileptr->entry;
    if (fileptr->Flags.write)
    {
        dir = getFileAttribute(fileptr, &fHandle);
        updateTimeStamp(dir);
        dir->DIR_FileSize = fileptr->size;
        
        /*
        if (writeFileEntry(fileptr,&fHandle))
        {
            error = CE_GOOD;
        }
        else
        {
            error = CE_WRITE_ERROR;
        }
        */

        fileptr->Flags.write = false;
    }
    return error;
}

FS_ERROR fread(FILEPTR fileptr, void* dest, uint32_t count)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fread <<<<<!");
  #endif
    partition_t* volume;
    FS_ERROR error = CE_GOOD;
    uint32_t pos;
    uint32_t sector, seek, size, temp;

    volume  = fileptr->volume;
    temp    = count;
    pos     = fileptr->pos;
    seek    = fileptr->seek;
    size    = fileptr->size;

    sector = cluster2sector(volume,fileptr->ccls);
    sector += (uint32_t)fileptr->sec;
    if (sectorRead(sector, volume->buffer) != SUCCESS)
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
                fileptr->sec++;
                if (fileptr->sec == volume->SecPerClus)
                {
                    fileptr->sec = 0;
                    error = fileSearchNextCluster(fileptr,1);
                }
                if (error == CE_GOOD)
                {
                    sector = cluster2sector(volume,fileptr->ccls);
                    sector += (uint32_t)fileptr->sec;
                    if (sectorRead(sector, volume->buffer) != SUCCESS)
                    {
                        error = CE_BAD_SECTOR_READ;
                    }
                }
            } // END: load new sector

            if (error == CE_GOOD)
            {
                *(uint8_t*)dest = MemoryReadByte(volume->buffer, pos++);
                dest += 1;
                seek++;
                (temp)--;
            }
        } // END: if not EOF
    } // while no error and more bytes to copy
    fileptr->pos  = pos;
    fileptr->seek = seek;
    return error;
}

void showDirectoryEntry(FILEROOTDIRECTORYENTRY dir)
{
    char strName[260];
    char strExt[4];
    strncpy(strName,dir->DIR_Name,8);
    strName[8]='.'; // 0-7 short filename, 8: dot
    strName[9]=0;   // terminate for strcat
    strncpy(strExt,dir->DIR_Extension,3);
    strExt[3]=0; // 0-2 short filename extension, 3: '\0' (end of string)
    strcat(strName,strExt);
    
    printf("\nname.ext: %s",        strName                                         );
    printf("\nattrib.:  %y",        dir->DIR_Attr                                   );
    printf("\ncluster:  %u",        dir->DIR_FstClusLO + 0x10000*dir->DIR_FstClusHI );
    printf("\nfilesize: %u byte",   dir->DIR_FileSize                               );
}
