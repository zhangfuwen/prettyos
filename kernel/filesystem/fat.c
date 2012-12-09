/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

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

#include "fat.h"
#include "util/util.h"
#include "kheap.h"
#include "video/console.h"
#include "time.h"
#include "serial.h"


enum // FAT subtypes
{
    FAT12, FAT16, FAT32,
    _SUBTYPE_COUNT
};

static const struct // Stores subtype-dependant constants
{
    const uint32_t end;
    const uint32_t last;
    const uint32_t fail;
} clusterVal[_SUBTYPE_COUNT] = {
    {.end = 0xFF7,      .last = 0xFF8,      .fail = 0xFFFF},
    {.end = 0xFFFE,     .last = 0xFFF8,     .fail = 0xFFFF},
    {.end = 0x0FFFFFF7, .last = 0x0FFFFFF8, .fail = 0x0FFFFFFF}
};


static bool writeFileEntry(FAT_file_t* fileptr, uint32_t* curEntry);


static uint32_t cluster2sector(FAT_partition_t* volume, uint32_t cluster)
{
    uint32_t sector;
    if (cluster <= 1) // root dir
    {
        if (volume->part->subtype == FS_FAT32) // In FAT32, there is no separate ROOT region. It is as well stored in DATA region
        {
            sector = volume->dataLBA + cluster * volume->SecPerClus;
        }
        else
        {
            sector = volume->root + cluster * volume->SecPerClus;
        }
    }
    else // data area
    {
        sector = volume->dataLBA + (cluster-2) * volume->SecPerClus;
    }

  #ifdef _FAT_DETAIL_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> cluster2sector <<<<<    cluster: %u  sector %u", cluster, sector);
  #endif
    return (sector);
}

static bool ValidateChars(char* FileName, bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> ValidateChars <<<<<");
  #endif

    bool radix = false;
    uint16_t StrSz = strlen(FileName);
    for (uint16_t i=0; i<StrSz; i++)
    {
        if ((FileName[i] <= 0x20 &&  FileName[i] != 0x05) ||
             FileName[i] == 0x22 ||  FileName[i] == 0x2B  ||
             FileName[i] == 0x2C ||  FileName[i] == 0x2F  ||
             FileName[i] == 0x3A ||  FileName[i] == 0x3B  ||
             FileName[i] == 0x3C ||  FileName[i] == 0x3D  ||
             FileName[i] == 0x3E ||  FileName[i] == 0x5B  ||
             FileName[i] == 0x5C ||  FileName[i] == 0x5D  ||
             FileName[i] == 0x7C || (FileName[i] == 0x2E  && radix))
        {
            return false;
        }
        else
        {
            if (mode == false)
            {
                if (FileName[i] == '*' || FileName[i] == '?')
                    return false;
            }
            if (FileName[i] == 0x2E)
            {
                radix = true;
            }

            FileName[i] = toUpper(FileName[i]); // Convert lower-case to upper-case
        }
    }
    return true;
}
static bool FormatFileName(const char* fileName, char* fN2, bool mode)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> FormatFileName <<<<<");
  #endif

    memset(fN2, ' ', 11);

    if (fileName[0] == '.' || fileName[0] == 0)
    {
        return false;
    }

    char szName[15];
    if (strlen(fileName) <= FILE_NAME_SIZE+1)
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

    char* pExt = strchr(szName, '.');
    if (pExt != 0)
    {
        *pExt = 0;
        pExt++;
        if (strlen(pExt) > 3 || strlen(szName) > 8) // No 8.3-Format
        {
            return false;
        }
    }

    if (strlen(szName)>8)
    {
        return false;
    }

    strncpy(fN2, szName, strlen(szName)); // Do not copy 0

    if (pExt)
    {
        strcpy(fN2+8, pExt);
    }

    return true;
}

static uint32_t fatRead(FAT_partition_t* volume, uint32_t currCluster)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> fatRead <<<<<");
  #endif

    uint8_t  q = 0;
    uint32_t posFAT; // position (byte) in FAT

    switch (volume->part->subtype)
    {
        case FS_FAT32:
            posFAT = currCluster * 4;
            break;
        case FS_FAT12:
            posFAT = currCluster * 3;
            q      = posFAT  & 1; // odd/even
            posFAT = posFAT >> 1;
            break;
        case FS_FAT16:
        default:
            posFAT = currCluster * 2;
            break;
    }

    uint32_t sector = volume->fat + (posFAT / volume->part->disk->sectorSize);

    posFAT &= volume->part->disk->sectorSize - 1;

    uint32_t c = 0;
    uint8_t fat_buffer[volume->part->disk->sectorSize];

    if (singleSectorRead(sector, fat_buffer, volume->part->disk) != CE_GOOD)
    {
        return clusterVal[volume->type].fail;
    }

    switch (volume->part->subtype)
    {
        case FS_FAT32:
            c = MemoryReadLong(fat_buffer, posFAT);
            break;
        case FS_FAT16:
            c = MemoryReadWord(fat_buffer, posFAT);
            break;
        case FS_FAT12:
            c = MemoryReadByte(fat_buffer, posFAT);
            if (q)
                c >>= 4;
            posFAT = (posFAT +1) & (volume->part->disk->sectorSize-1);
            if (posFAT == 0)
            {
                if (singleSectorRead(sector+1, fat_buffer, volume->part->disk) != CE_GOOD)
                {
                    return clusterVal[volume->type].fail;
                }
            }
            uint32_t d = MemoryReadByte(fat_buffer, posFAT);
            if (q)
                c += d <<4;
            else
                c += (d & 0x0F)<<8;
            break;
    }
    return min(c, clusterVal[volume->type].last);
}

static FS_ERROR fileGetNextCluster(FAT_file_t* fileptr, uint32_t n)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> fileGetNextCluster <<<<<");
  #endif

    FAT_partition_t* volume = fileptr->volume;

    do
    {
        fileptr->currCluster = fatRead(volume, fileptr->currCluster);

        if (fileptr->currCluster == clusterVal[volume->type].fail)
            return CE_BAD_SECTOR_READ;

        if (fileptr->currCluster >= clusterVal[volume->type].last)
            return CE_FAT_EOF;

        if (fileptr->currCluster >= volume->maxcls)
            return CE_INVALID_CLUSTER;

    } while (--n>0);
    return CE_GOOD;
}


///////////////
// directory //
///////////////

