/*
*  This code is based on example code for the PIC18F4550 given in the book: 
*  Jan Axelson, "USB Mass Storage Device" (2006), web site: www.Lvr.com
*
*  The copyright, page ii, tells:
*  "No part of the book, except the program code, may be reproduced or transmitted in any form by any means 
*  without the written permission of the publisher. The program code may be stored and executed in a computer system 
*  and may be incorporated into computer programs developed by the reader." 
*
*  I am a reader and have bought this helpful book (Erhard Henkes). 
*
*  Commented code is not tested with PrettyOS at the time being
*/

#include "util.h"
#include "usb2_msd.h"
#include "console.h"
#include "fat.h"

extern DISK usbStick;

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

static uint32_t cluster2sector(DISK* dsk, uint32_t cluster) 
{    
    uint32_t sector = 0;
    if ((cluster == 0) || (cluster == 1)) // root dir
    {
        sector = dsk->root + cluster;
    }
    else // data area
    {
        if (dsk->type == FAT16)
        {
            sector = dsk->data + (cluster-2) * dsk->SecPerClus; 
        }
        if (dsk->type == FAT32)
        {
            sector = dsk->data + (cluster-3) * dsk->SecPerClus; // HOTFIX for FAT32 
        }
    }
    printf("\n>>>>> cluster2sector<<<<<    cluster: %d  sector %d", cluster, sector);
    return (sector);    
}

static uint16_t fatRead(DISK* dsk, uint16_t ccls)
{
    printf("\n>>>>> fatRead <<<<<!");
    uint16_t c, p;
    uint32_t l;

    p = ccls;
    l = dsk->fat + (p>>8);

    if ( sectorRead(l,dsk->buffer) != SUCCESS )
    {
        return CLUSTER_FAIL;
    }

    c = RAMreadW(dsk->buffer,((p&0xFF)<<1));

    if (c>= LAST_CLUSTER_FAT16)
    {
        c = LAST_CLUSTER;
    }

    return c;
}

/*
static uint16_t fatReadQueued(DISK* dsk, uint16_t ccls)
{
    printf("\n>>>>> fatReadQueued <<<<<!");
    uint16_t c, p;
    uint32_t l;

    p = ccls;
    
    if ((ccls & 0xFF) == 0x00)
    {
        l = dsk->fat + (p>>8);

        if ( sectorRead(l,dsk->buffer) != SUCCESS )
        {
            return CLUSTER_FAIL;
        }
    }

    c = RAMreadW(dsk->buffer,((p&0xFF)<<1));

    if (c>= LAST_CLUSTER_FAT16)
    {
        c = LAST_CLUSTER;
    }

    return c;
}
*/

/*
static uint16_t fatWrite(DISK* dsk, uint16_t cls, uint16_t v)
{
    printf("\n>>>>> fatWrite <<<<<!");
    uint16_t  i, c, p;
    uint32_t  l, li;

    p = 2*cls;
    l= dsk->fat + (p>>9);
    p &= 0x1FF;

    if (sectorRead(l,dsk->buffer) != SUCCESS) // improve!
    {
        return FAIL;
    }

    RAMwrite(dsk->buffer,p,v);
    RAMwrite(dsk->buffer,p+1,(v>>8));

    for (i=0, li=l; i<dsk->fatcopy; i++,li+=dsk->fatsize)
    {
        if (sectorWrite(l,dsk->buffer) != SUCCESS)
        {
            return FAIL;
        }
    }

    if (c>= LAST_CLUSTER_FAT16)
    {
        c = LAST_CLUSTER;
    }

    return c;
}
*/

static uint8_t fileSearchNextCluster(FILEOBJ fo, uint16_t n)
{
    printf("\n>>>>> fileSearchNextCluster <<<<<!");
    uint8_t  error = CE_GOOD;
    uint16_t c, c2;
    DISK* disk;

    disk = fo->dsk;
    
    do
    {
        c2 = fo->ccls;
        if ((c=fatRead(disk,c2))==FAIL)
        {
            error = CE_BAD_SECTOR_READ;
        }
        else
        {
            if (c>=disk->maxcls)
            {
                error = CE_INVALID_CLUSTER;
            }
            c2 = LAST_CLUSTER;

            if (c>=c2)
            {
                error = CE_FAT_EOF;
            }
        }
        fo->ccls = c;
    }
    while ((--n > 0) && (error == CE_GOOD));

    return error;
}

