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
#include "usb2_msd.h"
#include "console.h"
#include "fat.h"

extern PARTITION usbMSDVolume;

uint8_t sectorWrite(uint32_t sector_addr, uint8_t* buffer) // to implement
{
    textColor(0x0A);
    printf("\n>>>>> sectorWrite not yet implemented <<<<<!");
    textColor(0x0F);

    uint8_t retVal = SUCCESS; // TEST

    return retVal;
}

uint8_t sectorRead(uint32_t sector_addr, uint8_t* buffer) // make it better!
{
    printf("\n>>>>> sectorRead <<<<<!");
    uint8_t retVal = SUCCESS; // TEST

    usbRead(sector_addr, buffer); // until now only realized for USB 2.0 Mass Storage Device

    return retVal;
}

static uint32_t cluster2sector(PARTITION* volume, uint32_t cluster)
{
    uint32_t sector = 0;
    if (cluster <= 1) // root dir
    {
        sector = volume->root + cluster;
    }
    else // data area
    {
        if (volume->type == FAT16)
        {
            sector = volume->data + (cluster-2) * volume->SecPerClus;
        }
        else if (volume->type == FAT32)
        {
            sector = volume->data + (cluster-3) * volume->SecPerClus; // HOTFIX for FAT32
        }
    }
    printf("\n>>>>> cluster2sector<<<<<    cluster: %d  sector %d", cluster, sector);
    return (sector);
}

static uint32_t fatRead(PARTITION* volume, uint32_t ccls)
{
    printf("\n>>>>> fatRead <<<<<!");

    uint32_t l = volume->fat + (ccls>>8);

    if ( sectorRead(l, volume->buffer) != SUCCESS )
    {
        return(CLUSTER_FAIL);
    }

    uint32_t c = RAMreadW(volume->buffer, ((ccls&0xFF)<<1));

    if (c >= LAST_CLUSTER_FAT16)
    {
        return(LAST_CLUSTER);
    }

    return(c);
}


static uint32_t fatReadQueued(PARTITION* volume, uint32_t ccls)
{
    printf("\n>>>>> fatReadQueued <<<<<!");

    if ((ccls & 0xFF) == 0x00)
    {
        uint32_t l = volume->fat + (ccls>>8);

        if ( sectorRead(l,volume->buffer) != SUCCESS )
        {
            return CLUSTER_FAIL;
        }
    }

    uint32_t c = RAMreadW(volume->buffer, ((ccls&0xFF)<<1));

    if (c >= LAST_CLUSTER_FAT16)
    {
        return(LAST_CLUSTER);
    }

    return c;
}



static uint32_t fatWrite(PARTITION* volume, uint32_t cls, uint32_t v)
{
    printf("\n>>>>> fatWrite <<<<<!");
    uint32_t  c;

    uint32_t p = 2*cls;
    uint32_t l = volume->fat + (p>>9);
    p &= 0x1FF;

    if (sectorRead(l,volume->buffer) != SUCCESS) // improve!
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

    if (c>= LAST_CLUSTER_FAT16)
    {
        c = LAST_CLUSTER; /// Should return directly
    }

    return c; /// Correct? c is uninitialized...
}


static uint8_t fileSearchNextCluster(FILEOBJ fo, uint32_t n)
{
    printf("\n>>>>> fileSearchNextCluster <<<<<!");
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
            c2 = LAST_CLUSTER;

            if (c>=c2)
            {
                return  CE_FAT_EOF;
            }
        }
        fo->ccls = c;
    }
    while (--n > 0);

    return CE_GOOD;
}


