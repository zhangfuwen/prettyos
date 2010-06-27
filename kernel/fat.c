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

// fseek
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// prototypes
static uint32_t fatWrite(partition_t* volume, uint32_t ccls, uint32_t value, bool forceWrite);
static bool writeFileEntry(FILE* fileptr, uint32_t* curEntry);

// data buffer
FILE*    globalFilePtr = NULL;                  // Global variable indicating which file is using the partition's data buffer
bool     globalBufferMemSet0      = false;      // Global variable indicating that the data buffer contains all zeros
uint32_t globalLastDataSectorRead = 0xFFFFFFFF; // Global variable indicating which data sector was read last
bool     globalDataWriteNecessary = false;      // Global variable indicating that there is data in the buffer that hasn't been written to the device.

// FAT
uint8_t  globalBufferFATSector[SECTOR_SIZE];    // Global FAT sector buffer
uint32_t globalLastFATSectorRead  = 0xFFFFFFFF; // Global variable indicating which FAT sector was read last
bool     globalFATWriteNecessary  = false;      // Global variable indicating that there is information that needs to be written to the FAT

static uint32_t cluster2sector(partition_t* volume, uint32_t cluster)
{
    uint32_t sector;
    if (cluster <= 1) // root dir
    {
        if (volume->type == FAT32) // In FAT32, there is no separate ROOT region. It is as well stored in DATA region
        {
            sector = volume->data + cluster * volume->SecPerClus;
        }
        else
        {
            sector = volume->root + cluster * volume->SecPerClus;
        }
    }
    else // data area
    {
        sector = volume->data + (cluster-2) * volume->SecPerClus;
    }

  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cluster2sector<<<<<    cluster: %u  sector %u", cluster, sector);
  #endif
    return (sector);
}

FS_ERROR sectorWrite(uint32_t lba, uint8_t* buffer, partition_t* part)
{
  #ifdef _FAT_READ_WRITE_TO_SECTOR_DIAGNOSIS_
    textColor(0x0E); printf("\n>>>>> sectorWrite: %u <<<<<",lba); textColor(0x0F);
  #endif
    return part->disk->type->writeSector(lba, buffer, part->disk->data);
}
FS_ERROR singleSectorWrite(uint32_t lba, uint8_t* buffer, partition_t* part)
{
    part->disk->accessRemaining++;
    return sectorWrite(lba, buffer, part);
}

FS_ERROR sectorRead(uint32_t lba, uint8_t* buffer, partition_t* part)
{
  #ifdef _FAT_READ_WRITE_TO_SECTOR_DIAGNOSIS_
    textColor(0x03); printf("\n>>>>> sectorRead: %u <<<<<",lba); textColor(0x0F);
  #endif
    return part->disk->type->readSector(lba, buffer, part->disk->data);
}
FS_ERROR singleSectorRead(uint32_t lba, uint8_t* buffer, partition_t* part)
{
    part->disk->accessRemaining++;
    return sectorRead(lba, buffer, part);
}

static uint32_t fatRead (partition_t* volume, uint32_t ccls)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatRead <<<<<");
  #endif

    uint8_t  q;
    uint32_t posFAT; // position (byte) in FAT
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
            posFAT = ccls * 3;
            q = posFAT & 1;
            posFAT >>= 1;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT12;
            break;
        case FAT16:
        default:
            posFAT = ccls * 2;
            q = 0;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            LastClusterLimit = LAST_CLUSTER_FAT16;
            break;
    }

    uint32_t sector = volume->fat + (posFAT / volume->sectorSize);

    posFAT &= volume->sectorSize - 1;

    uint32_t c = 0;
    uint32_t d;

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
                    if (globalFATWriteNecessary)
                    {
                        if (fatWrite(volume, 0, 0, true))
                        {
                            return ClusterFailValue;
                        }
                    }
                    if (singleSectorRead(sector+1, globalBufferFATSector, volume) != CE_GOOD)
                    {
                        globalLastFATSectorRead = 0xFFFF;
                        return ClusterFailValue;
                    }
                    globalLastFATSectorRead = sector+1;
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
        if (globalFATWriteNecessary)
        {
            if (fatWrite (volume, 0, 0, true))
                return ClusterFailValue;
        }
        if (singleSectorRead(sector, globalBufferFATSector, volume) != CE_GOOD)
        {
            globalLastFATSectorRead = 0xFFFF;
            return ClusterFailValue;
        }

        globalLastFATSectorRead = sector;
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
                if (q) { c >>= 4; }
                posFAT = (posFAT +1) & (volume->sectorSize-1);
                d = MemoryReadByte (globalBufferFATSector, posFAT);
                if (q) { c += d <<4; }
                else   { c += (d & 0x0F)<<8; }
                break;
        }
    }
    if (c >= LastClusterLimit) {c = LastClusterLimit;}
    return c;
}

static FS_ERROR fileGetNextCluster(FILE* fileptr, uint32_t n)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileGetNextCluster <<<<<");
  #endif

    uint32_t         c, c2, ClusterFailValue, LastClustervalue;
    FS_ERROR         error  = CE_GOOD;
    partition_t*     volume = fileptr->volume;

    switch (volume->type)
    {
        case FAT32:
            LastClustervalue = LAST_CLUSTER_FAT32;
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            break;
        case FAT12:
            LastClustervalue = LAST_CLUSTER_FAT12;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            break;
        case FAT16:
        default:
            LastClustervalue = LAST_CLUSTER_FAT16;
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            break;
    }

    do
    {
        c2 = fileptr->ccls;
        if ( (c = fatRead( volume, c2)) == ClusterFailValue)
        {
            error = CE_BAD_SECTOR_READ;
        }
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
    } while ((--n>0) && (error == CE_GOOD));
    return error;
}


///////////////
// directory //
///////////////