/*
static uint16_t fatFindEmptyCluster(FILEOBJ fo)
{
    printf("\n>>>>> fatFindEmptyCluster <<<<<!");
    uint16_t c, curcls, value = 0x0;
    DISK* disk;

    disk = fo->dsk;
    c    = fo->ccls;

    if (c<2)
    {
        c=2;
    }

    curcls = c;

    fatRead(disk,c);

    while (c)
    {
        c++;

        if ((value == END_CLUSTER) || (c >= disk->maxcls))
        {
            c=2;
        }

        if (c == curcls)
        {
            c=0;
            break;
        }

        if ((value = fatReadQueued(disk,c)) == CLUSTER_FAIL)
        {
            c=0;
            break;
        }

        if (value == CLUSTER_EMPTY)
        {
            break;
        }
    }
    return c;
}
*/

/*
static uint32_t checksum(char* ShortFileName)
{
    printf("\n>>>>> checksum <<<<<!");
    uint32_t Bit7, Character, Checksum=0;
    for (Character=0; Character<11; ++Character)
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
*/


///////////////
// directory //
///////////////

static DIRENTRY cacheFileEntry(FILEOBJ fo, uint16_t* curEntry, bool ForceRead)
{
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %d ForceRead: %d", *curEntry, ForceRead);
    uint8_t  numofclus, offset2;
    uint16_t ccls, cluster;
    uint32_t sector; 
    DISK*    dsk;
    DIRENTRY dir;

    dsk     = fo->dsk;
    cluster = fo->dirclus;
    ccls    = fo->dirccls;
    
    // Get the number of the entry's sector within the root dir.  
    offset2 = (*curEntry)/DIRENTRIES_PER_SECTOR;
    if (cluster != 0)
    {
        offset2 = offset2 % (dsk->SecPerClus);
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
                    numofclus = (*curEntry)/(DIRENTRIES_PER_SECTOR * dsk->SecPerClus); // changed!!!
                }
                else
                {
                    numofclus = 1;
                }

                while (numofclus)
                {
                    ccls = fatRead(dsk,ccls);
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
            sector = cluster2sector(dsk,ccls);
            if ( ( ccls == 0 ) && ( (sector+offset2) >= dsk->data ) ) 
            {
                dir = ((DIRENTRY)NULL);
            }
            else
            {
                if (sectorRead(sector+offset2,dsk->buffer) != SUCCESS)
                {
                    
                    dir = ((DIRENTRY)NULL);
                }
                else
                {
                    if (ForceRead)
                    {
                        dir = (DIRENTRY)(((DIRENTRY)dsk->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR));
                    }
                    else
                    {
                        dir = (DIRENTRY)dsk->buffer;
                    }                    
                }
            } // END: read an entry 
        } // END: a valid cluster is found
        else // invalid cluster number
        {
            dir = ((DIRENTRY)NULL);
        }
    }
    else // ForceRead is false AND curEntry is not the first entry in the sector
    {
        dir = (DIRENTRY)(((DIRENTRY)dsk->buffer) + ((*curEntry)%DIRENTRIES_PER_SECTOR));        
    }

    return dir;
}