static uint32_t fatFindEmptyCluster(FILEOBJ fo)
{
    printf("\n>>>>> fatFindEmptyCluster <<<<<!");
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

        if ((value == END_CLUSTER) || (c >= volume->maxcls))
        {
            c=2;
        }

        if (c == curcls)
        {
            return 0;
        }

        if ((value = fatReadQueued(volume,c)) == CLUSTER_FAIL)
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



uint32_t checksum(char* ShortFileName)
{
    printf("\n>>>>> checksum <<<<<!");
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

static DIRENTRY cacheFileEntry(FILEOBJ fo, uint32_t* curEntry, bool ForceRead)
{
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %d ForceRead: %d", *curEntry, ForceRead);
    uint8_t    numofclus;
    uint32_t   ccls       = fo->dirccls;
	uint32_t   cluster    = fo->dirclus;
    PARTITION* volume     = fo->volume;

    // Get the number of the entry's sector within the root dir.
    uint8_t offset2 = (*curEntry)/DIRENTRIES_PER_SECTOR;
    if (cluster != 0)
    {
        offset2 = offset2 % (volume->SecPerClus);
    }

    if (ForceRead || (((*curEntry) & 0xF) == 0))
    {
        if ( ((offset2 == 0) && ((*curEntry)>DIRENTRIES_PER_SECTOR)) || ForceRead )
        {
            if (cluster == 0) // root dir
            {
                ccls = 0;
            }
            else // not the root dir
            {
                if (ForceRead)
                {
                    numofclus = (*curEntry)/(DIRENTRIES_PER_SECTOR * volume->SecPerClus); // changed!!!
                }
                else
                {
                    numofclus = 1;
                }

                while (numofclus)
                {
                    ccls = fatRead(volume,ccls);
                    if (ccls >= LAST_CLUSTER)
                    {
                        break;
                    }
                    else
                    {
                        numofclus--;
                    }
                }
            }
        } // END: read a cluster number from the FAT

        if (ccls < LAST_CLUSTER)
        {
            fo->dirccls = ccls;
            uint32_t sector = cluster2sector(volume,ccls);
            if ( ( ccls == 0 ) && ( (sector+offset2) >= volume->data ) )
            {
                return((DIRENTRY)NULL);
            }
            else
            {
                if (sectorRead(sector+offset2,volume->buffer) != SUCCESS)
                {

                    return((DIRENTRY)NULL);
                }
                else
                {
                    if (ForceRead)
                    {
                        return (DIRENTRY)(((DIRENTRY)volume->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR));
                    }
                    else
                    {
                        return (DIRENTRY)volume->buffer;
                    }
                }
            } // END: read an entry
        } // END: a valid cluster is found
        else // invalid cluster number
        {
            return((DIRENTRY)NULL);
        }
    }
    else // ForceRead is false AND curEntry is not the first entry in the sector
    {
        return (DIRENTRY)(((DIRENTRY)volume->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR));
    }
}

static DIRENTRY loadDirectoryAttribute(FILEOBJ fo, uint32_t* fHandle)
{
    printf("\n>>>>> loadDirectoryAttribute <<<<<!");

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
                dir = cacheFileEntry(fo,fHandle,false);
                a = dir->DIR_Attr;
            }
        }
    }
    return dir;
}

static uint8_t writeFileEntry(FILEOBJ fo, uint32_t* curEntry)
{
    printf("\n>>>>> writeFileEntry <<<<<!");

    PARTITION* volume  = fo->volume;
    uint32_t   ccls    = fo->dirccls;
    uint8_t    offset2 = (*curEntry>>4);

    if (ccls != 0)
    {
        offset2 %= volume->SecPerClus;
    }

    uint32_t sector = cluster2sector(volume,ccls);

    return (sectorWrite(sector+offset2,volume->buffer) == SUCCESS);
}

static void updateTimeStamp(DIRENTRY dir)
{
    printf("\n>>>>> updateTimeStamp not yet implemented, does nothing <<<<<!");
    // TODO
}

///////////////////////////
////  File Operations  ////
///////////////////////////


static uint8_t eraseCluster(PARTITION* volume, uint32_t cluster)
{
    printf("\n>>>>> eraseCluster <<<<<!");

    uint32_t SectorAddress = cluster2sector(volume, cluster);
    memset(volume->buffer, 0x00, SDC_SECTOR_SIZE);
    for (uint8_t i=0; i<volume->SecPerClus; i++)
    {
        if (sectorWrite(SectorAddress++,volume->buffer) != SUCCESS)
        {
            return CE_WRITE_ERROR;
        }
    }
    return CE_GOOD;
}



static uint8_t fileCreateHeadCluster(FILEOBJ fo, uint32_t* cluster)
{
    printf("\n>>>>> fileCreateHeadCluster <<<<<!");

    PARTITION* volume = fo->volume;
    *cluster = fatFindEmptyCluster(fo);

    if (*cluster == 0)
    {
        return CE_DISK_FULL;
    }
    else
    {
        if (fatWrite(volume, *cluster, LAST_CLUSTER_FAT16)==FAIL)
        {
            return CE_WRITE_ERROR;
        }
        return eraseCluster(volume,*cluster);
    }
}



static uint8_t createFirstCluster(FILEOBJ fo)
{
    printf("\n>>>>> createFirstCluster <<<<<!");
    uint8_t  error;
    uint32_t cluster;

    if ((error = fileCreateHeadCluster(fo,&cluster))==CE_GOOD)
    {
		uint32_t fHandle = fo->entry;
        DIRENTRY dir = loadDirectoryAttribute(fo, &fHandle);
        dir->DIR_FstClusLO = cluster;

        if (writeFileEntry(fo,&fHandle) != true)
        {
            return CE_WRITE_ERROR;
        }
    }
    return error;
}



