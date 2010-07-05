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

#ifndef _FAT_H
#define _FAT_H

#include "fsmanager.h"

// RAM read/write

#define MemoryReadByte(a,f)      *(a+f) // reads a byte at an address plus an offset in RAM
#define MemoryReadWord(a,f)      *(uint16_t*)(a+f)
#define MemoryReadLong(a,f)      *(uint32_t*)(a+f)
#define MemoryWriteByte(a,f,d)   *(a+f)=d

// Media

#define SECTOR_SIZE    512

// FAT Filesystem

enum {FAT12 = 1, FAT16, FAT32};

// File

#define FILE_NAME_SIZE       11

#define END_CLUSTER_FAT12    0xFF7
#define LAST_CLUSTER_FAT12   0xFF8

#define END_CLUSTER_FAT16    0xFFFE
#define LAST_CLUSTER_FAT16   0xFFF8
#define CLUSTER_FAIL_FAT16   0xFFFF

#define END_CLUSTER_FAT32    0x0FFFFFF7
#define LAST_CLUSTER_FAT32   0x0FFFFFF8
#define CLUSTER_FAIL_FAT32   0x0FFFFFFF

#define MASK_MAX_FILE_ENTRY_LIMIT_BITS  0x0F // This is used to indicate to the Cache_File_Entry function that a new sector needs to be loaded.

#define CLUSTER_EMPTY        0x0000

#define ATTR_MASK            0x3F
#define ATTR_READ_ONLY       0x01
#define ATTR_HIDDEN          0x02
#define ATTR_SYSTEM          0x04
#define ATTR_VOLUME          0x08
#define ATTR_DIRECTORY       0x10
#define ATTR_ARCHIVE         0x20
#define ATTR_LONG_NAME       0x0F


// Directory

#define DIRECTORY             0x12
#define NUMBER_OF_BYTES_IN_DIR_ENTRY    32

#define FOUND                 0    // dir entry match
#define NOT_FOUND             1    // dir entry not found
#define NO_MORE               2    // no more files found

#define DIR_NAMESIZE          8
#define DIR_EXTENSION         3
#define DIR_NAMECOMP         (DIR_NAMESIZE+DIR_EXTENSION)

#define DIR_DEL               0xE5
#define DIR_EMPTY             0


// Volume with FAT Filesystem
struct disk;
typedef struct
{
    partition_t* part;          // universal partition container (fsmanager)
    uint32_t sectorSize;        // byte per sector
    uint32_t fat;               // LBA of FAT
    uint32_t root;              // LBA of root directory
    uint32_t dataLBA;           // LBA of data area
    uint32_t maxroot;           // max entries in root dir
    uint32_t maxcls;            // max data clusters
    uint32_t fatsize;           // sectors in FAT
    uint8_t  fatcopy;           // copies of FAT
    uint8_t  SecPerClus;        // sectors per cluster
    uint32_t FatRootDirCluster;
} FAT_partition_t;

// File

typedef struct
{
    FAT_partition_t* volume;  // volume containing the file
    file_t*   file;           // universal file container (fsmanager)
    uint32_t  firstCluster;   // first cluster
    uint32_t  currCluster;    // current cluster
    uint16_t  sec;            // current sector in the current cluster
    uint16_t  pos;            // current byte in the current sectors
    uint16_t time;            // last update time
    uint16_t date;            // last update date
    char     name[FILE_NAME_SIZE]; // TODO: Remove
    uint32_t entry;           // file's entry position in its directory
    uint16_t chk;             // checksum = ~(entry+name[0])
    uint16_t attributes;      // file's attributes
    uint32_t dirfirstCluster; // first cluster of the file's directory
    uint32_t dircurrCluster;  // current cluster of the file's directory
} FAT_file_t;

typedef struct
{
    char DIR_Name[DIR_NAMESIZE];
    char DIR_Extension[DIR_EXTENSION];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth; // tenths of second portion
    uint16_t DIR_CrtTime;     // created
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;  // last access
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;     // last update
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} _FILEROOTDIRECTORYENTRY;
typedef _FILEROOTDIRECTORYENTRY* FILEROOTDIRECTORYENTRY;

typedef enum
{
    LOOK_FOR_EMPTY_ENTRY = 0,
    LOOK_FOR_MATCHING_ENTRY
} SEARCH_TYPE;

// file handling
FS_ERROR FAT_fileErase(FAT_file_t* fileptr, uint32_t* fHandle, bool EraseClusters);
FS_ERROR FAT_createFileEntry(FAT_file_t* fileptr, uint32_t *fHandle, uint8_t mode);
FS_ERROR FAT_searchFile(FAT_file_t* fileptrDest, FAT_file_t* fileptrTest, uint8_t cmd, uint8_t mode);
FS_ERROR FAT_fopen(file_t* file, bool create, bool open);
FS_ERROR FAT_fclose(file_t* file);
FS_ERROR FAT_fread(FAT_file_t* fileptr, void* dest, uint32_t count);
FS_ERROR FAT_fseek(file_t* file, long offset, SEEK_ORIGIN whence);
uint32_t FAT_fwrite(const void* ptr, uint32_t size, uint32_t n, FAT_file_t* stream);
FS_ERROR FAT_remove (const char* fileName, FAT_partition_t* part);

// analysis functions
void FAT_showDirectoryEntry(FILEROOTDIRECTORYENTRY dir);

//additional functions
uint32_t FAT_checksum(char* ShortFileName);

#endif
