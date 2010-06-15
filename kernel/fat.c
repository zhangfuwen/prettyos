/*
*  This code is based on example code for the PIC18F4550 given in the book:
*  Jan Axelson, "USB Mass Storage Device" (2006), web site: www.Lvr.com
*
*  The copyright, page ii, tells:
*  "No part of the book, except the program code, may be reproduced or transmitted in any form by any means
*  without the written permission of the publisher. The program code may be stored and executed in a computer system
*  and may be incorporated into computer programs developed by the reader."
*
*  We are adapting this sourcecode to the needs of PrettyOS.
*/

#include "util.h"
#include "paging.h"
#include "kheap.h"
#include "usb2_msd.h"
#include "console.h"
#include "fat.h"
#include "devicemanager.h"

// prototypes
static uint32_t fatWrite (partition_t* volume, uint32_t ccls, uint32_t value, bool forceWrite);

#define WRITE_IS_APPROVED

uint8_t  globalBufferFATSector[SECTOR_SIZE]; // The global FAT sector buffer
bool     globalBufferMemSet0       = false;      // Global variable indicating that the data buffer contains all zeros
FILEPTR  globalBufferUsedByFILEPTR = NULL;       // Global variable indicating which file is using the data buffer
uint32_t globalLastDataSectorRead  = 0xFFFFFFFF; // Global variable indicating which data sector was read last
uint32_t globalLastFATSectorRead   = 0xFFFFFFFF; // Global variable indicating which FAT sector was read last
bool     globalFATWriteNecessary   = false;      // Global variable indicating that there is information that needs to be written to the FAT
bool     globalDataWriteNecessary  = false;      // Global variable indicating that there is data in the buffer that hasn't been written to the device.
uint8_t  FSerrno;                                // Global error number?

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

FS_ERROR sectorWrite(uint32_t sector_addr, uint8_t* buffer, partition_t* part)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorWrite <<<<<!");
  #endif
    textColor(0x0A); printf("\n>>>>> sectorWrite not yet implemented <<<<<!"); textColor(0x0F);

    FS_ERROR retVal = part->disk->type->writeSector(sector_addr, buffer);
    return retVal;
}


static FS_ERROR flushData()
{
  #ifdef WRITE_IS_APPROVED
    partition_t* volume;
    FILEPTR fileptr = globalBufferUsedByFILEPTR;
    uint32_t sector;
    volume = fileptr->volume;
    sector = cluster2sector(volume,fileptr->ccls);
    sector += (uint16_t)fileptr->sec;
    if(sectorWrite( sector, volume->buffer, false) != CE_GOOD)
    {
        return CE_WRITE_ERROR;
    }
    else
    {
        globalDataWriteNecessary = false;
        return CE_GOOD;
    }
  #endif
}


FS_ERROR sectorRead(uint32_t sector_addr, uint8_t* buffer, partition_t* part)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorRead <<<<<!");
  #endif
    return part->disk->type->readSector(sector_addr, buffer);
}
FS_ERROR singleSectorRead(uint32_t sector_addr, uint8_t* buffer, partition_t* part)
{
    part->disk->accessRemaining++;
    return sectorRead(sector_addr, buffer, part);
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
                c = MemoryReadLong(globalBufferFATSector, posFAT);
                break;
            case FAT16:
                c = MemoryReadWord(globalBufferFATSector, posFAT);
                break;
            case FAT12:
                c = MemoryReadByte(globalBufferFATSector, posFAT);
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
                    if(singleSectorRead(sector+1, globalBufferFATSector, volume) != CE_GOOD)
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
            if(fatWrite (volume, 0, 0, true))
                return ClusterFailValue;
        }
#endif
        if (singleSectorRead(sector, globalBufferFATSector, volume) != CE_GOOD)
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