static FAT_dirEntry_t* cacheFileEntry(FAT_file_t* fileptr, uint32_t* curEntry, bool ForceRead)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> cacheFileEntry <<<<< *curEntry: %u ForceRead: %u", *curEntry, ForceRead);
  #endif
    FAT_partition_t* volume       = fileptr->volume;
    uint32_t cluster              = fileptr->dirfirstCluster;
    uint32_t DirectoriesPerSector = volume->part->disk->sectorSize/sizeof(FAT_dirEntry_t);
    uint32_t offset2              = (*curEntry)/DirectoriesPerSector;

    if (volume->part->subtype == FS_FAT32 || cluster != 0) // FAT32 or no root dir
    {
        offset2 %= volume->SecPerClus;
    }

    uint32_t currCluster = fileptr->dircurrCluster;

    if (ForceRead || (*curEntry & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0)
    {
        // do we need to load the next cluster?
        if ((offset2 == 0 && *curEntry >= DirectoriesPerSector) || ForceRead)
        {
            if (cluster == 0)
            {
                currCluster = 0;
            }
            else
            {
                uint32_t numofclus;
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
                    currCluster = fatRead(volume, currCluster);

                    if (currCluster >= clusterVal[volume->type].last)
                    {
                        break;
                    }

                    numofclus--;
                }
            }
        }

        if (currCluster < clusterVal[volume->type].last)
        {
            fileptr->dircurrCluster = currCluster;
            uint32_t sector = cluster2sector(volume, currCluster);

            if ((currCluster == volume->FatRootDirCluster) && ((sector + offset2) >= volume->dataLBA) && (volume->part->subtype != FS_FAT32))
            {
                return (0);
            }

            FS_ERROR error = singleSectorRead(sector + offset2, volume->part->buffer, volume->part->disk);

            if (error != CE_GOOD)
            {
                return (0);
            }
            if (ForceRead)
            {
                return (FAT_dirEntry_t*)volume->part->buffer + ((*curEntry)%DirectoriesPerSector);
            }
            return (FAT_dirEntry_t*)volume->part->buffer;
        } // END: a valid cluster is found

        return (0); // invalid cluster number
    }

    return (FAT_dirEntry_t*)volume->part->buffer + ((*curEntry)%DirectoriesPerSector);
}

static FAT_dirEntry_t* getFatDirEntry(FAT_file_t* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> getFatDirEntry <<<<<");
  #endif

    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    FAT_dirEntry_t* dir = cacheFileEntry(fileptr, fHandle, true);

    if (dir == 0 || dir->Name[0] == DIR_EMPTY || dir->Name[0] == DIR_DEL)
    {
        return (0);
    }

    while (dir != 0 && dir->Attr == ATTR_LONG_NAME)
    {
        (*fHandle)++;
        dir = cacheFileEntry(fileptr, fHandle, false);
    }

    return (dir);
}

static void updateTimeStamp(FAT_dirEntry_t* dir)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\r\n>>>>> updateTimeStamp not yet implemented, does nothing <<<<<");
  #endif
    // TODO
}

///////////////////////////
////  File Operations  ////
///////////////////////////

static uint8_t fillFILEPTR(FAT_file_t* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fillFILEPTR <<<<<");
  #endif

    FAT_dirEntry_t* dir;

    if ((*fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0 && *fHandle != 0) // 4-bit mask because 16 root entries max per sector
    {
        fileptr->dircurrCluster = fileptr->dirfirstCluster;
        dir = cacheFileEntry(fileptr, fHandle, true);
      #ifdef _FAT_DIAGNOSIS_
        FAT_showDirectoryEntry(dir);
      #endif
    }
    else
        dir = cacheFileEntry(fileptr, fHandle, false);

    if (dir == 0 || dir->Name[0] == DIR_EMPTY)
        return NO_MORE;
    if (dir->Name[0] == DIR_DEL)
        return NOT_FOUND;

    memcpy(fileptr->name, dir->Name, DIR_NAMESIZE);
    if (dir->Extension[0] != ' ')
        memcpy(fileptr->name + DIR_NAMESIZE, dir->Extension, DIR_EXTENSION);

    fileptr->entry        = *fHandle;
    fileptr->file->size   = dir->FileSize;
    fileptr->firstCluster = ((dir->FstClusHI)<<16) | dir->FstClusLO;
    fileptr->time         = dir->WrtTime;
    fileptr->date         = dir->WrtDate;
    fileptr->attributes   = dir->Attr;
    return FOUND;
}

FS_ERROR FAT_searchFile(FAT_file_t* fileptrDest, char nameTest[11], uint8_t cmd)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> FAT_searchFile <<<<<");
  #endif

    FS_ERROR error              = CE_FILE_NOT_FOUND;
    fileptrDest->dircurrCluster = fileptrDest->dirfirstCluster;
    uint32_t fHandle            = fileptrDest->entry;

    memset(fileptrDest->name, 0x20, FILE_NAME_SIZE);
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\nfHandle (searchFile): %d", fHandle);
  #endif
    if (fHandle == 0 || (fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) != 0) // Maximum 16 entries possible
    {
        if (cacheFileEntry(fileptrDest, &fHandle, true) == 0)
        {
            return CE_BADCACHEREAD;
        }
    }

    while (error != CE_GOOD) // Loop until you reach the end or find the file
    {
      #ifdef _FAT_DIAGNOSIS_
        serial_log(SER_LOG_FAT, "\r\n\r\nfHandle %u\r\n", fHandle);
      #endif

        uint8_t state = fillFILEPTR(fileptrDest, &fHandle);
        if (state == NO_MORE)
            break;

        if (state == FOUND)
        {
          #ifdef _FAT_DIAGNOSIS_
            serial_log(SER_LOG_FAT, "\r\n\r\nstate == FOUND");
          #endif
            uint16_t attrib =  fileptrDest->attributes & ATTR_MASK;

            if ((attrib != ATTR_VOLUME) && ((attrib & ATTR_HIDDEN) != ATTR_HIDDEN))
            {
              #ifdef _FAT_DIAGNOSIS_
                serial_log(SER_LOG_FAT, "\r\n\r\nAn entry is found. Attributes OK for search");
              #endif
                error = CE_GOOD; // Indicate the already filled file data is correct and go back
                for (uint8_t i = (nameTest[0] != '*')?0:8; i < FILE_NAME_SIZE; i++)
                {
                    char character = fileptrDest->name[i];
                  #ifdef _FAT_DIAGNOSIS_
                    serial_log(SER_LOG_FAT, "\r\ni: %u character: %c test: %c", i, character, nameTest[i]);
                  #endif
                    if (nameTest[i] == '*') // TODO: Do we really need support for * and ? in the FAT _driver_?
                    {
                        if(i < 8)
                            i = 8;
                        else
                            break;
                    }
                    if (nameTest[i] != '?' && toLower(character) != toLower(nameTest[i]))
                    {
                        error = CE_FILE_NOT_FOUND; // it's not a match
                      #ifdef _FAT_DIAGNOSIS_
                        serial_log(SER_LOG_FAT, "\r\n\r\n %c <--- not equal", character);
                      #endif
                        break;
                    }
                }
            }
        }
        else
        {
            // looking for an empty/re-usable entry
            if (cmd == LOOK_FOR_EMPTY_ENTRY)
            {
                error = CE_GOOD;
            }
        } // found or not
        // increment it no matter what happened
        fHandle++;
    } // while
    return (error);
}

FS_ERROR FAT_fclose(file_t* file)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fclose <<<<<");
  #endif

    FAT_file_t* FATfile = file->data;
    FS_ERROR error      = CE_GOOD;
    uint32_t fHandle    = FATfile->entry;

    if (file->write)
    {
        FAT_dirEntry_t* dir = getFatDirEntry(FATfile, &fHandle);

        if (dir == 0)
        {
            return CE_EOF;
        }

        updateTimeStamp(dir);

        dir->FileSize = file->size;
        dir->Attr     = FATfile->attributes;

        if (writeFileEntry(FATfile, &fHandle))
        {
            error = CE_GOOD;
        }
        else
        {
            error = CE_WRITE_ERROR;
        }
    }

    free(FATfile);
    return error;
}

