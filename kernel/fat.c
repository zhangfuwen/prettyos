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

uint8_t sectorWrite(uint32_t sector_addr, uint8_t* buffer) // to implement
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorWrite <<<<<!");
  #endif
    textColor(0x0A); printf("\n>>>>> sectorWrite not yet implemented <<<<<!"); textColor(0x0F);
    uint8_t retVal = SUCCESS; // TEST
    return retVal;
}

uint8_t sectorRead(uint32_t sector_addr, uint8_t* buffer) // make it better!
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> sectorRead <<<<<!");
  #endif
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
            
            sector = volume->data + (cluster - ( 2 + volume->ClustersPerRootDir ) ) * volume->SecPerClus; 
        }
    }
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cluster2sector<<<<<    cluster: %u  sector %u", cluster, sector);
  #endif
    return (sector);
}

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
        l = volume->fat + (ccls>>7); // sector contains 128 dwords
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
        /* what to do? */
    }

    return(c);
}


static uint32_t fatReadQueued(PARTITION* volume, uint32_t ccls)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatReadQueued <<<<<!");
  #endif

    uint32_t l=0;

    if ((ccls & 0xFF) == 0x00) // FAT16: if LSB of cluster number == 0 then a new sector starts
    {
        if (volume->type == FAT16)
        {
            l = volume->fat + (ccls>>8); // sector contains 256 words
        }
        else if (volume->type == FAT32)
        {
            l = volume->fat + (ccls>>7); // sector contains 128 dwords
        }

        if ( sectorRead(l,volume->buffer) != SUCCESS )
        {
            return CLUSTER_FAIL;
        }
    }

    uint32_t c=0;

    if (volume->type == FAT16)
    {
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
        /* what to do? */
    }

    return(c);
}