static FILEROOTDIRECTORYENTRY cacheFileEntry(FILE* fileptr, uint32_t* curEntry, bool ForceRead)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %u ForceRead: %u", *curEntry, ForceRead);
  #endif
    partition_t* volume           = fileptr->volume;
    uint32_t cluster              = fileptr->dirclus;
    uint32_t DirectoriesPerSector = volume->sectorSize/NUMBER_OF_BYTES_IN_DIR_ENTRY;
    uint32_t offset2              = (*curEntry)/DirectoriesPerSector;
    uint32_t LastClusterLimit, numofclus = 0;

    if (volume->type == FAT32)
    {
        offset2  = offset2 % (volume->SecPerClus);
        LastClusterLimit = LAST_CLUSTER_FAT32;
    }
    else
    {
        if (cluster != 0)
        {
            offset2  = offset2 % (volume->SecPerClus);
        }
        LastClusterLimit = LAST_CLUSTER_FAT16;
    }

    uint32_t ccls = fileptr->dirccls;

    if (ForceRead || (*curEntry & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0)
    {
        // do we need to load the next cluster?
        if ((offset2 == 0 && *curEntry >= DirectoriesPerSector) || ForceRead)
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

                    numofclus--;
                }
            }
        }

        if (ccls < LastClusterLimit)
        {
            fileptr->dirccls = ccls;
            uint32_t sector = cluster2sector(volume,ccls);

            if ((ccls == volume->FatRootDirCluster) && ((sector + offset2) >= volume->data) && (volume->type != FAT32))
            {
                return NULL;
            }
            if (globalDataWriteNecessary)
            {
                // former flushdata:
                if (singleSectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                        globalFilePtr->volume->buffer,                                                  // buffer
                                        globalFilePtr->volume))
                {
                    return NULL;
                }
                globalDataWriteNecessary = false;
            }

            globalFilePtr = NULL;
            globalBufferMemSet0 = false;

            FS_ERROR error = singleSectorRead(sector + offset2, volume->buffer, volume);

            if ( error != CE_GOOD )
            {
                return NULL;
            }
            if (ForceRead)
            {
                return (FILEROOTDIRECTORYENTRY)volume->buffer + ((*curEntry)%DirectoriesPerSector);
            }
            return (FILEROOTDIRECTORYENTRY)volume->buffer;
        } // END: a valid cluster is found
        else // invalid cluster number
        {
            return NULL;
        }
    }

    return (FILEROOTDIRECTORYENTRY)(((FILEROOTDIRECTORYENTRY)volume->buffer) + ((*curEntry)%DirectoriesPerSector));
}

static FILEROOTDIRECTORYENTRY getFileAttribute(FILE* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> getFileAttribute <<<<<");
  #endif

    fileptr->dirccls = fileptr->dirclus; 
    FILEROOTDIRECTORYENTRY dir = cacheFileEntry(fileptr,fHandle,true);

    if (dir == NULL || dir->DIR_Name[0] == DIR_EMPTY || dir->DIR_Name[0] == DIR_DEL)
    {
        return NULL;
    }

    while (dir != NULL && dir->DIR_Attr == ATTR_LONG_NAME)
    {
        (*fHandle)++;
        dir = cacheFileEntry(fileptr, fHandle, false);
    }
    return dir;
}

static void updateTimeStamp(FILEROOTDIRECTORYENTRY dir)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> updateTimeStamp not yet implemented, does nothing <<<<<");
  #endif
    // TODO
}

///////////////////////////
////  File Operations  ////
///////////////////////////

/*
 This function will cache the sector of directory entries in the directory
 pointed to by the dirclus value in the FILE* 'fileptr'
 that contains the entry that corresponds to the fHandle offset.
 It will then copy the file information for that entry into the 'fo' FILE object.
*/

static uint8_t fillFILEPTR(FILE* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fillFILEPTR <<<<<");
  #endif

    FILEROOTDIRECTORYENTRY dir;

    if ((*fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0 && *fHandle != 0) // 4-bit mask because 16 root entries max per sector
    {
        fileptr->dirccls = fileptr->dirclus;
        dir = cacheFileEntry(fileptr, fHandle, true);
        showDirectoryEntry(dir);
    }
    else { dir = cacheFileEntry (fileptr, fHandle, false); }

    if (dir == NULL) { return NO_MORE; }
    if (dir->DIR_Name[0] == DIR_DEL)   { return NOT_FOUND; }
    if (dir->DIR_Name[0] == DIR_EMPTY) { return NO_MORE;   }

    uint8_t character, test=0;
    for (uint8_t i=0; i<DIR_NAMESIZE; i++)
    {
        character = dir->DIR_Name[i];
        fileptr->name[test++] = character;
    }
    character = dir->DIR_Extension[0];
    if (character != ' ')
    {
        for (uint8_t i=0; i<DIR_EXTENSION; i++)
        {
            character = dir->DIR_Extension[i];
            fileptr->name[test++]=character;
        }
    }
    fileptr->entry      = *fHandle;
    fileptr->size       = dir->DIR_FileSize;
    fileptr->cluster  = ((dir->DIR_FstClusHI)<<16) | dir->DIR_FstClusLO;
    fileptr->time       = dir->DIR_WrtTime;
    fileptr->date       = dir->DIR_WrtDate;
    fileptr->attributes = dir->DIR_Attr;
    return FOUND;
}

FS_ERROR searchFile(FILE* fileptrDest, FILE* fileptrTest, uint8_t cmd, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> searchFile <<<<<");
  #endif

    FS_ERROR error       = CE_FILE_NOT_FOUND;
    fileptrDest->dirccls   = fileptrDest->dirclus;
    uint16_t compareAttrib = 0xFFFF ^ fileptrTest->attributes;
    uint32_t fHandle = fileptrDest->entry;
    bool read = true;

    memset(fileptrDest->name, 0x20, DIR_NAMECOMP);
  #ifdef _FAT_DIAGNOSIS_
    textColor(0x0E); printf("\nfHandle (searchFile): %d",fHandle); textColor(0x0F);
  #endif
    if (fHandle == 0 || (fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) != 0) // Maximum 16 entries possible
    {
        if (cacheFileEntry(fileptrDest, &fHandle, read) == NULL)
        {
            return CE_BADCACHEREAD;
        }
    }

    uint8_t  state=0;
    uint32_t attrib;
    char character, test;
    while(error != CE_GOOD) // Loop until you reach the end or find the file
    {
      #ifdef _FAT_DIAGNOSIS_
        textColor(0x0E); printf("\n\nfHandle %u\n",fHandle); textColor(0x0F);
      #endif

        state = fillFILEPTR(fileptrDest, &fHandle);
        if (state == NO_MORE) { break; }

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
                        error = CE_GOOD;
                        for (uint8_t i=0; i<DIR_NAMECOMP; i++)
                        {
                            character = fileptrDest->name[i];
                          #ifdef _FAT_DIAGNOSIS_
                            printf("\ni: %u character: %c test: %c", i, character, fileptrTest->name[i]); textColor(0x0F); /// TEST
                          #endif
                            if (toLower(character) != toLower(fileptrTest->name[i]))
                            {
                                error = CE_FILE_NOT_FOUND;
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
                        error = CE_GOOD;                 // Indicate the already filled file data is correct and go back
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
                                    if (toLower(character) != toLower(test))
                                    {
                                        error = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }
                                }
                            }
                        }

                        // Before calling this "searchFile" fn, "formatfilename" must be called. Hence, extn always starts from position "8".
                        if ((fileptrTest->name[8] != '*') && (error == CE_GOOD))
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
                                    if (toLower(character) != toLower(test))
                                    {
                                        error = CE_FILE_NOT_FOUND; // it's not a match
                                        break;
                                    }
                                }
                            }
                        }
                    } // Attribute match
                    break; // end of case
            } // while
        } // not found // end of if (error != CE_BADCACHEREAD)
        else
        {
            // looking for an empty/re-usable entry
            if ( cmd == LOOK_FOR_EMPTY_ENTRY)
            {
                error = CE_GOOD;
            }
        } // found or not
        // increment it no matter what happened
        fHandle++;
    } // while
    return(error);
}