FS_ERROR FAT_fread(file_t* file, void* dest, size_t count)
{
  #ifdef _FAT_DETAIL_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fread <<<<<");
  #endif
    FAT_file_t* fatfile = file->data;
    FS_ERROR error      = CE_GOOD;
    partition_t* volume = fatfile->volume->part;
    uint32_t temp       = count;
    uint32_t pos        = fatfile->pos;
    uint32_t seek       = file->seek;
    uint32_t size       = file->size;
    uint32_t sector     = cluster2sector(volume->data, fatfile->currCluster) + fatfile->sec;
    uint32_t sectors    = (size%512 == 0) ? size/512 : size/512+1; // Number of sectors to be read
    volume->disk->accessRemaining += sectors;

    sectors--;
    if (sectorRead(sector, volume->buffer, volume->disk) != CE_GOOD)
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
            if (pos == volume->disk->sectorSize)
            {
                pos = 0;
                fatfile->sec++;
                if (fatfile->sec == fatfile->volume->SecPerClus)
                {
                    fatfile->sec = 0;
                    error = fileGetNextCluster(fatfile, 1);
                }
                if (error == CE_GOOD)
                {
                    sector = cluster2sector(volume->data, fatfile->currCluster);
                    sector += fatfile->sec;
                    sectors--;
                    if (sectorRead(sector, volume->buffer, volume->disk) != CE_GOOD)
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
                temp--;
            }
        } // END: if not EOF
    } // while no error and more bytes to copy

    volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been read

    fatfile->pos = pos;
    file->seek   = seek;
    return error;
}

void FAT_showDirectoryEntry(FAT_dirEntry_t* dir)
{
    char name[9];
    name[8] = 0;
    strncpy(name, dir->Name, 8);
    char extension[4];
    extension[3] = 0;
    strncpy(extension, dir->Extension, 3);

    serial_log(SER_LOG_FAT, "\r\nname.ext: %s.%s",   name, extension);
    serial_log(SER_LOG_FAT, "\r\nattrib.:  %yh",     dir->Attr);
    serial_log(SER_LOG_FAT, "\r\ncluster:  %u",      dir->FstClusLO + 0x10000*dir->FstClusHI);
    serial_log(SER_LOG_FAT, "\r\nfilesize: %u byte", dir->FileSize);
}


//////////////////////////////
// Write Files to Partition //
//////////////////////////////

static uint32_t fatWrite(FAT_partition_t* volume, uint32_t currCluster, uint32_t value)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fatWrite <<<<<");
  #endif

    uint32_t posFAT; // position (byte) in FAT
    uint8_t q = 0;
    switch (volume->part->subtype)
    {
        case FS_FAT32:
            posFAT = currCluster * 4;
            break;
        case FS_FAT12:
            posFAT = currCluster * 3;
            q      = posFAT  & 1; // odd/even
            posFAT = posFAT >> 1;
            break;
        case FS_FAT16:
        default:
            posFAT = currCluster * 2;
            break;
    }

    uint32_t sector = volume->fat + posFAT/volume->part->disk->sectorSize;
    posFAT &= volume->part->disk->sectorSize - 1;

    uint8_t fat_buffer[volume->part->disk->sectorSize];
    if (singleSectorRead(sector, fat_buffer, volume->part->disk) != CE_GOOD)
    {
        return clusterVal[volume->type].fail;
    }

    switch (volume->part->subtype)
    {
        case FS_FAT32:
            *((uint32_t*)(fat_buffer+posFAT)) = value&0x0FFFFFFF;
            break;
        case FS_FAT16:
            *((uint16_t*)(fat_buffer+posFAT)) = value;
            break;
        case FS_FAT12:
        {
            uint8_t c;
            if (q)
                c = ((value & 0x0F) << 4) | (MemoryReadByte(fat_buffer, posFAT) & 0x0F);
            else
                c =  (value & 0xFF);

            MemoryWriteByte(fat_buffer, posFAT, c);

            posFAT = (posFAT+1) & (volume->part->disk->sectorSize-1);
            if (posFAT == 0)
            {
                if (singleSectorWrite(sector, fat_buffer, volume->part->disk) != CE_GOOD) // update FAT
                    return clusterVal[volume->type].fail;

                sector++;
                if (singleSectorRead(sector, fat_buffer, volume->part->disk) != CE_GOOD) // next sector
                    return clusterVal[volume->type].fail;
            }
            c = MemoryReadByte(fat_buffer, posFAT); // second byte of the table entry
            if (q)
                c =  (value >> 4);
            else
                c = ((value >> 8) & 0x0F) | (c & 0xF0);
            MemoryWriteByte(fat_buffer, posFAT, c);
            break;
        }
    }
    if (singleSectorWrite(sector, fat_buffer, volume->part->disk) != CE_GOOD)
        return clusterVal[volume->type].fail;
    return (0);
}


static uint32_t fatFindEmptyCluster(FAT_file_t* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fatFindEmptyCluster <<<<<");
  #endif

    FAT_partition_t* volume = fileptr->volume;
    uint32_t c = max(2, fileptr->currCluster);

    fatRead(volume, c);

    uint32_t curcls = c;
    uint32_t value = 0x0;

    while (c)
    {
        if ((value = fatRead(volume, c)) == clusterVal[volume->type].fail)
            return (0);

        if (value == CLUSTER_EMPTY)
            return (c);

        c++;

        if (value == clusterVal[volume->type].end || c >= (volume->maxcls+2))
            c = 2;

        if (c == curcls)
            return (0);
    }

    return (c);
}

static FS_ERROR eraseCluster(FAT_partition_t* volume, uint32_t cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> eraseCluster <<<<<");
  #endif

    uint32_t SectorAddress = cluster2sector(volume, cluster);

    memset(volume->part->buffer, 0, volume->part->disk->sectorSize);

    for (uint16_t i=0; i<volume->SecPerClus; i++)
    {
        if (singleSectorWrite(SectorAddress++, volume->part->buffer, volume->part->disk) != CE_GOOD)
        {
            return CE_WRITE_ERROR;
        }
    }
    return (CE_GOOD);
}

static FS_ERROR fileAllocateNewCluster(FAT_file_t* fileptr, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fileAllocateNewCluster <<<<<");
  #endif

    FAT_partition_t* volume = fileptr->volume;
    uint32_t c = fatFindEmptyCluster(fileptr);
    if (c == 0)
        return CE_DISK_FULL;

    fatWrite(volume, c, clusterVal[volume->type].last);

    uint32_t curcls = fileptr->currCluster;
    fatWrite(volume, curcls, c);
    fileptr->currCluster = c;
    if (mode == 1)
        return (eraseCluster(volume, c));
    return CE_GOOD;
}