static uint32_t fatWrite(PARTITION* volume, uint32_t cls, uint32_t v)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fatWrite <<<<<!");
  #endif
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

    if ((volume->type == FAT16) && (c>= LAST_CLUSTER_FAT16))
    {
        c = LAST_CLUSTER; /// Should return directly
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
                c2 = LAST_CLUSTER;
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

static DIRENTRY cacheFileEntry(FILEOBJ fo, uint32_t* curEntry, bool ForceRead)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> cacheFileEntry <<<<< *curEntry: %u ForceRead: %u", *curEntry, ForceRead);
  #endif



    uint32_t   numofclus  = 0;
    uint32_t   ccls       = fo->dirccls; // directory's cluster to begin looking in // unused if ForceRead is true
	uint32_t   cluster    = fo->dirclus; // first cluster of the file's directory 
    PARTITION* volume     = fo->volume;

 

    // Get the number of the entry's sector within the root dir.
    uint32_t offset2 = (*curEntry)/DIRENTRIES_PER_SECTOR;
    
    if ( ((volume->type == FAT16) && (cluster != 0)) || ((volume->type == FAT32) && (cluster != 2 /* where to get from exactly */)) ) // it is not the root directory (FAT16)
    {
        if ((volume->type == FAT16)) { offset2 = offset2 % volume->SecPerClus; }
        if ((volume->type == FAT32)) { offset2 = offset2 % (volume->SecPerClus * volume->ClustersPerRootDir); }

    }

    if (ForceRead || (((*curEntry) & 0xF) == 0))
    {
        if ( ((offset2 == 0) && ((*curEntry)>DIRENTRIES_PER_SECTOR)) || ForceRead )
        {
            if ((cluster == 0) && (volume->type == FAT16) ) // root dir FAT16
            {
                ccls = 0;
            }
            else if ((cluster == 2) && (volume->type == FAT32) ) // root dir FAT16
            {
                ccls = 2;
            }
            else // not the root dir // ??? only first sector
            {
                if (ForceRead)
                {
                    if (volume->type == FAT16) { numofclus = (*curEntry)/(DIRENTRIES_PER_SECTOR * volume->SecPerClus); }
                    if (volume->type == FAT32) { numofclus = (*curEntry)/(DIRENTRIES_PER_SECTOR * volume->SecPerClus); } // ??
                }
                else
                {
                    numofclus = 1;
                }

                while (numofclus)
                {
                    ccls = fatRead(volume,ccls);
                    if ((volume->type == FAT16) && (ccls >= LAST_CLUSTER))
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

        if ( ((volume->type == FAT16) && (ccls < LAST_CLUSTER)) || (volume->type == FAT32) )
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
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> loadDirectoryAttribute <<<<<!");
  #endif
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


static uint8_t fillFileObject(FILEOBJ fo, uint32_t* fHandle)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fillFileObject <<<<<!");
  #endif
    DIRENTRY dir = cacheFileEntry(fo,fHandle,false);
    // read first character of file name from the entry
    uint8_t a = dir->DIR_Name[0];
  #ifdef _FAT_DIAGNOSIS_
    textColor(0x0E);printf("\n\nfirst character of file name from the entry: %c\n",a);textColor(0x0F); 
  #endif
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

uint8_t fileFind(FILEOBJ foDest, FILEOBJ foCompareTo, uint8_t cmd)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileFind <<<<<!");
  #endif
    
    for (uint8_t i=0; i<DIR_NAMECOMP; i++)
    {
        foDest->name[i] = 0x20; // set name and extension to spaces
    }    

	uint8_t statusB = CE_FILE_NOT_FOUND;
    foDest->dirccls = foDest->dirclus;
    if (foDest->volume->type == FAT32)
    {
        foDest->dirccls = 2; // TEST ???
    }

    uint32_t fHandle=0;
    bool read = true;

label1: // TEST
    printf("\nfHandle: %d",fHandle);    

    if (cacheFileEntry(foDest, &fHandle, read) == NULL)
    {
        return CE_BADCACHEREAD;
    }
    else
    {
		uint8_t  state;
		uint32_t attrib;
        while(true)
        {
          #ifdef _FAT_DIAGNOSIS_
            textColor(0x0E);printf("\n\nfHandle %u\n",fHandle);textColor(0x0F); 
          #endif

            if (statusB != CE_GOOD)
            {
                state = fillFileObject(foDest, &fHandle);
                if (state == NO_MORE) { break; }
            }
            else { break; }

            if (state == FOUND)
            {
              #ifdef _FAT_DIAGNOSIS_
                textColor(0x0A);printf("\n\nstate == FOUND");textColor(0x0F); 
              #endif
                attrib =  foDest->attributes;
                attrib &= ATTR_MASK;

                if ((attrib != ATTR_VOLUME) && ((attrib & ATTR_HIDDEN) != ATTR_HIDDEN))
                {
                  #ifdef _FAT_DIAGNOSIS_
                    textColor(0x0A);printf("\n\nAn entry is found. Attributes OK for search");textColor(0x0F); /// TEST
                  #endif
                    statusB = CE_GOOD;
                    for (uint8_t i = 0; i < DIR_NAMECOMP; i++)
                    {
                        uint8_t character = foDest->name[i];
                      #ifdef _FAT_DIAGNOSIS_   
                        printf("\ni: %u character: %c test: %c", i, character, foCompareTo->name[i]); textColor(0x0F); /// TEST
                      #endif
                        if (toLower(character) != toLower(foCompareTo->name[i]))
                        {
                            statusB = CE_FILE_NOT_FOUND;
                          #ifdef _FAT_DIAGNOSIS_                      
                            textColor(0x0C);printf("\n\n %c <--- not equal", character);textColor(0x0F);
                          #endif
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

            /// TEST FAT32
            if (foDest->volume->type == FAT32)
            {
                uint32_t oldCluster = foDest->dirccls;
                foDest->dirccls += (fHandle-1)/(DIRENTRIES_PER_SECTOR * foDest->volume->SecPerClus); 
                printf("\tfoDest->dirccls: %d",foDest->dirccls);
                read = false;
                if (foDest->dirccls > oldCluster)
                {
                    /* what to do ? */
                }
                goto label1;
            }
            /// TEST FAT32

        } // END: loop until found or end of directory

        printf("\n>>> end of fileFind <<<");
        waitForKeyStroke(); //TEST

    } // END: cacheFileEntry is successful
    return statusB;
}


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
                        c2 = LAST_CLUSTER;
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

uint8_t fileDelete(FILEOBJ fo, uint32_t* fHandle, uint8_t EraseClusters)
{
  #ifdef _FAT_DIAGNOSIS_
    printf("\n>>>>> fileDelete <<<<<!");
  #endif
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

void showDirectoryEntry(DIRENTRY dir)
{
    printf("\nname.ext: %s.%s",     dir->DIR_Name,dir->DIR_Extension                );
    printf("\nattrib.:  %y",        dir->DIR_Attr                                   );
    printf("\ncluster:  %u",        dir->DIR_FstClusLO + 0x10000*dir->DIR_FstClusHI );
    printf("\nfilesize: %u byte",   dir->DIR_FileSize                               );
}