FS_ERROR fclose(FILE* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fclose <<<<<");
  #endif

    FS_ERROR error   = CE_GOOD;
    uint32_t fHandle = fileptr->entry;
    FILEROOTDIRECTORYENTRY dir;

    if (fileptr->Flags.write)
    {
        if (globalDataWriteNecessary)
        {
            // former flushdata:
            if (singleSectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                   globalFilePtr->volume->buffer,                                                  // buffer
                                   globalFilePtr->volume))
            {
                return CE_WRITE_ERROR;
            }
            globalDataWriteNecessary = false;
        }

        fatWrite (fileptr->volume, 0, 0, true); // works correct with floppy only with HOTFIX there
        /*
        uint32_t i, sectorFAT;
        for (i=0, sectorFAT=globalLastFATSectorRead; i<1; i++, sectorFAT+=fileptr->volume->fatsize)
        {
            if (singleSectorWrite(sectorFAT, globalBufferFATSector, fileptr->volume) != CE_GOOD)
            {
                return CLUSTER_FAIL_FAT16;
            }
        }
        */
        globalFATWriteNecessary = false;

        dir = getFileAttribute(fileptr, &fHandle);

        if (dir == NULL)
        {
            return CE_EOF; // Before: EOF // ??
        }

        // update the time
        // ...
        // TODO
        // ...
        updateTimeStamp(dir);

        dir->DIR_FileSize = fileptr->size;
        dir->DIR_Attr     = fileptr->attributes;

        if (writeFileEntry(fileptr,&fHandle))
        {
            error = CE_GOOD;
        }
        else
        {
            error = CE_WRITE_ERROR;
        }

        fileptr->Flags.write = false;
    }

    // free(fileptr); <--- TODO: check! With that line loaded user programs freeze!
    return error;
}