FS_ERROR FAT_fwrite(file_t* file, const void* ptr, size_t size)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fwrite <<<<<");
  #endif

    if (!file->write)
        return (CE_READONLY);
    if (!size)
        return (CE_GOOD);

    FAT_file_t* fatfile = file->data;
    partition_t* volume = file->volume;
    uint16_t pos        = fatfile->pos;
    uint32_t seek       = file->seek;
    uint32_t sector     = cluster2sector(volume->data, fatfile->currCluster) + fatfile->sec;
    uint32_t sectors    = (size%512 == 0) ? size/512 : size/512+1; // Number of sectors to be written

    if (singleSectorRead(sector, volume->buffer, volume->disk) != CE_GOOD)
    {
        return(CE_BAD_SECTOR_READ);
    }

    volume->disk->accessRemaining += sectors;
    uint32_t filesize   = file->size;
    uint8_t* src        = (uint8_t*)ptr;
    uint16_t writeCount = 0;
    FS_ERROR error      = CE_GOOD;

    while (error==CE_GOOD && size>0)
    {
        if (seek==filesize)
        {
            file->EOF = true;
        }

        if (pos == volume->disk->sectorSize)
        {
            bool needRead = true;

            pos = 0;
            fatfile->sec++;
            if (fatfile->sec == fatfile->volume->SecPerClus)
            {
                fatfile->sec = 0;

                if (file->EOF)
                {
                    error = fileAllocateNewCluster(fatfile, 0);
                    needRead = false;
                }
                else
                {
                    error = fileGetNextCluster(fatfile, 1);
                }
            }

            if (error == CE_DISK_FULL)
            {
                volume->disk->accessRemaining -= sectors; // Subtract sectors that have not been written
                return (0);
            }

            if (error == CE_GOOD)
            {
                sector = cluster2sector(volume->data, fatfile->currCluster);
                sector += fatfile->sec;

                if (needRead)
                {
                    if (singleSectorRead(sector, volume->buffer, volume->disk) != CE_GOOD)
                    {
                        volume->disk->accessRemaining -= sectors; // Subtract sectors which has not been written
                        return (CE_BAD_SECTOR_READ);
                    }
                }
            }
        } //  load new sector

        if (error == CE_GOOD)
        {
            // Write one byte at a time
            MemoryWriteByte(volume->buffer, pos++, *(uint8_t*)src);
            src++;
            seek++;
            size--;
            writeCount++;
            if (file->EOF)
            {
                filesize++;
            }
            sectorWrite(sector, volume->buffer, volume->disk);
        }
    } // while count

    volume->disk->accessRemaining -= sectors; // Subtract sectors that have not been written

    fatfile->pos = pos;      // save positon
    file->seek   = seek;     // save seek
    file->size   = filesize; // new filesize

    return (error);
}


/*******************************************************************************/

static uint32_t getFullClusterNumber(FAT_dirEntry_t* entry)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> getFullClusterNumber <<<<<");
  #endif

    uint32_t TempFullClusterCalc = entry->FstClusHI;
    TempFullClusterCalc = TempFullClusterCalc << 16;
    TempFullClusterCalc |= entry->FstClusLO;
    return TempFullClusterCalc;
}

static bool writeFileEntry(FAT_file_t* fileptr, uint32_t* curEntry)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> writeFileEntry <<<<<");
  #endif

    FAT_partition_t* volume = fileptr->volume;
    uint32_t currCluster = fileptr->dircurrCluster;
    uint8_t offset2 = *curEntry / (volume->part->disk->sectorSize/32);

    if (volume->part->subtype == FS_FAT32 || currCluster != 0)
    {
        offset2 = offset2 % (volume->SecPerClus);
    }

    uint32_t sector = cluster2sector(volume, currCluster);

    return (singleSectorWrite(sector + offset2, volume->part->buffer, volume->part->disk) == CE_GOOD);
}


static bool fatEraseClusterChain(uint32_t cluster, FAT_partition_t* volume)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fatEraseClusterChain <<<<<");
  #endif

    if (cluster <= 1)  // Cluster assigned can't be "0" and "1"
        return(true);

    for(;;)
    {
        uint32_t c = fatRead(volume, cluster);

        if (c == clusterVal[volume->type].fail)
            return(false);

        if (c <= 1)  // Cluster assigned can't be "0" and "1"
            return(true);

        if (c >= clusterVal[volume->type].last)
            return(true);

        if (fatWrite(volume, cluster, CLUSTER_EMPTY) == clusterVal[volume->type].fail)
            return(false);

        cluster = c;
    }
}


FS_ERROR FAT_fileErase(FAT_file_t* fileptr, uint32_t* fHandle, bool EraseClusters)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fileErase <<<<<");
  #endif

    fileptr->dircurrCluster = fileptr->dirfirstCluster;

    FAT_dirEntry_t* dir = cacheFileEntry(fileptr, fHandle, true);
    if (dir == 0)
    {
        return CE_BADCACHEREAD;
    }
    if (dir->Name[0] == DIR_EMPTY || dir->Name[0] == DIR_DEL)
    {
        return CE_FILE_NOT_FOUND;
    }

    dir->Name[0] = DIR_DEL;
    uint32_t clus = getFullClusterNumber(dir);

    if ((writeFileEntry(fileptr, fHandle)) == false)
    {
        return CE_ERASE_FAIL;
    }

    if (EraseClusters && clus != fileptr->volume->FatRootDirCluster)
    {
        fatEraseClusterChain(clus, fileptr->volume);
    }
    return (CE_GOOD);
}

static FS_ERROR PopulateEntries(FAT_file_t* fileptr, char *name, uint32_t *fHandle, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> PopulateEntries <<<<<");
  #endif

    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    FAT_dirEntry_t* dir = cacheFileEntry(fileptr, fHandle, true);

    if (dir == 0) return CE_BADCACHEREAD;

    strncpy(dir->Name, name, FILE_NAME_SIZE); // HACK, accesses dir->Name and dir->Extension
    if (mode == DIRECTORY)
    {
        dir->Attr = ATTR_DIRECTORY;
    }
    else
    {
        dir->Attr = ATTR_ARCHIVE;
    }

    dir->FileSize  =    0x0;     // file size in uint32_t
    dir->NTRes     =    0x00;    // nt reserved
    dir->FstClusHI =    0x0000;  // hiword of this entry's first cluster number
    dir->FstClusLO =    0x0000;  // loword of this entry's first cluster number

   // time info as example
    dir->CrtTimeTenth = 0xB2;    // millisecond stamp
    dir->CrtTime =      0x7278;  // time created
    dir->CrtDate =      0x32B0;  // date created
    dir->LstAccDate =   0x32B0;  // Last access date
    dir->WrtTime =      0x7279;  // last update time
    dir->WrtDate =      0x32B0;  // last update date

    fileptr->file->size  = dir->FileSize;
    fileptr->time        = dir->CrtTime;
    fileptr->date        = dir->CrtDate;
    fileptr->attributes  = dir->Attr;
    fileptr->entry       = *fHandle;

    if (writeFileEntry(fileptr, fHandle) == false)
    {
        return CE_WRITE_ERROR;
    }
    return CE_GOOD;
}

uint8_t FAT_FindEmptyEntries(FAT_file_t* fileptr, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> FindEmptyEntries <<<<<");
  #endif

    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    FAT_dirEntry_t* dir = cacheFileEntry(fileptr, fHandle, true);

    if (dir == 0)
    {
        return(false); // Failure
    }

    uint32_t bHandle;
    char a = ' ';
    for(;;)
    {
        uint8_t amountfound = 0;
        bHandle = *fHandle;

        do
        {
            dir = cacheFileEntry(fileptr, fHandle, false);
            if (dir != 0)
            {
                a = dir->Name[0];
            }
            (*fHandle)++;
        } while ((a == DIR_DEL || a == DIR_EMPTY) && dir != 0 &&  ++amountfound < 1);

        if (dir == 0)
        {
            uint32_t b = fileptr->dircurrCluster;
            if (b == fileptr->volume->FatRootDirCluster)
            {
                if (fileptr->volume->part->subtype != FS_FAT32)
                    return(false);

                fileptr->currCluster = b;

                if (fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
                    return(false);

                *fHandle = bHandle;
                return(true);
            }

            fileptr->currCluster = b;
            if (fileAllocateNewCluster(fileptr, 1) == CE_DISK_FULL)
                return(false);

            *fHandle = bHandle;
            return(true);
        }

        if (amountfound == 1)
        {
            *fHandle = bHandle;
            return(true);
        }
    }
}
static FS_ERROR fileCreateHeadCluster(FAT_file_t* fileptr, uint32_t* cluster)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fileCreateHeadCluster <<<<<");
  #endif

    FAT_partition_t* volume = fileptr->volume;
    *cluster = fatFindEmptyCluster(fileptr);

    if (*cluster == 0)
    {
        return CE_DISK_FULL;
    }

    if (fatWrite(volume, *cluster, clusterVal[volume->type].last) == clusterVal[volume->type].fail)
        return CE_WRITE_ERROR;

    return eraseCluster(volume, *cluster);
}