static FS_ERROR fileGetNextCluster(FILEPTR fileptr, uint32_t n)
{
    uint32_t         c, c2, ClusterFailValue, LastClustervalue;
    FS_ERROR         error  = CE_GOOD;
    partition_t*     volume = fileptr->volume;

    switch (volume->type)
    {
        case FAT32:
            LastClustervalue = LAST_CLUSTER_FAT32;
            ClusterFailValue  = CLUSTER_FAIL_FAT32;
            break;
        case FAT12:
            LastClustervalue = LAST_CLUSTER_FAT12;
            ClusterFailValue  = CLUSTER_FAIL_FAT16;
            break;
        case FAT16:
        default:
            LastClustervalue = LAST_CLUSTER_FAT16;
            ClusterFailValue  = CLUSTER_FAIL_FAT16;
            break;
    }

    do
    {
        c2 = fileptr->ccls;
        if ( (c = fatRead( volume, c2)) == ClusterFailValue)
            error = CE_BAD_SECTOR_READ;
        else
        {
            if ( c >= volume->maxcls)
            {
                error = CE_INVALID_CLUSTER;
            }

            if ( c >= LastClustervalue)
            {
                error = CE_FAT_EOF;
            }
        }
        fileptr->ccls = c;
    } while (--n > 0 && error == CE_GOOD);
    return(error);
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
                return NULL;
            }
            else
            {
              #ifdef WRITE_IS_APPROVED
                if (globalDataWriteNecessary) {if (flushData()) {return NULL;}}
              #endif
                globalBufferUsedByFILEPTR = NULL;
                globalBufferMemSet0 = false;

                uint8_t retVal = singleSectorRead(sector + offset2, volume->buffer, volume);

                if ( retVal != CE_GOOD )
                {
                    return NULL;
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
            return NULL;
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
        dir = NULL;
    }
    if (dir != NULL)
    {
        if (a == DIR_DEL)
        {
            dir = NULL;
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

    if (dir == NULL) { return NO_MORE; }
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
            if (cacheFileEntry(fileptrDest, &fHandle, read) == NULL) { statusB = CE_BADCACHEREAD; }
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
                                for (uint16_t i=0; i<DIR_NAMESIZE; i++)
                                {
                                    character = fileptrDest->name[i];
                                    test = fileptrTest->name[i];
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
                                for (uint16_t i=8; i<DIR_NAMECOMP; i++)
                                {

                                    character = fileptrDest->name[i]; // Get the source character
                                    test = fileptrTest->name[i]; // Get the destination character
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
    partition_t* volume = fileptr->volume;
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
            uint32_t sector = cluster2sector(volume,fileptr->ccls); // determine LBA of the file's current cluster
            if (singleSectorRead(sector, volume->buffer, volume) != CE_GOOD)
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

    uint32_t sectors = (size%512 == 0) ? size/512 : size/512+1;
    volume->disk->accessRemaining += sectors;

    sectors--;
    if (sectorRead(sector, volume->buffer, volume) != CE_GOOD)
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
            if (pos == SECTOR_SIZE)
            {
                pos = 0;
                fileptr->sec++;
                if (fileptr->sec == volume->SecPerClus)
                {
                    fileptr->sec = 0;
                    error = fileGetNextCluster(fileptr,1);
                }
                if (error == CE_GOOD)
                {
                    sector = cluster2sector(volume,fileptr->ccls);
                    sector += (uint32_t)fileptr->sec;
                    sectors--;
                    if (sectorRead(sector, volume->buffer, volume) != CE_GOOD)
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

    volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been read
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


//////////////////////////////
// Write Files to Partition //
//////////////////////////////

static uint32_t fatWrite (partition_t* volume, uint32_t ccls, uint32_t value, bool forceWrite)
{
  #ifdef WRITE_IS_APPROVED
    uint8_t q;
    uint8_t c;
    uint32_t posFAT; // position (byte) in FAT
    uint32_t sector;
    uint32_t ClusterFailValue;

    if ((volume->type != FAT32) && (volume->type != FAT16) && (volume->type != FAT12))
    {
        return CLUSTER_FAIL_FAT32;
    }

    if ((volume->type != FAT16) && (volume->type != FAT12))
    {
        return CLUSTER_FAIL_FAT16;
    }

    switch (volume->type)
    {
    case FAT32:
        ClusterFailValue = CLUSTER_FAIL_FAT32;
        break;
    case FAT12:
    case FAT16:
    default:
        ClusterFailValue = CLUSTER_FAIL_FAT16;
        break;
    }

    globalBufferMemSet0 = false;

    if (forceWrite) // write the current FAT sector to the partition "volume"
    {
        for (uint8_t i=0, li=globalLastFATSectorRead; i<volume->fatcopy; i++, li+=volume->fatsize)
        {
            if (sectorWrite(li, globalBufferFATSector, volume) != CE_GOOD)
            {
                return ClusterFailValue;
            }
        }
        globalFATWriteNecessary = false;
        return 0;
    }

    switch (volume->type)
    {
        case FAT32:
            posFAT = (uint32_t)ccls * 4;
            q = 0;
            break;
        case FAT12:
            posFAT = (uint32_t)ccls * 3;
            q      = posFAT  & 1; // odd/even
            posFAT = posFAT >> 1;
            break;
        case FAT16:
        default:
            posFAT = (uint32_t)ccls * 2;   // "p" is the position
            q = 0;
            break;
    }

    sector = volume->fat + posFAT/volume->sectorSize;
    posFAT &= volume->sectorSize - 1;

    if (globalLastFATSectorRead != sector)
    {
        // If we are loading a new sector then write the current one to the volume if we need to
        if (globalFATWriteNecessary)
        {
            for (uint8_t i=0, li=globalLastFATSectorRead; i<volume->fatcopy; i++, li+=volume->fatsize)
            {
                if (sectorWrite(li, globalBufferFATSector, volume) != CE_GOOD)
                {
                    return ClusterFailValue;
                }
            }
            globalFATWriteNecessary = false;
        }

        // Load the new sector
        if (sectorRead (sector, globalBufferFATSector, volume) != CE_GOOD)
        {
            globalLastFATSectorRead = 0xFFFF;
            return ClusterFailValue;
        }
        else
        {
            globalLastFATSectorRead = sector;
        }
    }

    switch (volume->type)
    {
    case FAT32:
        MemoryWriteByte (globalBufferFATSector, posFAT,   ((value & 0x000000FF)      ));
        MemoryWriteByte (globalBufferFATSector, posFAT+1, ((value & 0x0000FF00) >>  8));
        MemoryWriteByte (globalBufferFATSector, posFAT+2, ((value & 0x00FF0000) >> 16));
        MemoryWriteByte (globalBufferFATSector, posFAT+3, ((value & 0x0F000000) >> 24));
        break;
    case FAT16:
        MemoryWriteByte (globalBufferFATSector, posFAT,     value);
        MemoryWriteByte (globalBufferFATSector, posFAT+1, ((value & 0xFF00) >> 8));
        break;
    case FAT12:
        c = MemoryReadByte (globalBufferFATSector, posFAT);
        if (q) { c = ((value & 0x0F) << 4) | ( c & 0x0F); }
        else   { c = ( value & 0xFF); }

        MemoryWriteByte (globalBufferFATSector, posFAT, c); //

        // crossing sectors possible, do we need to load a new sector?
        posFAT = (posFAT+1) & (volume->sectorSize-1);
        if (posFAT == 0)
        {

            if (fatWrite (volume, 0, 0, true)) // update FAT
            {
                return ClusterFailValue;
            }

            if (sectorRead (sector+1, globalBufferFATSector, volume)) // next sector
            {
                globalLastFATSectorRead = 0xFFFF;
                return ClusterFailValue;
            }
            else
            {
                globalLastFATSectorRead = sector+1;
            }
        }
        c = MemoryReadByte(globalBufferFATSector, posFAT); // second byte of the table entry
        if (q) { c = (value >> 4); }
        else   { c = ((value >> 8) & 0x0F) | (c & 0xF0); }
        MemoryWriteByte (globalBufferFATSector, posFAT, c);
        break;
    }
    globalFATWriteNecessary = true;
    return 0;
  #endif
}


static uint32_t fatFindEmptyCluster(FILEPTR fileptr)
{
  #ifdef WRITE_IS_APPROVED
    uint32_t    value = 0x0;
    uint32_t    c,curcls, EndClusterLimit, ClusterFailValue;
    partition_t* volume = fileptr->volume;
    c = fileptr->ccls;

    switch (volume->type)
    {
        case FAT32:
            EndClusterLimit = END_CLUSTER_FAT32;
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            break;
        case FAT12:
            EndClusterLimit = END_CLUSTER_FAT12;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            break;
        case FAT16:
        default:
            EndClusterLimit = END_CLUSTER_FAT16;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            break;
    }

    if(c < 2) 
        c = 2;

    curcls = c;
    fatRead(volume, c);

    while(c)
    {
        if ( (value = fatRead(volume, c)) == ClusterFailValue)
        {
            c = 0;
            break;
        }
    
        if (value == CLUSTER_EMPTY)
            break;

        c++;    
        
        if (value == EndClusterLimit || c >= (volume->maxcls+2))
            c = 2;
        
        if ( c == curcls)
        {
            c = 0;
            break;
        }
    }  

    return(c);
  #endif
}

static FS_ERROR eraseCluster(partition_t* volume, uint32_t cluster)
{
  #ifdef WRITE_IS_APPROVED
    uint32_t SectorAddress;
    FS_ERROR error = CE_GOOD;

    SectorAddress = cluster2sector(volume,cluster);
    if (globalDataWriteNecessary)
        if (flushData())
            return CE_WRITE_ERROR;

    globalBufferUsedByFILEPTR = NULL;

    if (globalBufferMemSet0 == false)
    {
        memset(volume->buffer, 0, SECTOR_SIZE);
        globalBufferMemSet0 = true;
    }

    for(uint16_t i=0; i<volume->SecPerClus && error == CE_GOOD; i++)
    {
        if (sectorWrite( SectorAddress++, volume->buffer, volume) != CE_GOOD)
        {
            error = CE_WRITE_ERROR;
        }
    }
    return(error);
  #endif
}

static FS_ERROR fileAllocateNewCluster( FILEPTR fileptr, uint8_t mode)
{
  #ifdef WRITE_IS_APPROVED

    partition_t* volume = fileptr->volume;
    uint32_t curcls;
    uint32_t c = fileptr->ccls;
    c = fatFindEmptyCluster(fileptr);
    if (c == 0) { return CE_DISK_FULL; }

    switch (volume->type)
    {
    case FAT12:
        fatWrite( volume, c, LAST_CLUSTER_FAT12, false);
        break;
    case FAT16:
        fatWrite( volume, c, LAST_CLUSTER_FAT16, false);
        break;
    case FAT32:
        fatWrite( volume, c, LAST_CLUSTER_FAT32, false);
        break;
    }

    curcls = fileptr->ccls;
    fatWrite( volume, curcls, c, false);
    fileptr->ccls = c;
    if (mode == 1) return (eraseCluster(volume, c));
    else return  CE_GOOD;
  #endif
}

// 'size' of one object, 'n' is the number of these objects
uint32_t fwrite(const void* ptr, uint32_t size, uint32_t n, FILEPTR fileptr)
{
  #ifdef WRITE_IS_APPROVED
    FS_ERROR      error = CE_GOOD;
    partition_t*  volume;
    uint32_t      count = size * n;
    uint8_t*      src = (uint8_t*) ptr;
    uint32_t      sector;
    uint16_t      pos;
    uint32_t      seek;
    uint32_t      filesize;
    uint16_t      writeCount = 0;

    if(!(fileptr->Flags.write))
    {
        FSerrno = CE_READONLY;
        error   = CE_WRITE_ERROR;
        return 0;
    }
    if (!count){return 0;}
    globalBufferMemSet0 = false;
    volume  = fileptr->volume;
    pos     = fileptr->pos;
    seek    = fileptr->seek;
    sector  = cluster2sector(volume,fileptr->ccls);
    sector += (uint16_t)fileptr->sec;

    if (globalBufferUsedByFILEPTR != fileptr)
    {
        if (globalDataWriteNecessary)
        {
            if (flushData())
            {
                FSerrno = CE_WRITE_ERROR;
                return 0;
            }
        }
        globalBufferUsedByFILEPTR = fileptr;
    }
    if (globalLastDataSectorRead != sector)
    {
        if (globalDataWriteNecessary)
        {
            if (flushData())
            {
                FSerrno = CE_WRITE_ERROR;
                return 0;
            }
        }

        globalBufferMemSet0 = false;
        if(sectorRead( sector, volume->buffer, volume) != CE_GOOD )
        {
            FSerrno = CE_BADCACHEREAD;
            error   = CE_BAD_SECTOR_READ;
        }
        globalLastDataSectorRead = sector;
    }

    filesize = fileptr->size;

    while ((error==CE_GOOD) && (count>0))
    {
        if (seek==filesize)
        {
            fileptr->Flags.FileWriteEOF = true;
        }

        if (pos == volume->sectorSize)
        {
            uint8_t needRead = true;

            if (globalDataWriteNecessary)
                if (flushData())
                {
                    FSerrno = CE_WRITE_ERROR;
                    return 0;
                }

            pos = 0;
            fileptr->sec++;
            if (fileptr->sec == volume->SecPerClus)
            {
                fileptr->sec = 0;

                if (fileptr->Flags.FileWriteEOF)
                {
                    error = fileAllocateNewCluster(fileptr, 0);
                    needRead = false;
                }
                else
                {
                    error = fileGetNextCluster( fileptr, 1);
                }
            }

            if (error == CE_DISK_FULL)
            {
                FSerrno = CE_DISK_FULL;
                return 0;
            }

            if(error == CE_GOOD)
            {
                sector = cluster2sector(volume,fileptr->ccls);
                sector += (uint16_t)fileptr->sec;
                globalBufferUsedByFILEPTR = fileptr;

                if (needRead)
                {
                    if (sectorRead( sector, volume->buffer, volume) != CE_GOOD)
                    {
                        FSerrno = CE_BADCACHEREAD;
                        error   = CE_BAD_SECTOR_READ;
                        globalLastDataSectorRead = 0xFFFFFFFF;
                        return 0;
                    }
                    else
                    {
                        globalLastDataSectorRead = sector;
                    }
                }
                else
                {
                    globalLastDataSectorRead = sector;
                }
            }
        } //  load new sector

        if(error == CE_GOOD)
        {
            // Write one byte at a time
            MemoryWriteByte(volume->buffer, pos++, *(uint8_t*)src);
            src = src + 1; // compiler bug ???
            seek++;
            count--;
            writeCount++;
            if(fileptr->Flags.FileWriteEOF)
            {
                filesize++;
            }
            globalDataWriteNecessary = true;
        }
    } // while count

    fileptr->pos = pos;       // save positon
    fileptr->seek = seek;     // save seek
    fileptr->size = filesize; // new filesize

    return(writeCount/size);
  #endif
}


/*******************************************************************************/

static uint32_t getFullClusterNumber(FILEROOTDIRECTORYENTRY entry)
{
    uint32_t TempFullClusterCalc = (entry->DIR_FstClusHI);
    TempFullClusterCalc = TempFullClusterCalc << 16;
    TempFullClusterCalc |= entry->DIR_FstClusLO;
    return TempFullClusterCalc;
}

static uint8_t writeFileEntry( FILEPTR fileptr, uint32_t* curEntry)
{
  #ifdef WRITE_IS_APPROVED
    uint8_t      status;
    uint8_t      offset2;
    uint32_t     sector;
    uint32_t     ccls;

    partition_t* volume = fileptr->volume;
    ccls                = fileptr->dirccls;
    offset2             = (*curEntry / (volume->sectorSize/32));

    switch (volume->type)
    {
    case FAT32:
            offset2 = offset2 % (volume->SecPerClus);
            break;
    case FAT12:
    case FAT16:
            if(ccls != 0)
                offset2 = offset2 % (volume->SecPerClus);
            break;
    }

    sector = cluster2sector(volume,ccls);

    if ( sectorWrite( sector + offset2, volume->buffer, volume) != CE_GOOD)
        status = false;
    else
        status = true;
    return(status);
  #endif
}


static bool fatEraseClusterChain (uint32_t cluster, partition_t* volume)
{
  #ifdef WRITE_IS_APPROVED
    uint32_t     c,c2,ClusterFailValue;
    enum _status {Good, Fail, Exit}status;
    status = Good;

    switch (volume->type)
    {
        case FAT32:
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            c2 =  LAST_CLUSTER_FAT32;
            break;
        case FAT12:
            ClusterFailValue = CLUSTER_FAIL_FAT16; // FAT16 value itself
            c2 =  LAST_CLUSTER_FAT12;
            break;
        case FAT16:
        default:
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            c2 =  LAST_CLUSTER_FAT16;
            break;
    }

    // Make sure there is actually a cluster assigned
    if(cluster == 0 || cluster == 1)  // Cluster assigned can't be "0" and "1"
    {
        status = Exit;
    }
    else
    {
        while(status == Good)
        {
            // Get the FAT entry
            if((c = fatRead( volume, cluster)) == ClusterFailValue)
                status = Fail;
            else
            {
                if(c == 0 || c == 1)  // Cluster assigned can't be "0" and "1"
                {
                    status = Exit;
                }
                else
                {
                    // compare against max value of a cluster in FATxx
                    // look for the last cluster in the chain
                    if ( c >= c2)
                        status = Exit;

                    // Now erase this FAT entry
                    if(fatWrite(volume, cluster, CLUSTER_EMPTY, false) == ClusterFailValue)
                        status = Fail;

                    // now update what the current cluster is
                    cluster = c;
                }
            }
        }// while status
    }// cluster == 0

    fatWrite(volume, 0, 0, true);

    if(status == Exit)
        return(true);
    else
        return(false);
 #endif
}


FS_ERROR fileErase( FILEPTR fileptr, uint32_t* fHandle, uint8_t EraseClusters)
{
  #ifdef WRITE_IS_APPROVED
    FILEROOTDIRECTORYENTRY dir;
    uint8_t      a;
    FS_ERROR     status = CE_GOOD;
    uint32_t     clus;
    partition_t* volume = fileptr->volume;

    clus = fileptr->dirclus;
    fileptr->dirccls = clus;

    dir = cacheFileEntry(fileptr, fHandle, true);

    if (dir == NULL)
    {
        FSerrno = CE_ERASE_FAIL;
        return CE_BADCACHEREAD;
    }

    a = dir->DIR_Name[0];

    if(dir == NULL || a == DIR_EMPTY)
    {
        status = CE_FILE_NOT_FOUND;
    }
    else
    {
        if ( a == DIR_DEL)
        {
            status = CE_FILE_NOT_FOUND;
        }
        else
        {
            a = dir->DIR_Attr;
            dir->DIR_Name[0] = DIR_DEL;
            clus = getFullClusterNumber(dir);

            if ( (status != CE_GOOD) || ((writeFileEntry( fileptr, fHandle)) != CE_GOOD))
            {
                status = CE_ERASE_FAIL;
            }
            else
            {
                if (clus != 0) // FatRootDirClusterValue = 0 ??? <<<------------------- CHECK
                {
                    if(EraseClusters)
                    {
                        status = ((fatEraseClusterChain(clus, volume)) ? CE_GOOD : CE_ERASE_FAIL);
                    }
                }
            }
        }
    }

    if (status == CE_GOOD)  FSerrno = CE_GOOD;
    else                    FSerrno = CE_ERASE_FAIL;

    return (status);
  #endif
}

static FS_ERROR PopulateEntries(FILEPTR fileptr, char *name , uint32_t *fHandle, uint8_t mode)
{
  #ifdef WRITE_IS_APPROVED
    FS_ERROR error = CE_GOOD;
    FILEROOTDIRECTORYENTRY dir;

    fileptr->dirccls = fileptr->dirclus;
    dir = cacheFileEntry( fileptr, fHandle, true);

    if (dir == NULL) return CE_BADCACHEREAD;

    strncpy(dir->DIR_Name,name,DIR_NAMECOMP);
    if (mode == DIRECTORY)
    {
        dir->DIR_Attr = ATTR_DIRECTORY;
    }
    else
    {
        dir->DIR_Attr = ATTR_ARCHIVE;
    }

    dir->DIR_FileSize  =    0x0;         // file size in uint32_t
    dir->DIR_NTRes     =    0x00;        // nt reserved
    dir->DIR_FstClusHI =    0x0000;      // hiword of this entry's first cluster number
    dir->DIR_FstClusLO =    0x0000;      // loword of this entry's first cluster number
    
   // time info as example
    dir->DIR_CrtTimeTenth = 0xB2;        // millisecond stamp
    dir->DIR_CrtTime =      0x7278;      // time created
    dir->DIR_CrtDate =      0x32B0;      // date created
    dir->DIR_LstAccDate =   0x32B0;      // Last access date
    dir->DIR_WrtTime =      0x7279;      // last update time
    dir->DIR_WrtDate =      0x32B0;      // last update date

    fileptr->size        = dir->DIR_FileSize;
    fileptr->time        = dir->DIR_CrtTime;
    fileptr->date        = dir->DIR_CrtDate;
    fileptr->attributes  = dir->DIR_Attr;
    fileptr->entry       = *fHandle;

    if (writeFileEntry(fileptr,fHandle) != true)
    {
        error = CE_WRITE_ERROR;
    }
    return(error);
  #endif
}

uint8_t FindEmptyEntries(FILEPTR fileptr, uint32_t *fHandle)
{
  #ifdef WRITE_IS_APPROVED
    uint8_t   status = NOT_FOUND;
    uint8_t   amountfound;
    char a = ' ';
    uint16_t  bHandle;
    uint32_t  b;
    FILEROOTDIRECTORYENTRY  dir;

    fileptr->dirccls = fileptr->dirclus;
    if((dir = cacheFileEntry( fileptr, fHandle, true)) == NULL)
    {
        status = CE_BADCACHEREAD;
    }
    else
    {
        while (status == NOT_FOUND)
        {
            amountfound = 0;
            bHandle = *fHandle;

            do
            {
                dir = cacheFileEntry( fileptr, fHandle, false);
                if(dir != NULL)
                {
                    a = dir->DIR_Name[0];
                }
                (*fHandle)++;
            }
            while((a == DIR_DEL || a == DIR_EMPTY) && (dir != NULL) &&  (++amountfound < 1));

            if(dir == NULL)
            {
                b = fileptr->dirccls;
                if(b == 0) // FatRootDirClusterValue = 0 ??? <<<------------------- CHECK
                {
                    if (fileptr->volume->type != FAT32)
                        status = NO_MORE;
                    else
                    {
                        fileptr->ccls = b;

                        if(fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
                            status = NO_MORE;
                        else
                        {
                            *fHandle = bHandle;
                            status = FOUND;
                        }
                    }
                }
                else
                {
                    fileptr->ccls = b;
                    if(fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
                    {
                        status = NO_MORE;
                    }
                    else
                    {
                        *fHandle = bHandle;
                        status = FOUND;     
                    }
                }
            }
            else
            {
                if(amountfound == 1)
                {
                    status = FOUND;
                    *fHandle = bHandle;
                }
            }
        }
        *fHandle = bHandle;
    }
    if(status == FOUND) return(true);
    else return(false);
  #endif
}

static FILEROOTDIRECTORYENTRY loadDirAttrib(FILEPTR fo, uint32_t* fHandle)
{
    FILEROOTDIRECTORYENTRY dir;
    uint8_t a;

    fo->dirccls = fo->dirclus;
    // Get the entry
    dir = cacheFileEntry( fo, fHandle, true);
    if (dir == NULL)
        return NULL;

    a = dir->DIR_Name[0];
    if(a == DIR_EMPTY) dir = NULL;

    if(dir != NULL)
    {
        if ( a == DIR_DEL)
            dir = NULL;
        else
        {
            a = dir->DIR_Attr;
            while(a == ATTR_LONG_NAME)
            {
                (*fHandle)++;
                dir = cacheFileEntry( fo, fHandle, false);
                if (dir == NULL) return NULL;
                a = dir->DIR_Attr;
            } 
        } 
    }
    return(dir);
}


static FS_ERROR fileCreateHeadCluster( FILEPTR fo, uint32_t* cluster)
{
  #ifdef WRITE_IS_APPROVED
    FS_ERROR error = CE_GOOD;
    partition_t* volume = fo->volume;
    *cluster = fatFindEmptyCluster(fo);

    if(*cluster == 0)
    {
        error = CE_DISK_FULL;
    }
    else
    {
        if(volume->type == FAT12)
        {
            if(fatWrite( volume, *cluster, LAST_CLUSTER_FAT12, false) == CLUSTER_FAIL_FAT16)
            {
                error = CE_WRITE_ERROR;
            }
        }
        else if(volume->type == FAT16)
        {
            if(fatWrite( volume, *cluster, LAST_CLUSTER_FAT16, false) == CLUSTER_FAIL_FAT16)
            {
                error = CE_WRITE_ERROR;
            }
        }
        else
        {
            if(fatWrite( volume, *cluster, LAST_CLUSTER_FAT32, false) == CLUSTER_FAIL_FAT32)
            {
                error = CE_WRITE_ERROR;
            }
        }

        if(error == CE_GOOD)
        {
            error = eraseCluster(volume,*cluster);
        }
    }

    return(error);
  #endif
}

static FS_ERROR CreateFirstCluster(FILEPTR fileptr)
{
    #ifdef WRITE_IS_APPROVED
    FS_ERROR   error;
    uint32_t   cluster, TempMsbCluster;
    uint32_t   fHandle = fileptr->entry;
    FILEROOTDIRECTORYENTRY   dir;

    if((error = fileCreateHeadCluster(fileptr,&cluster)) == CE_GOOD)
    {
        dir = loadDirAttrib(fileptr, &fHandle);
        dir->DIR_FstClusLO = (cluster & 0x0000FFFF);

        TempMsbCluster = (cluster & 0x0FFF0000); // only 28 bits in FAT32
        TempMsbCluster = TempMsbCluster >> 16;
        dir->DIR_FstClusHI = TempMsbCluster;

        if(writeFileEntry(fileptr, &fHandle) != true)
        {
            error = CE_WRITE_ERROR;
        }
    }
    return(error);
  #endif
}

FS_ERROR createFileEntry(FILEPTR fileptr, uint32_t *fHandle, uint8_t mode)
{
  #ifdef WRITE_IS_APPROVED
    FS_ERROR error = CE_GOOD;
    FSerrno = CE_GOOD;
    char name[11];
    *fHandle = 0;

    for (uint8_t i=0; i<FILE_NAME_SIZE; i++)
    {
        name[i] = fileptr->name[i];
    }

    if(FindEmptyEntries(fileptr, fHandle))
    {
        if((error = PopulateEntries(fileptr, name ,fHandle, mode)) == CE_GOOD)
        {
            error = CreateFirstCluster(fileptr);
        }
    }
    else
    {
        error = CE_DIR_FULL;
    }

    FSerrno = error;
    return(error);
  #endif
}