FS_ERROR fread(FILE* fileptr, void* dest, uint32_t count)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fread <<<<<");
  #endif
    FS_ERROR error      = CE_GOOD;
    partition_t* volume = fileptr->volume;
    uint32_t temp       = count;
    uint32_t pos        = fileptr->pos;
    uint32_t seek       = fileptr->seek;
    uint32_t size       = fileptr->size;
    uint32_t sector     = cluster2sector(volume,fileptr->ccls) + fileptr->sec;
    uint32_t sectors    = (size%512 == 0) ? size/512 : size/512+1; // Number of sectors to be read
    volume->disk->accessRemaining += sectors;

    sectors--;
    if (sectorRead(sector, volume->buffer, volume) != CE_GOOD)
    {
        error = CE_BAD_SECTOR_READ;
    }
    while (error == CE_GOOD && temp>0)
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
                    sector += fileptr->sec;
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
                dest++;
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
    strncpy(strName,dir->DIR_Name,8);
    strName[8]='.'; // 0-7 short filename, 8: dot
    strName[9]=0;   // terminate for strcat
    strncat(strName, dir->DIR_Extension, 3);

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
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatWrite forceWrite: %d <<<<<",forceWrite);
  #endif

    if ((volume->type != FAT32) && (volume->type != FAT16) && (volume->type != FAT12))
    {
        return CLUSTER_FAIL_FAT32;
    }

    uint32_t ClusterFailValue;
    if (volume->type == FAT32)
        ClusterFailValue = CLUSTER_FAIL_FAT32;
    else
        ClusterFailValue = CLUSTER_FAIL_FAT16;

    globalBufferMemSet0 = false;

    if (forceWrite) // write the current FAT sector to the partition "volume"
    {
        for (uint32_t i=0, sectorFAT=globalLastFATSectorRead; i<volume->fatcopy; i++, sectorFAT+=volume->fatsize)
        {
            if (singleSectorWrite(sectorFAT, globalBufferFATSector, volume) != CE_GOOD)
            {
                return ClusterFailValue;
            }
        }
        globalFATWriteNecessary = false;
        return 0;
    }

    uint32_t posFAT; // position (byte) in FAT
    uint8_t q;
    switch (volume->type)
    {
        case FAT32:
            posFAT = ccls * 4;
            q = 0;
            break;
        case FAT12:
            posFAT = ccls * 3;
            q      = posFAT  & 1; // odd/even
            posFAT = posFAT >> 1;
            break;
        case FAT16:
        default:
            posFAT = ccls * 2;
            q = 0;
            break;
    }

    uint32_t sector = volume->fat + posFAT/volume->sectorSize;
    posFAT &= volume->sectorSize - 1;

    if (globalLastFATSectorRead != sector)
    {
        if (globalFATWriteNecessary)
        {
            for (uint8_t i=0, lba=globalLastFATSectorRead; i<volume->fatcopy; i++, lba+=volume->fatsize)
            {
                if (singleSectorWrite(lba, globalBufferFATSector, volume) != CE_GOOD)
                {
                    return ClusterFailValue;
                }
            }
            globalFATWriteNecessary = false;
        }

        if (singleSectorRead(sector, globalBufferFATSector, volume) != CE_GOOD)
        {
            globalLastFATSectorRead = 0xFFFF;
            return ClusterFailValue;
        }
        globalLastFATSectorRead = sector;
    }

    uint8_t c;
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
            if (q) { c = ((value & 0x0F) << 4) | ( MemoryReadByte(globalBufferFATSector, posFAT) & 0x0F ); }
            else   { c = ( value & 0xFF); }

            MemoryWriteByte(globalBufferFATSector, posFAT, c);

            posFAT = (posFAT+1) & (volume->sectorSize-1);
            if (posFAT == 0)
            {
                if (fatWrite (volume, 0, 0, true) != CE_GOOD) // update FAT
                {
                    return ClusterFailValue;
                }

                if (singleSectorRead(sector+1, globalBufferFATSector, volume) != CE_GOOD) // next sector
                {
                    globalLastFATSectorRead = 0xFFFF;
                    return ClusterFailValue;
                }
                globalLastFATSectorRead = sector+1;
            }
            c = MemoryReadByte(globalBufferFATSector, posFAT); // second byte of the table entry
            if (q) { c = (value >> 4); }
            else   { c = ((value >> 8) & 0x0F) | (c & 0xF0); }
            MemoryWriteByte (globalBufferFATSector, posFAT, c);
            break;
    }
    globalFATWriteNecessary = true;
    return 0;
}


static uint32_t fatFindEmptyCluster(FILE* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatFindEmptyCluster <<<<<");
  #endif

    uint32_t EndClusterLimit, ClusterFailValue;
    partition_t* volume = fileptr->volume;
    uint32_t c = fileptr->ccls;

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

    if (c < 2)
    {
        c = 2;
    }

    fatRead(volume, c);

    uint32_t curcls = c;
    uint32_t value = 0x0;

    while(c)
    {
        if ((value = fatRead(volume, c)) == ClusterFailValue)
            return(0);

        if (value == CLUSTER_EMPTY)
            return(c);

        c++;

        if (value == EndClusterLimit || c >= (volume->maxcls+2))
            c = 2;

        if (c == curcls)
            return(0);
    }

    return(c);
}

static FS_ERROR eraseCluster(partition_t* volume, uint32_t cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> eraseCluster <<<<<");
  #endif

    uint32_t SectorAddress = cluster2sector(volume,cluster);

    if (globalDataWriteNecessary)
    {
        // former flushdata:
        if (singleSectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                globalFilePtr->volume->buffer,                                                  // buffer
                                globalFilePtr->volume))
        {
            return CE_WRITE_ERROR;
        }
        globalDataWriteNecessary = false;
    }

    globalFilePtr = NULL;

    if (globalBufferMemSet0 == false)
    {
        memset(volume->buffer, 0, SECTOR_SIZE);
        globalBufferMemSet0 = true;
    }

    for(uint16_t i=0; i<volume->SecPerClus; i++)
    {
        if (singleSectorWrite(SectorAddress++, volume->buffer, volume) != CE_GOOD)
        {
            return CE_WRITE_ERROR;
        }
    }
    return(CE_GOOD);
}

static FS_ERROR fileAllocateNewCluster(FILE* fileptr, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileAllocateNewCluster <<<<<");
  #endif

    partition_t* volume = fileptr->volume;
    uint32_t curcls;
    uint32_t c = fatFindEmptyCluster(fileptr);
    if (c == 0) { return CE_DISK_FULL; }

    switch (volume->type)
    {
        case FAT12:
            fatWrite(volume, c, LAST_CLUSTER_FAT12, false);
            break;
        case FAT16:
            fatWrite(volume, c, LAST_CLUSTER_FAT16, false);
            break;
        case FAT32:
            fatWrite(volume, c, LAST_CLUSTER_FAT32, false);
            break;
    }

    curcls = fileptr->ccls;
    fatWrite( volume, curcls, c, false);
    fileptr->ccls = c;
    if (mode == 1) return (eraseCluster(volume, c));
    return CE_GOOD;
}