static DIRENTRY loadDirectoryAttribute(FILEOBJ fo, uint16_t* fHandle)
{
    printf("\n>>>>> loadDirectoryAttribute <<<<<!");
    uint8_t a;
    DIRENTRY dir;

    dir = cacheFileEntry(fo,fHandle,true);
    a   = dir->DIR_Name[0];
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

static uint8_t writeFileEntry(FILEOBJ fo, uint16_t* curEntry)
{
    printf("\n>>>>> writeFileEntry <<<<<!");
    uint8_t  offset2, status;
    uint16_t ccls;
    uint32_t sector;
    DISK*    dsk;

    dsk  = fo->dsk;
    ccls = fo->dirccls;
    offset2 = (*curEntry>>4);
    
    if (ccls != 0)
    {
        offset2 %= dsk->SecPerClus;
    }

    sector = cluster2sector(dsk,ccls);

    if (sectorWrite(sector+offset2,dsk->buffer) != SUCCESS)
    {
        status = false;
    }
    else
    {
        status = true;
    }
    return status;
}

static void updateTimeStamp(DIRENTRY dir)
{
    printf("\n>>>>> updateTimeStamp not yet implemented, does nothing <<<<<!");
    // TODO 
}

///////////////////////////
////  File Operations  ////
///////////////////////////

/*
static uint8_t eraseCluster(DISK* disk, uint16_t cluster)
{
    printf("\n>>>>> eraseCluster <<<<<!");
    uint8_t  error=CE_GOOD, index;
    uint32_t SectorAddress;

    SectorAddress = cluster2sector(disk, cluster);
    memset(disk->buffer, 0x00, SDC_SECTOR_SIZE);
    for (index=0; index<disk->SecPerClus && error == CE_GOOD; index++)
    {
        if (sectorWrite(SectorAddress++,disk->buffer) != SUCCESS)
        {
            error = CE_WRITE_ERROR;
        }
    }
    return error;
}
*/

/*
static uint8_t fileCreateHeadCluster(FILEOBJ fo, uint16_t* cluster)
{
    printf("\n>>>>> fileCreateHeadCluster <<<<<!");
    uint8_t error=CE_GOOD;
    //uint16_t curcls;
    DISK*    disk;

    disk = fo->dsk;
    *cluster = fatFindEmptyCluster(fo);

    if (*cluster == 0)
    {
        error = CE_DISK_FULL;
    }
    else
    {
        if (fatWrite(disk, *cluster, LAST_CLUSTER_FAT16)==FAIL)
        {
            error = CE_WRITE_ERROR;
        }
        if (error == CE_GOOD)
        {
            error = eraseCluster(disk,*cluster);
        }
    }
    return error;
}
*/

/*
static uint8_t createFirstCluster(FILEOBJ fo)
{
    printf("\n>>>>> createFirstCluster <<<<<!");
    uint8_t  error;
    uint16_t cluster, fHandle;
    DIRENTRY dir;

    fHandle = fo->entry;

    if ((error = fileCreateHeadCluster(fo,&cluster))==CE_GOOD) 
    {
        dir = loadDirectoryAttribute(fo, &fHandle);
        dir->DIR_FstClusLO = cluster;

        if (writeFileEntry(fo,&fHandle) != true)
        {
            error = CE_WRITE_ERROR;
        }
    }
    return error;
}
*/

/*
static uint8_t fileAllocateNewCluster(FILEOBJ fo)
{
    printf("\n>>>>> fileAllocateNewCluster <<<<<!");
    uint16_t c, curcls;
    DISK*    dsk;

    dsk  = fo->dsk;
    c    = fo->ccls;
    
    c = fatFindEmptyCluster(fo);
    if (c==0)
    {   
        return CE_DISK_FULL;
    }
    fatWrite(dsk,c,LAST_CLUSTER_FAT16);
    curcls = fo->ccls;
    fatWrite(dsk,curcls,c);
    fo->ccls = c;
    return CE_GOOD;
}
*/

static uint8_t fillFileObject(FILEOBJ fo, uint16_t* fHandle)
{
    printf("\n>>>>> fillFileObject <<<<<!");
    uint8_t  a, character, index, status, test=0;
    uint32_t temp;
    DIRENTRY dir;

    dir = cacheFileEntry(fo,fHandle,false); 

    // read first character of file name from the entry
    a = dir->DIR_Name[0];
    textColor(0x0E);printf("\n\nfirst character of file name from the entry: %c\n",a);textColor(0x0F); //TEST

    if ((dir==(DIRENTRY)NULL) || (a == DIR_EMPTY))
    {
        status = NO_MORE;
    }
    else
    {
        if (a==DIR_DEL)
        {
            status = NOT_FOUND;
        }
        else
        {
            status = FOUND;
            for (index=0; index<DIR_NAMESIZE; index++)
            {
                character = dir->DIR_Name[index];
                fo->name[test++] = character;
            }

            character = dir->DIR_Extension[0];

            if(character != ' ')
            {
                for (index=0; index<DIR_EXTENSION; index++)
                {
                    character = dir->DIR_Extension[index];
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
        } // END: the entry is not deleted
    } // END: an entry exists

    return status;
}

uint8_t fileFind(FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd)
{
    printf("\n>>>>> fileFind <<<<<!");
    uint8_t  character, index, state, test, statusB = CE_FILE_NOT_FOUND;
    uint16_t attrib, fHandle=0;
    
    foDest->dirccls = foDest->dirclus;

    if (cacheFileEntry(foDest, &fHandle, true) == NULL)
    {
        statusB = CE_BADCACHEREAD;
    }
    else
    {
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
                textColor(0x0E);printf("\nelse with break");textColor(0x0F); /// TEST
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
                    character = (uint8_t)'m'; // random value
                    for (index=0;(statusB==CE_GOOD)&&(index<DIR_NAMECOMP);index++)
                    {

                        character = foDest->name[index]; 
                        
                        // textColor(0x0A);printf("\ncharacter value. %y",character); //TEST
                        
                        test = foCompareTo->name[index];
                        
                        printf("\nindex: %d character: %c test: %c",index,character,test);textColor(0x0F); /// TEST
                                                
                        if (toLower(character) != toLower(test))
                        {
                            statusB = CE_FILE_NOT_FOUND;
                            textColor(0x0C);printf("\n\nnot equal");textColor(0x0F);
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

/*
static uint8_t populateEntries(FILEOBJ fo, char* name, uint16_t* fHandle)
{
    printf("\n>>>>> populateEntries <<<<<!");
    uint8_t  error=CE_GOOD;
    DIRENTRY dir;

    dir = cacheFileEntry(fo, fHandle, true);
    strncpy(dir->DIR_Name,name,DIR_NAMECOMP);   /// TODO: check!!!!!!!!!!!!!!!!!
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
    return error;
}
*/

/*
static uint8_t findEmptyEntry(FILEOBJ fo, uint16_t* fHandle)
{
    printf("\n>>>>> findEmptyEntry <<<<<!");
    uint8_t  a, amountfound, status = NOT_FOUND;
    uint16_t bHandle;
    DIRENTRY dir;

    if ((dir=cacheFileEntry(fo, fHandle, true))==NULL)
    {
        status = CE_BADCACHEREAD;
    }
    else
    {
        while (status == NOT_FOUND)
        {
            amountfound=0;
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

    if (status==FOUND) return (true);
    else               return (false); // also possible instead of if/else: return (!status);  
}
*/

/*
static uint8_t createFileEntry(FILEOBJ fo, uint16_t* fHandle)
{
    printf("\n>>>>> createFileEntry <<<<<!");
    uint8_t error = CE_GOOD, index;
    char name[DIR_NAMECOMP];
    
    for (index=0; index<FILE_NAME_SIZE; index++)
    {
        name[index] = fo->name[index];
    }
    if (error == CE_GOOD)
    {
        *fHandle = 0;
        if (findEmptyEntry(fo,fHandle))
        {
            if ((error = populateEntries(fo, name, fHandle)) == CE_GOOD)
            {
                error = createFirstCluster(fo);
            }
        }
        else
        {
            error = CE_DIR_FULL;
        }
    }
    return error;
}
*/

/*
static uint8_t fatEraseClusterChain(uint16_t cluster, DISK* dsk)
{
    printf("\n>>>>> fatEraseClusterChain <<<<<!");
    uint16_t c, c2;
    enum _status{Good,Fail,Exit}status;
    status = Good;

    if (cluster==0 || cluster==1)
    {
        status = Exit;
    }
    else
    {
        while(status == Good)
        {
            if ((c = fatRead(dsk,cluster)) == FAIL)
            {
                status = Fail;
            }
            else
            {
                if (c==0 || c==1)
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
                    if (fatWrite(dsk,cluster, CLUSTER_EMPTY)==FAIL)
                    {
                        status = Fail;
                    }
                    cluster = c;
                }
            }
        } // END: while not the end of the chain and no error
    }
    if (status == Exit)
    {
        return true;
    }
    else
    {
        return false;
    }
}
*/

/*
uint8_t fileDelete(FILEOBJ fo, uint16_t* fHandle, uint8_t EraseClusters)
{
    printf("\n>>>>> fileDelete <<<<<!");
    uint8_t  a, status = CE_GOOD;
    uint16_t clus;
    DIRENTRY dir;
    DISK*    disk;

    disk = fo->dsk;
    clus = fo->dirclus;
    fo->dirccls = clus;

    dir = cacheFileEntry(fo, fHandle, true);
    a = dir->DIR_Name[0];
    if (dir == (DIRENTRY)NULL || a==DIR_EMPTY)
    {
        status = CE_FILE_NOT_FOUND;
    }
    else
    {
        if (a==DIR_DEL)
        {
            status = CE_FILE_NOT_FOUND;
        }
        else
        {
            a = dir->DIR_Attr;
            dir->DIR_Name[0] = DIR_DEL;
            clus = dir->DIR_FstClusLO;
            if (status != CE_GOOD || !(writeFileEntry(fo,fHandle)))
            {
                status = CE_ERASE_FAIL;
            }
            else
            {
                if (EraseClusters)
                {
                    status = ((fatEraseClusterChain(clus,disk)) ? CE_GOOD : CE_ERASE_FAIL);
                }
            }
        } // END: a not empty, not deleted entry was returned 
    } // END: a not empty entry was returned
    return status;
}
*/

uint8_t fopen(FILEOBJ fo, uint16_t* fHandle, char type)
{
    printf("\n>>>>> fopen <<<<<!");
    DISK*  dsk;
    uint8_t r, error = CE_GOOD;
    uint32_t l;

    dsk = (DISK*)(fo->dsk);
    if (dsk->mount == false)
    {
        textColor(0x0C); printf("\nError: CE_NOT_INIT!"); textColor(0x0F); /// MESSAGE
        error = CE_NOT_INIT;
    }
    else
    {
        cacheFileEntry(fo, fHandle, true);
        r = fillFileObject(fo, fHandle);
        if (r != FOUND)
        {
            error = CE_FILE_NOT_FOUND;
        }
        else // a file was found
        {
            fo->seek = 0;
            fo->ccls = fo->cluster;
            fo->sec  = 0;
            fo->pos  = 0;
            l = cluster2sector(dsk,fo->ccls); // determine LBA of the file's current cluster
            if (sectorRead(l,dsk->buffer)!=SUCCESS)
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
    uint16_t fHandle;
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

uint8_t fread(FILEOBJ fo, void* dest, uint16_t count)
{
    printf("\n>>>>> fread <<<<<!");
    DISK*    dsk;
    uint8_t  error = CE_GOOD;
    uint16_t pos;
    uint32_t l, seek, size, temp;

    dsk  = (DISK*)fo->dsk;
    temp = count;
    pos  = fo->pos;
    seek = fo->seek;
    size = fo->size;

    l = cluster2sector(dsk,fo->ccls);
    l += (uint16_t)fo->sec;
    if (sectorRead(l,dsk->buffer) != SUCCESS)
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
                if (fo->sec == dsk->SecPerClus)
                {
                    fo->sec = 0;
                    error = fileSearchNextCluster(fo,1);
                }
                if (error == CE_GOOD)
                {
                    l = cluster2sector(dsk,fo->ccls);
                    l += (uint16_t)fo->sec;
                    if (sectorRead(l,dsk->buffer) != SUCCESS)
                    {
                        error = CE_BAD_SECTOR_READ;
                    }
                }
            } // END: load new sector

            if (error == CE_GOOD)
            {
                *(uint8_t*)dest = RAMread(dsk->buffer,pos++);
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
uint8_t fwrite(FILEOBJ fo, void* src, uint16_t count)
{
    printf("\n>>>>> fwrite <<<<<!");
    DISK*    dsk;
    bool     sectorloaded = false;
    uint8_t  error = CE_GOOD;
    uint16_t pos, tempo;
    uint32_t l, seek, size;
    bool     IsWriteProtected = false; 

    if (fo->Flags.write)
    {
        if (!IsWriteProtected)
        {
            tempo = count;
            dsk   = fo->dsk;
            pos   = fo->pos;
            seek  = fo->seek;
            
            l = cluster2sector(dsk,fo->ccls);
            l += (uint16_t) fo->sec;
            if (sectorRead(l,dsk->buffer) != SUCCESS)
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
                        if (sectorWrite(l,dsk->buffer)!=SUCCESS)
                        {
                            error = CE_WRITE_ERROR;
                        }
                    }
                    pos = 0;
                    fo->sec++;
                    
                    if (fo->sec == dsk->SecPerClus)
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
                        l = cluster2sector(dsk,fo->ccls);
                        l += (uint16_t) fo->sec;
                    
                        if (sectorRead(l,dsk->buffer) != SUCCESS)
                        {
                            error = CE_BAD_SECTOR_READ;
                        }
                        sectorloaded = true;
                    }
                } // END: write a sector and read the next sector

                if (error == CE_GOOD)
                {
                    RAMwrite(dsk->buffer, pos++, *(uint8_t*)src);
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
                l = cluster2sector(dsk,fo->ccls);
                l += (uint16_t)fo->sec;
                if (sectorWrite(l,dsk->buffer) != SUCCESS)
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
            error = CE_WRITE_PROTECTED;
        }
    }
    else
    {
        error = CE_WRITE_ERROR;
    }
    return error;
}
*/

void showDirectoryEntry(DIRENTRY dir)
{
    printf("\nname.ext: %s.%s", dir->DIR_Name,dir->DIR_Extension                );
    printf("\nattrib.:  %y",    dir->DIR_Attr                                   );
    printf("\ncluster:  %d",    dir->DIR_FstClusLO + 0x10000*dir->DIR_FstClusHI ); 
    printf("\nfilesize: %d",    dir->DIR_FileSize                               );
}

void testFAT()
{
    /////////////////////////////////////////////////////////////////
    ///                                                            //
    ///   for tests with usb-MSD use FORMAT /FS:FAT or /FS:FAT32   // 
    ///                                                            //
    /////////////////////////////////////////////////////////////////

    // activate volume usbStick 
    // data determined in analyzeBootSector(...)
    textColor(0x03);
    printf("\nbuffer:     %X", usbStick.buffer);
    printf("\ntype:       %d", usbStick.type);
    printf("\nSecPerClus: %d", usbStick.SecPerClus);
    printf("\nmaxroot:    %d", usbStick.maxroot);
    printf("\nfatsize:    %d", usbStick.fatsize);
    printf("\nfatcopy:    %d", usbStick.fatcopy);
    printf("\nfirsts:     %d", usbStick.firsts);
    printf("\nfat:        %d", usbStick.fat);
    printf("\nroot:       %d", usbStick.root);
    printf("\ndata:       %d", usbStick.data);
    printf("\nmaxcls:     %d", usbStick.maxcls);
    printf("\nmount:      %d", usbStick.mount);
    textColor(0x0F);
    waitForKeyStroke();

    // file name
    FILE toCompare;
    FILEOBJ foCompareTo = &toCompare;            
    strncpy(foCompareTo->name,"CLEAN   BAT",11); // <--------------- this file will be searched
    
    // file to search
    FILE dest;
    FILEOBJ fo  = &dest;    
    fo->dsk     = &usbStick;        
    fo->dirclus = 0; 

    uint8_t retVal = fileFind(fo, foCompareTo, 1); // fileFind(FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd)
    if (retVal == CE_GOOD) 
    {
        textColor(0x0A);
        printf("\n\nfileFind OK");
        printf("\nfile name: %s", fo->name);
        textColor(0x0F);
    }
    else
    {
        textColor(0x0C);
        printf("\n\nfileFind not OK, error: %d", retVal);                
        textColor(0x0F);
    }               
    
    printf("\nNumber of entry: %d",fo->entry); // number of file entry "clean.bat"
             
    fopen(fo, &(fo->entry), 'r');        
    
    waitForKeyStroke();
    
    fread(fo, fo->dsk->buffer, fo->size); 
    printf("\n");
    memshow(  fo->dsk->buffer, fo->size); 
    fclose(fo);
    
    waitForKeyStroke();
}