static FS_ERROR createFirstCluster(FAT_file_t* fileptr)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> createFirstCluster <<<<<");
  #endif

    uint32_t cluster;
    uint32_t fHandle = fileptr->entry;

    FS_ERROR error = fileCreateHeadCluster(fileptr, &cluster);
    if (error == CE_GOOD)
    {
        FAT_dirEntry_t* dir = getFatDirEntry(fileptr, &fHandle);
        dir->FstClusHI = (cluster & 0x0FFF0000) >> 16; // only 28 bits in FAT32
        dir->FstClusLO = (cluster & 0x0000FFFF);

        if (writeFileEntry(fileptr, &fHandle) == false)
        {
            return CE_WRITE_ERROR;
        }
    }
    return error;
}

FS_ERROR FAT_createFileEntry(FAT_file_t* fileptr, uint32_t *fHandle, uint8_t mode)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> createFileEntry <<<<<");
  #endif

    char name[FILE_NAME_SIZE];
    *fHandle = 0;

    memcpy(name, fileptr->name, FILE_NAME_SIZE);

    if (FAT_FindEmptyEntries(fileptr, fHandle))
    {
        FS_ERROR error = PopulateEntries(fileptr, name, fHandle, mode);
        if (error == CE_GOOD)
            return createFirstCluster(fileptr);
        return error;
    }
    return CE_DIR_FULL;
}


static FS_ERROR FAT_fdopen(FAT_file_t* fileptr, uint32_t* fHandle, char type)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fdopen <<<<<");
  #endif

    partition_t* volume = fileptr->volume->part;
    if (volume->mount == false)
    {
        return CE_NOT_INIT;
    }

    FS_ERROR error = CE_GOOD;

    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    if (*fHandle == 0)
    {
        if (cacheFileEntry(fileptr, fHandle, true) == 0)
        {
            error = CE_BADCACHEREAD;
        }
    }
    else
    {
        if ((*fHandle & 0xf) != 0)
        {
            if (cacheFileEntry(fileptr, fHandle, true) == 0)
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
    fileptr->file->seek = 0;
    fileptr->currCluster = fileptr->firstCluster;
    fileptr->sec  = 0;
    fileptr->pos  = 0;

    if (r == NOT_FOUND)
    {
        error = CE_FILE_NOT_FOUND;
    }
    else
    {
        uint32_t l = cluster2sector(volume->data, fileptr->currCluster);

        if (singleSectorRead(l, volume->buffer, volume->disk) != CE_GOOD)
        {
            error = CE_BAD_SECTOR_READ;
        }
    }

    fileptr->file->EOF = false;

    fileptr->file->write = (type == 'w' || type == 'a');
    fileptr->file->read  = !fileptr->file->write;

    return (error);
}

FS_ERROR FAT_fseek(file_t* file, int32_t offset, SEEK_ORIGIN whence)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> fseek<<<<<");
  #endif

    FAT_file_t* FATfile = file->data;
    FAT_partition_t* volume = FATfile->volume;

    switch (whence)
    {
        case SEEK_CUR:
            offset += file->seek;
            break;
        case SEEK_END:
            offset = file->size - offset;
            break;
        default:
            break;
    }

    FATfile->currCluster = FATfile->firstCluster;
    uint32_t temp =  file->size;

    if (offset > temp)
    {
        return CE_SEEK_ERROR;
    }

    file->EOF = false;
    file->seek = offset;
    uint32_t numsector = offset / volume->part->disk->sectorSize;
    offset -= (numsector * volume->part->disk->sectorSize);
    FATfile->pos = offset;
    temp = numsector / volume->SecPerClus;
    numsector -= (volume->SecPerClus * temp);
    FATfile->sec = numsector;

    if (temp > 0)
    {
        FS_ERROR test = fileGetNextCluster(FATfile, temp);
        if (test != CE_GOOD)
        {
            if (test == CE_FAT_EOF)
            {
                if (file->write)
                {
                    FATfile->currCluster = FATfile->firstCluster;
                    if (temp != 1)
                    test = fileGetNextCluster(FATfile, temp-1);
                    if (fileAllocateNewCluster(FATfile, 0) != CE_GOOD)
                    {
                        return CE_COULD_NOT_GET_CLUSTER;
                    }
                }
                else
                {
                    FATfile->currCluster = FATfile->firstCluster;
                    test = fileGetNextCluster(FATfile, temp-1);
                    if (test != CE_GOOD)
                    {
                        return CE_COULD_NOT_GET_CLUSTER;
                    }
                    FATfile->pos = volume->part->disk->sectorSize;
                    FATfile->sec = volume->SecPerClus - 1;
                }
            }
            else
            {
                return CE_SEEK_ERROR;
            }
        }
    }

    temp = cluster2sector(volume, FATfile->currCluster);
    numsector = FATfile->sec;
    temp += numsector;
    if (singleSectorRead(temp, volume->part->buffer, volume->part->disk) != CE_GOOD)
    {
        return CE_BAD_SECTOR_READ;
    }

    return CE_GOOD;
}