// 'size' of one object, 'n' is the number of these objects
uint32_t fwrite(const void* ptr, uint32_t size, uint32_t n, FILE* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fwrite <<<<<");
  #endif

    if (!fileptr->Flags.write)
    {
        return 0;
    }
    uint32_t count = size * n;
    if (!count) {return 0;}

    FS_ERROR error      = CE_GOOD;
    globalBufferMemSet0 = false;
    partition_t* volume = fileptr->volume;
    uint16_t pos        = fileptr->pos;
    uint32_t seek       = fileptr->seek;
    uint32_t sector     = cluster2sector(volume,fileptr->ccls) + fileptr->sec;
    uint32_t sectors    = (size%512 == 0) ? size/512 : size/512+1; // Number of sectors to be written
    volume->disk->accessRemaining += sectors;

    if (globalFilePtr != fileptr)
    {
        if (globalDataWriteNecessary)
        {
            sectors--;
            // former flushdata:
            if (sectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                    globalFilePtr->volume->buffer,                                           // buffer
                                    globalFilePtr->volume))
            {
                volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                return 0; // write count
            }
            globalDataWriteNecessary = false;
        }

        globalFilePtr = fileptr;
    }
    if (globalLastDataSectorRead != sector)
    {
        if (globalDataWriteNecessary)
        {
            sectors--;
            // former flushdata:
            if (sectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                    globalFilePtr->volume->buffer,                                           // buffer
                                    globalFilePtr->volume))
            {
                volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                return 0; // write count
            }
            globalDataWriteNecessary = false;
        }

        globalBufferMemSet0 = false;
        if (singleSectorRead(sector, volume->buffer, volume) != CE_GOOD )
        {
            error = CE_BAD_SECTOR_READ;
        }
        globalLastDataSectorRead = sector;
    }

    uint32_t filesize   = fileptr->size;
    uint8_t* src        = (uint8_t*)ptr;
    uint16_t writeCount = 0;

    while (error==CE_GOOD && count>0)
    {
        if (seek==filesize)
        {
            fileptr->Flags.FileWriteEOF = true;
        }

        if (pos == volume->sectorSize)
        {
            bool needRead = true;

            if (globalDataWriteNecessary)
            {
                sectors--;
                // former flushdata:
                if (sectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                        globalFilePtr->volume->buffer,                                                  // buffer
                                        globalFilePtr->volume))
                {
                    volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                    return 0; // write count
                }
                globalDataWriteNecessary = false;
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
                volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                return 0;
            }

            if (error == CE_GOOD)
            {
                sector = cluster2sector(volume,fileptr->ccls);
                sector += fileptr->sec;
                globalFilePtr = fileptr;

                if (needRead)
                {
                    if (singleSectorRead(sector, volume->buffer, volume) != CE_GOOD)
                    {
                        error   = CE_BAD_SECTOR_READ;
                        globalLastDataSectorRead = 0xFFFFFFFF;
                        volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                        return 0;
                    }
                }
                globalLastDataSectorRead = sector;
            }
        } //  load new sector

        if (error == CE_GOOD)
        {
            // Write one byte at a time
            MemoryWriteByte(volume->buffer, pos++, *(uint8_t*)src);
            src = src + 1; // compiler bug ???
            seek++;
            count--;
            writeCount++;
            if (fileptr->Flags.FileWriteEOF)
            {
                filesize++;
            }
            globalDataWriteNecessary = true;
        }
    } // while count

    volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written

    fileptr->pos  = pos;      // save positon
    fileptr->seek = seek;     // save seek
    fileptr->size = filesize; // new filesize

    return(writeCount/size);
}


/*******************************************************************************/

static uint32_t getFullClusterNumber(FILEROOTDIRECTORYENTRY entry)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> getFullClusterNumber <<<<<");
  #endif

    uint32_t TempFullClusterCalc = entry->DIR_FstClusHI;
    TempFullClusterCalc = TempFullClusterCalc << 16;
    TempFullClusterCalc |= entry->DIR_FstClusLO;
    return TempFullClusterCalc;
}

static bool writeFileEntry(FILE* fileptr, uint32_t* curEntry)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> writeFileEntry <<<<<");
  #endif

    partition_t* volume = fileptr->volume;
    uint32_t ccls       = fileptr->dirccls;
    uint8_t offset2     = *curEntry / (volume->sectorSize/32);

    if (volume->type == FAT32 || ccls != 0)
    {
        offset2 = offset2 % (volume->SecPerClus);
    }

    uint32_t sector = cluster2sector(volume,ccls);

    return (singleSectorWrite(sector + offset2, volume->buffer, volume) == CE_GOOD );
}


static bool fatEraseClusterChain(uint32_t cluster, partition_t* volume)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatEraseClusterChain <<<<<");
  #endif

    uint32_t c2, ClusterFailValue;

    switch (volume->type)
    {
        case FAT32:
            ClusterFailValue = CLUSTER_FAIL_FAT32;
            c2 = LAST_CLUSTER_FAT32;
            break;
        case FAT12:
            ClusterFailValue = CLUSTER_FAIL_FAT16; 
            c2 = LAST_CLUSTER_FAT12;
            break;
        case FAT16:
        default:
            ClusterFailValue = CLUSTER_FAIL_FAT16;
            c2 = LAST_CLUSTER_FAT16;
            break;
    }

    enum _status {Good, Fail, Exit} status = Good;

    if (cluster <= 1)  // Cluster assigned can't be "0" and "1"
    {
        status = Exit;
    }
    else
    {
        while(status == Good)
        {
            uint32_t c = fatRead(volume, cluster);
            if (c == ClusterFailValue)
            {
                status = Fail;
            }
            else
            {
                if (c <= 1)  // Cluster assigned can't be "0" and "1"
                {
                    status = Exit;
                }
                else
                {
                    if (c >= c2)
                    {
                        status = Exit;
                    }
                    if (fatWrite(volume, cluster, CLUSTER_EMPTY, false) == ClusterFailValue)
                    {
                        status = Fail;
                    }
                    cluster = c;
                }
            }
        }
    }

    fatWrite(volume, 0, 0, true);

    return(status == Exit);
}


FS_ERROR fileErase( FILE* fileptr, uint32_t* fHandle, bool EraseClusters)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileErase <<<<<");
  #endif

    fileptr->dirccls = fileptr->dirclus;

    FILEROOTDIRECTORYENTRY dir = cacheFileEntry(fileptr, fHandle, true);
    if (dir == NULL)
    {
        return CE_BADCACHEREAD;
    }
    if (dir->DIR_Name[0] == DIR_EMPTY || dir->DIR_Name[0] == DIR_DEL)
    {
        return CE_FILE_NOT_FOUND;
    }

    dir->DIR_Name[0] = DIR_DEL;
    uint32_t clus = getFullClusterNumber(dir);

    if ((writeFileEntry( fileptr, fHandle)) == false)
    {
        return CE_ERASE_FAIL;
    }

    if (clus != fileptr->volume->FatRootDirCluster) 
    {
        if (EraseClusters)
        {
            fatEraseClusterChain(clus, fileptr->volume);
        }
    }
    return(CE_GOOD);
}