static uint8_t fileAllocateNewCluster(FILEOBJ fo)
{
    printf("\n>>>>> fileAllocateNewCluster <<<<<!");

    PARTITION* volume  = fo->volume;
    uint32_t c         = fatFindEmptyCluster(fo);

    if (c==0)
    {
        return CE_DISK_FULL;
    }

    fatWrite(volume,c,LAST_CLUSTER_FAT16);
    uint32_t curcls = fo->ccls;
    fatWrite(volume,curcls,c);
    fo->ccls = c;
    return CE_GOOD;
}


static uint8_t fillFileObject(FILEOBJ fo, uint32_t* fHandle)
{
    printf("\n>>>>> fillFileObject <<<<<!");

    DIRENTRY dir = cacheFileEntry(fo,fHandle,false);

    // read first character of file name from the entry
    uint8_t a = dir->DIR_Name[0];
    textColor(0x0E);printf("\n\nfirst character of file name from the entry: %c\n",a);textColor(0x0F); //TEST

    if ((dir==(DIRENTRY)NULL) || (a == DIR_EMPTY))
    {
        return NO_MORE;
    }
    else
    {
        if (a==DIR_DEL)
        {
            return NOT_FOUND;
        }
        else
        {
			uint8_t  character, test=0;
			uint32_t temp;
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

            temp =  ((dir->DIR_FstClusHI)<<16);
            temp |= dir->DIR_FstClusLO;
            fo->cluster = temp;

            fo->time = (dir->DIR_WrtTime);
            fo->date = (dir->DIR_WrtDate);

            a = dir->DIR_Attr;
            fo->attributes = a;
            return FOUND;
        } // END: the entry is not deleted
    } // END: an entry exists
}

uint8_t fileFind(FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd)
{
    printf("\n>>>>> fileFind <<<<<!");

    /// TEST
    for (uint8_t i=0; i<DIR_NAMECOMP; i++)
    {
        foDest->name[i] = 0x20; // set name and extension to spaces
    }
    /// TEST

	uint8_t statusB = CE_FILE_NOT_FOUND;
    uint32_t fHandle=0;

    foDest->dirccls = foDest->dirclus;

    if (cacheFileEntry(foDest, &fHandle, true) == NULL)
    {
        return CE_BADCACHEREAD;
    }
    else
    {
		uint8_t  state;
		uint32_t attrib;
        while(true)
        {
            textColor(0x0E);printf("\n\nfHandle %d\n",fHandle);textColor(0x0F); /// TEST

            if (statusB != CE_GOOD)
            {
                //textColor(0x0E);printf("\nstatusB != CE_GOOD");textColor(0x0F); /// TEST
                state = fillFileObject(foDest, &fHandle);
                if (state == NO_MORE)
                {
                    //textColor(0x0E);printf("\nstate == NO_MORE");textColor(0x0F); /// TEST
                    break;
                }
            }
            else
            {
                break;
            }

            if (state == FOUND)
            {
                textColor(0x0A);printf("\n\nstate == FOUND");textColor(0x0F); /// TEST

                attrib =  foDest->attributes;
                attrib &= ATTR_MASK;

                if ((attrib != ATTR_VOLUME) && ((attrib & ATTR_HIDDEN) != ATTR_HIDDEN))
                {
                    textColor(0x0A);printf("\n\nAn entry is found. Attributes OK for search");textColor(0x0F); /// TEST

                    statusB = CE_GOOD;
                    for (uint8_t i = 0; i < DIR_NAMECOMP; i++)
                    {
                        uint8_t character = foDest->name[i];

                        //textColor(0x0A);printf("\ncharacter value: %y", character); //TEST

                        printf("\ni: %d character: %c test: %c", i, character, foCompareTo->name[i]); textColor(0x0F); /// TEST

                        if (toLower(character) != toLower(foCompareTo->name[i]))
                        {
                            statusB = CE_FILE_NOT_FOUND;
                            textColor(0x0C);printf("\n\n %c <--- not equal", character);textColor(0x0F);
							break;
                        }
                    }
                } // END: An entry is found
            }
            else
            {
                if (cmd == 2)
                {
                        statusB = CE_GOOD;
                }
            }
            fHandle++;
        } // END: loop until found or end of directory

        waitForKeyStroke(); //TEST

    } // END: cacheFileEntry is successful
    return statusB;
}