FS_ERROR FAT_fopen(file_t* file, bool create, bool overwrite)
{
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> FAT_fopen <<<<<");
  #endif

    FAT_file_t* FATfile = malloc(sizeof(FAT_file_t), 0, "FAT_fopen-FATfile");
    file->data = FATfile;
    FATfile->file = file;

    //HACK
    if (!FormatFileName(file->name, FATfile->name, !overwrite&&!create))
    {
        free(FATfile);
        return (CE_INVALID_FILENAME);
    }

    FATfile->volume            = file->volume->data;
    FATfile->firstCluster      = 0;
    FATfile->currCluster       = 0;
    FATfile->entry             = 0;
    FATfile->attributes        = ATTR_ARCHIVE;
    FATfile->dirfirstCluster   = FATfile->volume->FatRootDirCluster;

  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\nFATfile->volume->FatRootDirCluster: %u", FATfile->volume->FatRootDirCluster);
  #endif

    FATfile->dircurrCluster    = FATfile->volume->FatRootDirCluster;

    uint32_t fHandle;
    FS_ERROR error;

    char name[11];
    memcpy(name, FATfile->name, 11);

    if (FAT_searchFile(FATfile, name, LOOK_FOR_MATCHING_ENTRY) == CE_GOOD)
    {
        fHandle = FATfile->entry;

        if (overwrite) // Should overwrite
        {
            error = FAT_fileErase(FATfile, &fHandle, true);

            if (error == CE_GOOD)
            {
                error = FAT_createFileEntry(FATfile, &fHandle, 0);

                if (error == CE_GOOD)
                {
                    error = FAT_fdopen(FATfile, &fHandle, 'w');

                    if (FATfile->attributes & ATTR_DIRECTORY)
                    {
                        error = 0xFF; // ??
                    }
                    else
                    {
                        error = FAT_fseek(file, 0, SEEK_END);
                    }
                }
            }
        }
        else if (create) // Should not overwrite, create includes that its allowed to create it, so that it is not 'r'-mode -> Append
        {
            if (file->size != 0)
            {
                error = FAT_fdopen(FATfile, &fHandle, 'w');

                if (FATfile->attributes & ATTR_DIRECTORY)
                {
                    error = 0xFF; // ??
                }

                if (error == CE_GOOD)
                {
                    error = FAT_fseek(file, 0, SEEK_END);
                    if (error != CE_GOOD)
                    {
                        error = CE_SEEK_ERROR; // correct ??
                    }
                    else
                    {
                        fatRead(FATfile->volume, FATfile->currCluster);
                    }
                }
            }
            else
            {
                error = FAT_fileErase(FATfile, &fHandle, true);

                if (error == CE_GOOD)
                {
                    // now create a new one
                    error = FAT_createFileEntry(FATfile, &fHandle, 0);

                    if (error == CE_GOOD)
                    {
                        error = FAT_fdopen(FATfile, &fHandle, 'w');

                        if (FATfile->attributes & ATTR_DIRECTORY)
                        {
                            error = 0xFF; // ??
                        }
                        else if (error == CE_GOOD)
                        {
                            error = FAT_fseek(file, 0, SEEK_END);
                            if (error != CE_GOOD)
                            {
                                error = CE_SEEK_ERROR; // correct ??
                            }
                        }
                    }
                }
            }
        }
        else
        {
            error = FAT_fdopen(FATfile, &fHandle, 'r');
        }
    }
    else
    {
        if (create)
        {
            memcpy(FATfile->name, name, 11); // Name in file handle has been changed during search. Restore it.
            fHandle = 0;
            error = FAT_createFileEntry(FATfile, &fHandle, 0);

            if (error == CE_GOOD)
            {
                error = FAT_fdopen(FATfile, &fHandle, 'w');
                if (FATfile->attributes & ATTR_DIRECTORY)
                {
                    error = 0xFF; // ??
                }
                else
                {
                    error = FAT_fseek(file, 0, SEEK_END);

                    if (error != CE_GOOD)
                    {
                        error = CE_SEEK_ERROR;
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
        free(FATfile);
    }

    return error;
}

FS_ERROR FAT_remove(const char* fileName, partition_t* part)
{
    FAT_file_t tempFile;
    FAT_file_t* fileptr = &tempFile;
    FormatFileName(fileName, fileptr->name, false); // must be 8+3 formatted first
    fileptr->volume = part->data;
    fileptr->firstCluster = 0;
    fileptr->currCluster  = 0;
    fileptr->entry = 0;
    fileptr->attributes = ATTR_ARCHIVE;

    // start at the root directory
    fileptr->dirfirstCluster = fileptr->dircurrCluster = fileptr->volume->FatRootDirCluster;

    char name[11];
    memcpy(name, tempFile.name, 11);
    FS_ERROR result = FAT_searchFile(fileptr, name, LOOK_FOR_MATCHING_ENTRY);

    if (result != CE_GOOD)
    {
        return CE_FILE_NOT_FOUND;
    }

    if (fileptr->attributes & ATTR_DIRECTORY)
    {
        return CE_DELETE_DIR;
    }

    return FAT_fileErase(fileptr, &fileptr->entry, true);
}

static FS_ERROR FAT_fileRename(FAT_file_t* fileptr, const char* fileName)
{
    if (fileptr == 0)
    {
        return CE_FILENOTOPENED;
    }

    FormatFileName(fileName, fileptr->name, false); // must be 8+3 formatted first

    char string[12];
    strncpy(string, fileptr->name, 11);

    uint32_t goodHandle = fileptr->entry;
    uint32_t fHandle = 0;

    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    FAT_dirEntry_t* dir = cacheFileEntry(fileptr, &fHandle, true);
    if (dir == 0)
    {
        return CE_BADCACHEREAD;
    }

    if (memcmp(dir->Name, string, 11) == 0) // HACK, accesses dir->Name and dir->Extension
    {
        return CE_FILENAME_EXISTS;
    }

    while (true)
    {
        dir = cacheFileEntry(fileptr, &fHandle, false);
        if (dir == 0)
        {
            return CE_BADCACHEREAD;
        }
        if (dir->Name[0] == 0)
        {
            break;
        }

        if (memcmp(dir->Name, string, 11) == 0) // HACK, accesses dir->Name and dir->Extension
        {
            return CE_FILENAME_EXISTS;
        }
        fHandle++;
    }

    fHandle = goodHandle;
    fileptr->dircurrCluster = fileptr->dirfirstCluster;
    dir = getFatDirEntry(fileptr, &fHandle);

    if (dir == 0)
    {
        return CE_BADCACHEREAD;
    }

    memcpy(dir->Name, fileptr->name, 11); // HACK, accesses dir->Name and dir->Extension

    if (!writeFileEntry(fileptr, &fHandle))
    {
        return CE_WRITE_ERROR;
    }

    return CE_GOOD;
}

FS_ERROR FAT_rename(const char* fileNameOld, const char* fileNameNew, partition_t* part)
{
    serial_log(SER_LOG_FAT, "\r\n rename: fileNameOld: %s, fileNameNew: %s", fileNameOld, fileNameNew);

    FAT_file_t tempFile;
    FormatFileName(fileNameOld, tempFile.name, false); // must be 8+3 formatted first
    tempFile.volume = part->data;
    tempFile.firstCluster = 0;
    tempFile.currCluster  = 0;
    tempFile.entry = 0;
    tempFile.attributes = ATTR_ARCHIVE;

    // start at the root directory
    tempFile.dirfirstCluster = tempFile.dircurrCluster = tempFile.volume->FatRootDirCluster;

    char name[11];
    memcpy(name, tempFile.name, 11);
    FS_ERROR result = FAT_searchFile(&tempFile, name, LOOK_FOR_MATCHING_ENTRY);

    if (result != CE_GOOD)
    {
        return CE_FILE_NOT_FOUND;
    }

    return FAT_fileRename(&tempFile, fileNameNew);
}


static void writeBootsector(partition_t* part, uint8_t* sector)
{
    // Prepare some temp data used later
    tm_t time;
    cmosTime(&time);
    uint32_t compressedTime = (time.year << 24) | (time.month << 20) | (time.dayofmonth << 15) | (time.hour << 10) | (time.minute << 4) | time.second;

    FAT_partition_t* fpart = part->data;

    // Prepare boot sector
    memset(sector, 0, 512);

    // Jmp-Command to end of BIOS parameter block
    sector[0] = 0xE9; // jmp
    if (part->subtype == FS_FAT32)
        sector[1] = 0x57;
    else
        sector[1] = 0x3B;
    sector[2] = 0;

    // BIOS parameter block
    strncpy((char*)sector+3, "MSWIN4.1", 8); // OEM name
    sector[0x0B] = BYTE1(part->disk->sectorSize); // Bytes per sector
    sector[0x0C] = BYTE2(part->disk->sectorSize);
    sector[0x0D] = fpart->SecPerClus; // Sectors per cluster
    sector[0x0E] = BYTE1(fpart->reservedSectors); // Reserved sectors
    sector[0x0F] = BYTE2(fpart->reservedSectors);
    sector[0x10] = 2; // Number of FATs
    sector[0x11] = BYTE1(fpart->maxroot); // Maximum of root entries
    sector[0x12] = BYTE2(fpart->maxroot);
    sector[0x13] = 0; // Number of sectors. We use the 32-bit field at 0x20 therefore.
    sector[0x14] = 0;
    if (part->disk->type == &FLOPPYDISK)
        sector[0x15] = 0xF0; // Media descriptor (0xF0: Floppy with 80 tracks and 18 sectors per track)
    else
        sector[0x15] = 0xF8; // Media descriptor (0xF8: Hard disk)
    if (part->subtype == FS_FAT32)
    {
        sector[0x16] = 0; // FAT size
        sector[0x17] = 0;
    }
    else
    {
        sector[0x16] = BYTE1(fpart->fatsize); // Number of sectors for FAT12 and 16
        sector[0x17] = BYTE2(fpart->fatsize);
    }
    sector[0x18] = BYTE1(part->disk->secPerTrack); // Sectors per track
    sector[0x19] = BYTE2(part->disk->secPerTrack);
    sector[0x1A] = BYTE1(part->disk->headCount); // Number of heads
    sector[0x1B] = BYTE2(part->disk->headCount);
    sector[0x1C] = BYTE1(part->start); // Hidden sectors
    sector[0x1D] = BYTE2(part->start);
    sector[0x1E] = BYTE3(part->start);
    sector[0x1F] = BYTE4(part->start);
    sector[0x20] = BYTE1(part->size); // Number of sectors
    sector[0x21] = BYTE2(part->size);
    sector[0x22] = BYTE3(part->size);
    sector[0x23] = BYTE4(part->size);
    if (part->subtype == FS_FAT32)
    {
        sector[0x24] = BYTE1(fpart->fatsize); // FAT size
        sector[0x25] = BYTE2(fpart->fatsize);
        sector[0x26] = BYTE3(fpart->fatsize);
        sector[0x27] = BYTE4(fpart->fatsize);
        sector[0x28] = 0; // FAT flags
        sector[0x29] = 0;
        sector[0x2A] = 0; // FAT32 version
        sector[0x2B] = 0;
        sector[0x2C] = BYTE1(fpart->root); // First cluster of root
        sector[0x2D] = BYTE2(fpart->root);
        sector[0x2E] = BYTE3(fpart->root);
        sector[0x2F] = BYTE4(fpart->root);
        sector[0x30] = BYTE1(1); // Info sector
        sector[0x31] = BYTE2(1);
        sector[0x32] = BYTE1(6); // Sector of copy of bootsector
        sector[0x33] = BYTE2(6);
        memset(sector+0x34, 0, 12); // reserved
        sector[0x40] = part->disk->BIOS_driveNum; // BIOS number of device. OS-specific, so set to 0 at the moment.
        sector[0x41] = 0; // reserved
        sector[0x42] = 0x29; // Extended boot signature
        sector[0x43] = BYTE1(compressedTime); // Filesystem ID
        sector[0x44] = BYTE2(compressedTime);
        sector[0x45] = BYTE3(compressedTime);
        sector[0x46] = BYTE4(compressedTime);
        strncpyandfill((char*)sector+0x47, part->serial, 11, ' '); // Name of filesystem
        strncpyandfill((char*)sector+0x36, "FAT32", 8, ' '); // FAT type
    }
    else
    {
        sector[0x24] = part->disk->BIOS_driveNum; // BIOS number of device. OS-specific, so set to 0 at the moment.
        sector[0x25] = 0; // reserved
        sector[0x26] = 0x29; // Extended boot signature
        sector[0x27] = BYTE1(compressedTime); // Filesystem ID
        sector[0x28] = BYTE2(compressedTime);
        sector[0x29] = BYTE3(compressedTime);
        sector[0x2A] = BYTE4(compressedTime);
        strncpyandfill((char*)sector+0x2B, part->serial, 11, ' '); // Name of filesystem
        if (part->subtype == FS_FAT12)
            strncpyandfill((char*)sector+0x36, "FAT12", 8, ' '); // FAT type
        else
            strncpyandfill((char*)sector+0x36, "FAT16", 8, ' '); // FAT type
    }
    sector[510] = 0x55; // Boot signature
    sector[511] = 0xAA;
}


// HACKs
#define ROOT_DIR_ENTRIES 224
#include "storage/flpydsk.h"

FS_ERROR FAT_format(partition_t* part) // TODO: Remove floppy dependancies. Make it working for FAT16 and FAT32.
{
    /*free(part->data);
    FAT_partition_t* fpart = malloc(sizeof(FAT_partition_t), 0, "FAT_partition_t");
    part->data = fpart;*/
    FAT_partition_t* fpart = part->data; // HACK. Assuming that the partition was formatted with FAT before.

    fpart->fatcopy = 2; // HACK? Keep old value here?
    fpart->maxroot = ROOT_DIR_ENTRIES; // HACK? Keep old value here?
    fpart->SecPerClus = 1; // HACK? Keep old value here?
    fpart->fatsize = 9; // HACK? Only valid for FAT12? Keep old value here?
    switch(part->subtype)
    {
        case FS_FAT12:
            fpart->type = FAT12;
            fpart->reservedSectors = 1; // HACK? Keep old value here?
            break;
        case FS_FAT16:
            fpart->type = FAT16;
            fpart->reservedSectors = 1; // HACK? Keep old value here?
            break;
        case FS_FAT32:
            fpart->type = FAT32;
            fpart->reservedSectors = 32; // HACK? Keep old value here?
            break;
    }

    /// Prepare track 0
    static uint8_t track[9216];
    /// Bootsector
    writeBootsector(part, track);
    /// FAT1 and FAT2
    memset(track+512, 0, 9216-512);
    uint32_t FAT1sec = fpart->reservedSectors;
    uint32_t FAT2sec = FAT1sec+fpart->fatsize;
    track[FAT1sec*512]   = 0xF0;
    track[FAT2sec*512]   = 0xF0;
    track[FAT1sec*512+1] = 0xFF;
    track[FAT2sec*512+1] = 0xFF;
    track[FAT1sec*512+2] = 0xFF;
    track[FAT2sec*512+2] = 0xFF;
    /// Write track 0
    flpydsk_write_ia(0, track, TRACK); // HACK: floppy dependance

    /// Prepare track 1
    /// Prepare root directory
    memset(track, 0, 9216);
    strncpyandfill((char*)track+512, part->serial, 11, ' '); // Volume label
    track[512+11] = ATTR_VOLUME | ATTR_ARCHIVE; // Attribute
    memset(track+7680, 0xF6, 9216-7680); // format ID of MS Windows
    /// Write track 1
    flpydsk_write_ia(1, track, TRACK); // HACK: floppy dependance

    /// DONE
    printf("Quickformat complete.\r\n");
    serial_log(SER_LOG_FAT, "Quickformat complete.\r\n");
    return CE_GOOD;
}

extern uint32_t usbMSDVolumeMaxLBA; // HACK
FS_ERROR FAT_pinstall(partition_t* part)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\r\n>>>>> FAT_pinstall <<<<<");
    serial_log(SER_LOG_FAT, "\r\n>>>>> FAT_pinstall <<<<<");
  #endif

    FAT_partition_t* fpart = malloc(sizeof(FAT_partition_t), 0, "FAT_partition_t");
    part->data  = fpart;
    fpart->part = part;

    uint8_t buffer[512];
    singleSectorRead(part->start, buffer, part->disk);

    BPBbase_t* BPB     = (BPBbase_t*)buffer;
    BPB1216_t* BPB1216 = (BPB1216_t*)buffer;
    BPB32_t* BPB32     = (BPB32_t*)  buffer;

    // Determine subtype (HACK: unrecommended way to determine type. cf. FAT specification)
    if (BPB1216->FStype[0] == 'F' && BPB1216->FStype[1] == 'A' && BPB1216->FStype[2] == 'T' && BPB1216->FStype[3] == '1' && BPB1216->FStype[4] == '2')
    {
        printf("FAT12");
        part->subtype = FS_FAT12;
        fpart->type = FAT12;
    }
    else if (BPB1216->FStype[0] == 'F' && BPB1216->FStype[1] == 'A' && BPB1216->FStype[2] == 'T' && BPB1216->FStype[3] == '1' && BPB1216->FStype[4] == '6')
    {
        printf("FAT16");
        part->subtype = FS_FAT16;
        fpart->type = FAT16;
    }
    else
    {
        printf("FAT32");
        part->subtype = FS_FAT32;
        fpart->type = FAT32;
    }

    if (BPB->TotalSectors16 == 0)
        part->size = BPB->TotalSectors32;
    else
        part->size = BPB->TotalSectors16;

    fpart->fatcopy         = BPB->FATcount;
    fpart->SecPerClus      = BPB->SectorsPerCluster;
    fpart->maxroot         = BPB->MaxRootEntries;
    fpart->reservedSectors = BPB->ReservedSectors;
    fpart->fat             = part->start + BPB->ReservedSectors;
    part->serial           = malloc(5, 0, "part->serial");
    part->serial[5]        = 0;

    if (part->subtype == FS_FAT32)
    {
        fpart->fatsize           = BPB32->FATsize32;
        fpart->FatRootDirCluster = BPB32->rootCluster;
        fpart->root              = fpart->fat + fpart->fatcopy*fpart->fatsize + fpart->SecPerClus*(fpart->FatRootDirCluster-2);
        fpart->dataLBA           = fpart->root;
        memcpy(part->serial, &BPB32->VolID, 4);
      #ifdef _FAT_DIAGNOSIS_
        printf("\r\nFAT32 result: root: %u dataLBA: %u start: %u", fpart->root, fpart->dataLBA, fpart->part->start);
      #endif
    }
    else
    {
        fpart->fatsize           = BPB->FATsize16;
        fpart->FatRootDirCluster = 0;
        fpart->root              = fpart->fat + fpart->fatcopy*fpart->fatsize;
        fpart->dataLBA           = fpart->root + fpart->maxroot/(part->disk->sectorSize/sizeof(FAT_dirEntry_t));
        memcpy(part->serial, &BPB1216->VolID, 4);
      #ifdef _FAT_DIAGNOSIS_
        printf("\r\nFAT12/16 result: root: %u dataLBA: %u start: %u", fpart->root, fpart->dataLBA, fpart->part->start);
      #endif
    }

    fpart->maxcls = (usbMSDVolumeMaxLBA - fpart->dataLBA - part->start) / fpart->SecPerClus;
    return (CE_GOOD);
}


FS_ERROR FAT_folderAccess(folder_t* folder, folderAccess_t mode)
{
    // TODO: Not only root dir
    // Read directory content. Analyze it as a FAT_dirEntry_t array. Put all valid entries into the already created subfolder and files lists.

  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\n>>>>> FAT_folderAccess <<<<<");
  #endif

    // TODO: Get rid of FAT_file_t
    FAT_file_t tempFile;
    tempFile.volume = folder->volume->data;
    FAT_partition_t* fpart = folder->volume->data;
    tempFile.dirfirstCluster = fpart->FatRootDirCluster;
    tempFile.dircurrCluster = fpart->FatRootDirCluster;
    tempFile.entry = 0;

    FS_ERROR error              = CE_GOOD;
    uint32_t fHandle            = tempFile.entry;

    ///memset(fileptrDest->name, 0x20, FILE_NAME_SIZE);
  #ifdef _FAT_DIAGNOSIS_
    serial_log(SER_LOG_FAT, "\r\nfHandle (searchFile): %d", fHandle);
  #endif
    if (fHandle == 0 || (fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) != 0) // Maximum 16 entries possible
    {
        if (cacheFileEntry(&tempFile, &fHandle, true) == 0)
        {
            return CE_BADCACHEREAD;
        }
    }

    while (true) // Loop until you reach the end or find the file
    {
      #ifdef _FAT_DIAGNOSIS_
        serial_log(SER_LOG_FAT, "\r\n\r\nfHandle %u\r\n", fHandle);
      #endif
        FAT_dirEntry_t* entry;

        if ((fHandle & MASK_MAX_FILE_ENTRY_LIMIT_BITS) == 0 && fHandle != 0) // 4-bit mask because 16 root entries max per sector
        {
            tempFile.dircurrCluster = tempFile.dirfirstCluster;
            entry = cacheFileEntry(&tempFile, &fHandle, true);
          #ifdef _FAT_DIAGNOSIS_
            FAT_showDirectoryEntry(entry);
          #endif
        }
        else
            entry = cacheFileEntry(&tempFile, &fHandle, false);

        if (entry->Name[0] == DIR_EMPTY) break; // free from here on

        if ((uint8_t)entry->Name[0] != DIR_DEL &&            // Entry is not deleted
           (entry->Attr & ATTR_LONG_NAME) != ATTR_LONG_NAME) // Entry is not part of long file name (VFAT)
        {
            fsnode_t* node = malloc(sizeof(fsnode_t), 0, "fsnode");
            node->name = malloc(8+1+3+1, 0, "fsnode.name"); // Space for filename.ext (8+3)

            // Filename
            size_t letters = 0;
            for (uint8_t j = 0; j < 8; j++)
            {
                if (entry->Name[j] != 0x20) // Empty space
                {
                    node->name[letters] = entry->Name[j];
                    letters++;
                }
            }
            if (!(entry->Attr & ATTR_VOLUME) && // No volume label
                 entry->Extension[0] != 0x20 && entry->Extension[1] != 0x20 && entry->Extension[2] != 0x20) // Has extension
            {
                node->name[letters] = '.';
                letters++;
            }

            for (uint8_t j = 0; j < 3; j++)
            {
                if (entry->Extension[j] != 0x20) // Empty space
                {
                    node->name[letters] = entry->Extension[j];
                    letters++;
                }
            }
            node->name[letters] = 0;

            if(!(entry->Attr & ATTR_DIRECTORY) && !(entry->Attr & ATTR_VOLUME))
                node->size = entry->FileSize;

            node->attributes = 0;
            if(entry->Attr & ATTR_VOLUME)
                node->attributes |= NODE_VOLUME;
            if(entry->Attr & ATTR_DIRECTORY)
                node->attributes |= NODE_DIRECTORY;
            if(entry->Attr & ATTR_READ_ONLY)
                node->attributes |= NODE_READONLY;
            if(entry->Attr & ATTR_HIDDEN)
                node->attributes |= NODE_HIDDEN;
            if(entry->Attr & ATTR_SYSTEM)
                node->attributes |= NODE_SYSTEM;
            if(entry->Attr & ATTR_ARCHIVE)
                node->attributes |= NODE_ARCHIVE;

            list_append(folder->nodes, node);
        }
        // increment it no matter what happened
        fHandle++;
    } // while

    return (error);
}

void FAT_folderClose(folder_t* folder)
{
    for(dlelement_t* e = folder->nodes->head; e; e = e->next) {
        fsnode_t* node = e->data;
        free(node->name);
        free(e->data);
    }
}


/*
* Copyright (c) 2010-2012 The PrettyOS Project. All rights reserved.
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