static FS_ERROR PopulateEntries(FILE* fileptr, char *name , uint32_t *fHandle, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> PopulateEntries <<<<<");
  #endif

    fileptr->dirccls = fileptr->dirclus;
    FILEROOTDIRECTORYENTRY dir = cacheFileEntry( fileptr, fHandle, true);

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

    if (writeFileEntry(fileptr,fHandle) == false)
    {
        return CE_WRITE_ERROR;
    }
    return CE_GOOD;
}

uint8_t FindEmptyEntries(FILE* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> FindEmptyEntries <<<<<");
  #endif

    uint8_t  status = NOT_FOUND;
    uint8_t  amountfound;
    uint32_t bHandle;
    uint32_t b;
    char a = ' ';
    FILEROOTDIRECTORYENTRY  dir;

    fileptr->dirccls = fileptr->dirclus;
    if ((dir = cacheFileEntry(fileptr, fHandle, true)) == NULL)
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
                dir = cacheFileEntry(fileptr, fHandle, false);
                if (dir != NULL)
                {
                    a = dir->DIR_Name[0];
                }
                (*fHandle)++;
            }
            while((a == DIR_DEL || a == DIR_EMPTY) && dir != NULL &&  ++amountfound < 1);

            if (dir == NULL)
            {
                b = fileptr->dirccls;
                if (b == fileptr->volume->FatRootDirCluster) 
                {
                    if (fileptr->volume->type != FAT32)
                        status = NO_MORE;
                    else
                    {
                        fileptr->ccls = b;

                        if (fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
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
                    if (fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
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
                if (amountfound == 1)
                {
                    status = FOUND;
                    *fHandle = bHandle;
                }
            }
        }
        *fHandle = bHandle;
    }
    return(status == FOUND);
}

static FILEROOTDIRECTORYENTRY loadDirAttrib(FILE* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> loadDirAttrib <<<<<");
  #endif

    fileptr->dirccls = fileptr->dirclus;
    // Get the entry
    FILEROOTDIRECTORYENTRY dir = cacheFileEntry(fileptr, fHandle, true);

    if (dir == NULL || dir->DIR_Name[0] == DIR_EMPTY || dir->DIR_Name[0] == DIR_DEL)
    {
        return NULL;
    }

    while(dir != NULL && dir->DIR_Attr == ATTR_LONG_NAME)
    {
        (*fHandle)++;
        dir = cacheFileEntry(fileptr, fHandle, false);
    }

    return(dir);
}

static FS_ERROR fileCreateHeadCluster(FILE* fileptr, uint32_t* cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileCreateHeadCluster <<<<<");
  #endif

    partition_t* volume = fileptr->volume;
    *cluster = fatFindEmptyCluster(fileptr);

    if (*cluster == 0)
    {
        return CE_DISK_FULL;
    }
    
    if (volume->type == FAT12)
    {
        if (fatWrite(volume, *cluster, LAST_CLUSTER_FAT12, false) == CLUSTER_FAIL_FAT16)
        {
            return CE_WRITE_ERROR;
        }
    }
    else if (volume->type == FAT16)
    {
        if (fatWrite(volume, *cluster, LAST_CLUSTER_FAT16, false) == CLUSTER_FAIL_FAT16)
        {
            return CE_WRITE_ERROR;
        }
    }
    else
    {
        if (fatWrite(volume, *cluster, LAST_CLUSTER_FAT32, false) == CLUSTER_FAIL_FAT32)
        {
            return CE_WRITE_ERROR;
        }
    }
    return eraseCluster(volume,*cluster);
}

static FS_ERROR createFirstCluster(FILE* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> createFirstCluster <<<<<");
  #endif

    uint32_t cluster;
    uint32_t fHandle = fileptr->entry;

    FS_ERROR error = fileCreateHeadCluster(fileptr,&cluster);
    if (error == CE_GOOD)
    {
        FILEROOTDIRECTORYENTRY dir = loadDirAttrib(fileptr, &fHandle);
        dir->DIR_FstClusHI = (cluster & 0x0FFF0000) >> 16; // only 28 bits in FAT32
        dir->DIR_FstClusLO = (cluster & 0x0000FFFF);

        if (writeFileEntry(fileptr, &fHandle) == false)
        {
            return CE_WRITE_ERROR;
        }
    }
    return error;
}

FS_ERROR createFileEntry(FILE* fileptr, uint32_t *fHandle, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> createFileEntry <<<<<");
  #endif

    FS_ERROR error = CE_GOOD;
    char name[11];
    *fHandle = 0;

    for (uint8_t i=0; i<FILE_NAME_SIZE; i++)
    {
        name[i] = fileptr->name[i];
    }

    if (FindEmptyEntries(fileptr, fHandle))
    {
        if ((error = PopulateEntries(fileptr, name ,fHandle, mode)) == CE_GOOD)
        {
            return createFirstCluster(fileptr);
        }
    }
    else
    {
        return CE_DIR_FULL;
    }
    return error;
}



static bool ValidateChars(char* FileName , bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> ValidateChars <<<<<");
  #endif

    bool radix = false;
    uint16_t StrSz = strlen(FileName);
    for(uint16_t i=0; i<StrSz; i++)
    {
        if (((FileName[i] <= 0x20) &&  (FileName[i] != 0x05)) ||
             (FileName[i] == 0x22) ||  (FileName[i] == 0x2B)  ||
             (FileName[i] == 0x2C) ||  (FileName[i] == 0x2F)  ||
             (FileName[i] == 0x3A) ||  (FileName[i] == 0x3B)  ||
             (FileName[i] == 0x3C) ||  (FileName[i] == 0x3D)  ||
             (FileName[i] == 0x3E) ||  (FileName[i] == 0x5B)  ||
             (FileName[i] == 0x5C) ||  (FileName[i] == 0x5D)  ||
             (FileName[i] == 0x7C) || ((FileName[i] == 0x2E)  && (radix == true)))
        {
            return false;
        }
        else
        {
            if (mode == false)
            {
                if ((FileName[i] == '*') || (FileName[i] == '?'))
                    return false;
            }
            if (FileName[i] == 0x2E)
            {
                radix = true;
            }
            // Convert lower-case to upper-case
            if ((FileName[i] >= 0x61) && (FileName[i] <= 0x7A))
            {
                FileName[i] -= 0x20;
            }
        }
    }
    return true;
}

static bool FormatFileName(const char* fileName, char* fN2, bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> FormatFileName <<<<<");
  #endif

    char* pExt;
    uint16_t temp;
    char szName[15];

    for (uint8_t count=0; count<11; count++)
    {
        fN2[count] = ' ';
    }

    if (fileName[0] == '.' || fileName[0] == 0)
    {
        return false;
    }

    temp = strlen(fileName);

    if (temp <= FILE_NAME_SIZE+1)
    {
        strcpy(szName, fileName);
    }
    else
    {
        return false; //long file name
    }

    if (!ValidateChars(szName, mode))
    {
        return false;
    }

    if ((pExt = strchr(szName, '.')) != 0)
    {
        *pExt = 0;
        pExt++;
        if (strlen(pExt) > 3)
        {
            return false;
        }
    }

    if (strlen(szName)>8)
    {
        return false;
    }

    for (uint8_t count=0; count<strlen(szName); count++)
    {
        fN2[count] = szName[count];
    }

    if (pExt && *pExt)
    {
        for (uint8_t count=0; count<strlen(pExt); count++)
        {
            fN2[count + 8] = pExt[count];
        }
    }

    return true;
}

static void fileptrCopy(FILE* dest, FILE* source)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileptrCopy <<<<<");
  #endif

    uint8_t size = sizeof(FILE);
    for(uint8_t i=0; i<size; i++)
    {
        dest[i] = source[i];
    }
}