static uint8_t populateEntries(FILEOBJ fo, char* name, uint32_t* fHandle)
{
    printf("\n>>>>> populateEntries <<<<<!");

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


static uint8_t findEmptyEntry(FILEOBJ fo, uint32_t* fHandle)
{
    printf("\n>>>>> findEmptyEntry <<<<<!");
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
            uint8_t amountfound=0;
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


static uint8_t fatEraseClusterChain(uint32_t cluster, PARTITION* volume)
{
    printf("\n>>>>> fatEraseClusterChain <<<<<!");
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
                    c2 = LAST_CLUSTER;
                    if (c>=c2)
                    {
                        status = Exit;
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

uint8_t createFileEntry(FILEOBJ fo, uint32_t* fHandle)
{
    printf("\n>>>>> createFileEntry <<<<<!");
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

uint8_t fileDelete(FILEOBJ fo, uint32_t* fHandle, uint8_t EraseClusters)
{
    printf("\n>>>>> fileDelete <<<<<!");
    uint8_t  status = CE_GOOD;
    uint32_t clus   = fo->dirclus;
    PARTITION* volume = fo->volume;

    fo->dirccls = clus;

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
    printf("\n>>>>> fopen <<<<<!");

    PARTITION* volume = (PARTITION*)(fo->volume);
    uint8_t error = CE_GOOD;

    if (volume->mount == false)
    {
        textColor(0x0C); printf("\nError: CE_NOT_INIT!"); textColor(0x0F); /// MESSAGE
        return CE_NOT_INIT;
    }
    else
    {
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
    printf("\n>>>>> fclose <<<<<!");
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
    printf("\n>>>>> fread <<<<<!");
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


uint8_t fwrite(FILEOBJ fo, void* src, uint32_t count)
{
    printf("\n>>>>> fwrite <<<<<!");
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

void showDirectoryEntry(DIRENTRY dir)
{
    printf("\nname.ext: %s.%s", dir->DIR_Name,dir->DIR_Extension                );
    printf("\nattrib.:  %y",    dir->DIR_Attr                                   );
    printf("\ncluster:  %d",    dir->DIR_FstClusLO + 0x10000*dir->DIR_FstClusHI );
    printf("\nfilesize: %d",    dir->DIR_FileSize                               );
}

void testFAT(char* filename)
{
    /////////////////////////////////////////////////////////////////
    ///                                                            //
    ///   for tests with usb-MSD use FORMAT /FS:FAT or /FS:FAT32   //
    ///                                                            //
    /////////////////////////////////////////////////////////////////

    // activate volume usbMSDVolume
    // data determined in analyzeBootSector(...)
    textColor(0x03);
    printf("\nbuffer:     %X", usbMSDVolume.buffer);
    printf("\ntype:       %d", usbMSDVolume.type);
    printf("\nSecPerClus: %d", usbMSDVolume.SecPerClus);
    printf("\nmaxroot:    %d", usbMSDVolume.maxroot);
    printf("\nfatsize:    %d", usbMSDVolume.fatsize);
    printf("\nfatcopy:    %d", usbMSDVolume.fatcopy);
    printf("\nfirsts:     %d", usbMSDVolume.firsts);
    printf("\nfat:        %d", usbMSDVolume.fat);
    printf("\nroot:       %d", usbMSDVolume.root);
    printf("\ndata:       %d", usbMSDVolume.data);
    printf("\nmaxcls:     %d", usbMSDVolume.maxcls);
    printf("\nmount:      %d", usbMSDVolume.mount);
    printf("\nserial #:   %y %y %y %y", usbMSDVolume.serialNumber[0], usbMSDVolume.serialNumber[1], usbMSDVolume.serialNumber[2], usbMSDVolume.serialNumber[3]);

    textColor(0x0F);
    waitForKeyStroke();

    // file name
    FILE toCompare;
    FILEOBJ foCompareTo = &toCompare;
    strncpy(foCompareTo->name,filename,11); // <--------------- this file will be searched

    // file to search
    FILE dest;
    FILEOBJ fo  = &dest;
    fo->volume  = &usbMSDVolume;
    fo->dirclus = 0;

    uint8_t retVal = fileFind(fo, foCompareTo, 1); // fileFind(FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd)
    if (retVal == CE_GOOD)
    {
        textColor(0x0A);
        printf("\n\nfileFind OK");
        printf("\nfile name: %s", fo->name);
    }
    else
    {
        textColor(0x0C);
        printf("\n\nfileFind not OK, error: %d", retVal);
    }
    textColor(0x0F);

    printf("\nNumber of entry: %d",fo->entry); // number of file entry "clean.bat"

    fopen(fo, &(fo->entry), 'r');

    waitForKeyStroke();

    fread(fo, fo->volume->buffer, fo->size);
    printf("\n");
    memshow(  fo->volume->buffer, fo->size);
    fclose(fo);

    waitForKeyStroke();
}