FS_ERROR fdopen(FILE* fileptr, uint32_t* fHandle, char type)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fdopen <<<<<");
  #endif

    FS_ERROR error = CE_GOOD;

    partition_t* volume = fileptr->volume;
    if (volume->mount == false)
    {
        return CE_NOT_INIT;
    }

    fileptr->dirccls = fileptr->dirclus;
    if (*fHandle == 0)
    {
        if (cacheFileEntry(fileptr, fHandle, true) == NULL)
        {
            error = CE_BADCACHEREAD;
        }
    }
    else
    {
        if ((*fHandle & 0xf) != 0)
        {
            if (cacheFileEntry(fileptr, fHandle, true) == NULL)
            {
                error = CE_BADCACHEREAD;
            }
        }
    }

    uint8_t r = fillFILEPTR(fileptr, fHandle);
    if (r != FOUND)
    {
        return CE_FILE_NOT_FOUND;
    }
    fileptr->seek = 0;
    fileptr->ccls = fileptr->cluster;
    fileptr->sec  = 0;
    fileptr->pos  = 0;

    if (r == NOT_FOUND)
    {
        error = CE_FILE_NOT_FOUND;
    }
    else
    {
        uint32_t l = cluster2sector(volume,fileptr->ccls);

        if (globalDataWriteNecessary)
        {
            // former flushdata:
            if (singleSectorWrite(cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                  globalFilePtr->volume->buffer,                                                  // buffer
                                  globalFilePtr->volume))
            {
                return CE_WRITE_ERROR;
            }
            globalDataWriteNecessary = false;
        }

        globalFilePtr = fileptr;
        if (globalLastDataSectorRead != l)
        {
            globalBufferMemSet0 = false;
            if (singleSectorRead(l, volume->buffer, volume) != CE_GOOD)
            {
                error = CE_BAD_SECTOR_READ;
            }
            globalLastDataSectorRead = l;
        }
    }

    fileptr->Flags.FileWriteEOF = false;

    fileptr->Flags.write = (type == 'w' || type == 'a');
    fileptr->Flags.read  = !fileptr->Flags.write;

    return(error);
}

FS_ERROR fseek(FILE* fileptr, int32_t offset, int whence) // return values should be adapted to FS_ERROR types
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fseek<<<<<");
  #endif

    partition_t* volume = fileptr->volume;

    switch(whence)
    {
        case SEEK_CUR:
            offset += fileptr->seek;
            break;
        case SEEK_END:
            offset = fileptr->size - offset;
            break;
        default:
            break;
    }

    if (globalDataWriteNecessary)
    {
        // former flushdata:
        if (singleSectorWrite( cluster2sector(globalFilePtr->volume,globalFilePtr->ccls) + globalFilePtr->sec, // sector
                                globalFilePtr->volume->buffer,                                                  // buffer
                                globalFilePtr->volume))
        {
            return CE_EOF; // Before: EOF
        }
        globalDataWriteNecessary = false;
    }


    uint32_t temp = fileptr->cluster;
    fileptr->ccls = temp;
    temp = fileptr->size;

    if (offset > temp)
    {
        return CE_SEEK_ERROR; // Before: -1
    }

    fileptr->Flags.FileWriteEOF = false;
    fileptr->seek = offset;
    uint32_t numsector = offset / volume->sectorSize;
    offset -= (numsector * volume->sectorSize);
    fileptr->pos = offset;
    temp = numsector / volume->SecPerClus;
    numsector -= (volume->SecPerClus * temp);
    fileptr->sec = numsector;

    if (temp > 0)
    {
        FS_ERROR test = fileGetNextCluster(fileptr, temp);
        if (test != CE_GOOD)
        {
            if (test == CE_FAT_EOF)
            {
                if (fileptr->Flags.write)
                {
                    fileptr->ccls = fileptr->cluster;
                    if (temp != 1)
                    test = fileGetNextCluster(fileptr, temp-1);
                    if (fileAllocateNewCluster(fileptr, 0) != CE_GOOD)
                    {
                        return CE_COULD_NOT_GET_CLUSTER; // Before: -1
                    }
                }
                else
                {
                    fileptr->ccls = fileptr->cluster;
                    test = fileGetNextCluster(fileptr, temp-1);
                    if (test != CE_GOOD)
                    {
                        return CE_COULD_NOT_GET_CLUSTER; // Before: -1
                    }
                    fileptr->pos = volume->sectorSize;
                    fileptr->sec = volume->SecPerClus - 1;
                }
            }
            else
            {
                return CE_SEEK_ERROR; // Before: -1 // past the limits
            }
        }
    }

    temp = cluster2sector(volume,fileptr->ccls);
    numsector = fileptr->sec;
    temp += numsector;
    globalFilePtr = NULL;
    globalBufferMemSet0 = false;
    if (singleSectorRead(temp, volume->buffer, volume) != CE_GOOD)
    {
        return CE_BAD_SECTOR_READ; // Before: -1 // Bad read
    }
    globalLastDataSectorRead = temp;

    return CE_GOOD;
}

FILE* fopen(const char* path, const char* mode)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fopen <<<<<");
  #endif

    partition_t* part = getPartition(path);   // Save Partition
    const char* fileName = getFilename(path); // Remaining path is filename

    FILE* filePtr = (FILE*) malloc(sizeof(FILE),PAGESIZE);

    if (!FormatFileName(fileName, filePtr->name, 0))
    {
        free(filePtr);
        return NULL;
    }

    filePtr->volume     = part;
    filePtr->cluster    = 0;
    filePtr->ccls       = 0;
    filePtr->entry      = 0;
    filePtr->attributes = ATTR_ARCHIVE;
    filePtr->dirclus    = filePtr->volume->FatRootDirCluster; 
    filePtr->dirccls    = filePtr->volume->FatRootDirCluster; 

    FILE* filePtrTemp = malloc(sizeof(FILE),PAGESIZE); // why?

    uint32_t fHandle;
    FS_ERROR error;

    fileptrCopy(filePtrTemp, filePtr);

    if (searchFile(filePtr, filePtrTemp, LOOK_FOR_MATCHING_ENTRY, 0) == CE_GOOD)
    {
        switch(mode[0])
        {
            case 'w':
            case 'W':
                fHandle = filePtr->entry;
                error = fileErase(filePtr, &fHandle, true);

                if (error == CE_GOOD)
                {
                    error = createFileEntry(filePtr, &fHandle, 0);

                    if (error == CE_GOOD)
                    {
                        error = fdopen(filePtr, &fHandle, 'w');

                        if (filePtr->attributes & ATTR_DIRECTORY)
                        {
                            error = 0xFF; // ??
                        }

                        if (error == CE_GOOD)
                        {
                            error = fseek(filePtr, 0, SEEK_END);
                            if (mode[1] == '+')
                            {
                                filePtr->Flags.read = true;
                            }
                        }
                    }
                }
                break;
            case 'A':
            case 'a':
                if (filePtr->size != 0)
                {
                    fHandle = filePtr->entry;

                    error = fdopen(filePtr, &fHandle, 'w');

                    if (filePtr->attributes & ATTR_DIRECTORY)
                    {
                        error = 0xFF; // ??
                    }

                    if (error == CE_GOOD)
                    {
                        error = fseek(filePtr, 0, SEEK_END);
                        if (error != CE_GOOD)
                        {
                            error = CE_SEEK_ERROR; // correct ??
                        }
                        else
                        {
                            fatRead(part, filePtr->ccls);
                        }
                        if (mode[1] == '+')
                        {
                            filePtr->Flags.read = true;
                        }
                    }
                }
                else
                {
                    fHandle = filePtr->entry;
                    error = fileErase(filePtr, &fHandle, true);

                    if (error == CE_GOOD)
                    {
                        // now create a new one
                        error = createFileEntry(filePtr, &fHandle, 0);

                        if (error == CE_GOOD)
                        {
                            error = fdopen(filePtr, &fHandle, 'w');

                            if (filePtr->attributes & ATTR_DIRECTORY)
                            {
                                error = 0xFF; // ??
                            }

                            if (error == CE_GOOD)
                            {
                                error = fseek(filePtr, 0, SEEK_END);
                                if (error != CE_GOOD)
                                {
                                    error = CE_SEEK_ERROR; // correct ??
                                }
                                if (mode[1] == '+')
                                {
                                    filePtr->Flags.read = true;
                                }
                            }
                        }
                    }
                }
                break;
            case 'R':
            case 'r':
                fHandle = filePtr->entry;
                error = fdopen(filePtr, &fHandle, 'r');

                if (mode[1] == '+' && !(filePtr->attributes & ATTR_DIRECTORY))
                    filePtr->Flags.write = true;

                break;
            default:
                error = 0xFF; // ??
                break;
        }
    }
    else
    {
        fileptrCopy(filePtr, filePtrTemp);

        if (mode[0] == 'w' || mode[0] == 'W' || mode[0] == 'a' || mode[0] == 'A')
        {
            fHandle = 0;
            error = createFileEntry(filePtr, &fHandle, 0);

            if (error == CE_GOOD)
            {
                error = fdopen(filePtr, &fHandle, 'w');
                if (filePtr->attributes & ATTR_DIRECTORY)
                {
                    error = 0xFF; // ??
                }

                if (error == CE_GOOD)
                {
                    error = fseek(filePtr, 0, SEEK_END);
                  
                    if (error != CE_GOOD)
                    {
                        error = CE_SEEK_ERROR;
                    }
                    if (mode[1] == '+')
                    {
                        filePtr->Flags.read = true;
                    }
                }
            }
        }
        else
        {
            error = CE_FILE_NOT_FOUND;
        }
    }

    if (error != CE_GOOD)
    {
        if (filePtr)
        {
            free(filePtr);
            return NULL;
        }
    }
    return filePtr;
}
