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

#ifndef FAT_H
#define FAT_H

#include "fsmanager.h"

// RAM read/write

#define MemoryReadByte(a,f)    (*((uint8_t*)(a)+(f))) // reads a byte at an address plus an offset in RAM
#define MemoryReadWord(a,f)    (*(uint16_t*)((uint8_t*)(a)+(f)))
#define MemoryReadLong(a,f)    (*(uint32_t*)((uint8_t*)(a)+(f)))
#define MemoryWriteByte(a,f,d) (*((uint8_t*)(a)+(f))=d)

// File

#define MASK_MAX_FILE_ENTRY_LIMIT_BITS 0x0F // This is used to indicate to the Cache_File_Entry function that a new sector needs to be loaded.

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

#define FOUND                 0    // dir entry match
#define NOT_FOUND             1    // dir entry not found
#define NO_MORE               2    // no more files found

#define DIR_NAMESIZE          8
#define DIR_EXTENSION         3
#define FILE_NAME_SIZE        (DIR_NAMESIZE+DIR_EXTENSION)

#define DIR_DEL               0xE5
#define DIR_EMPTY             0


// Volume with FAT filesystem
typedef struct
{
    partition_t* part;          // universal partition container (fsmanager)
    uint32_t fat;               // LBA of FAT
    uint32_t root;              // LBA of root directory
    uint32_t dataLBA;           // LBA of data area
    uint32_t maxroot;           // max entries in root dir
    uint32_t maxcls;            // max data clusters
    uint32_t fatsize;           // sectors in FAT
    uint8_t  fatcopy;           // copies of FAT
    uint8_t  SecPerClus;        // sectors per cluster
    uint32_t FatRootDirCluster;
    uint32_t reservedSectors;
    uint8_t  type;              // FAT12, 16 or 32 (for array access)
} FAT_partition_t;

// File
typedef struct
{
    FAT_partition_t* volume;  // volume containing the file
    file_t*  file;            // universal file container (fsmanager)
    uint32_t firstCluster;    // first cluster
    uint32_t currCluster;     // current cluster
    uint16_t sec;             // current sector in the current cluster
    uint16_t pos;             // current byte in the current sectors
    uint16_t time;            // last update time
    uint16_t date;            // last update date
    char     name[FILE_NAME_SIZE];
    uint32_t entry;           // file's entry position in its directory
    uint16_t chk;             // checksum = ~(entry+name[0])
    uint16_t attributes;      // file's attributes
    uint32_t dirfirstCluster; // first cluster of the file's directory
    uint32_t dircurrCluster;  // current cluster of the file's directory
} FAT_file_t;

// Entry in directory
typedef struct
{
    char     Name[DIR_NAMESIZE];
    char     Extension[DIR_EXTENSION];
    uint8_t  Attr;
    uint8_t  NTRes;
    uint8_t  CrtTimeTenth; // tenths of second portion
    uint16_t CrtTime;      // created
    uint16_t CrtDate;
    uint16_t LstAccDate;   // last access
    uint16_t FstClusHI;
    uint16_t WrtTime;      // last update
    uint16_t WrtDate;
    uint16_t FstClusLO;
    uint32_t FileSize;
} __attribute__((packed)) FAT_dirEntry_t;

typedef enum
{
    LOOK_FOR_EMPTY_ENTRY = 0,
    LOOK_FOR_MATCHING_ENTRY
} SEARCH_TYPE;

typedef struct
{
    char     jumpBoot[3];
    char     SysName[8];
    uint16_t bytesPerSector;
    uint8_t  SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t  FATcount;
    uint16_t MaxRootEntries;
    uint16_t TotalSectors16;
    uint8_t  MediaDescriptor;
    uint16_t FATsize16;
    uint16_t SectorsPerTrack;
    uint16_t HeadCount;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
} __attribute__((packed)) BPBbase_t;

typedef struct
{
    BPBbase_t base;
    uint8_t   driveNum;
    uint8_t   reserved;
    uint8_t   BootSig;
    uint32_t  VolID;
    char      VolLabel[11];
    char      FStype[8];
} __attribute__((packed)) BPB1216_t;

typedef struct
{
    BPBbase_t base;
    uint32_t  FATsize32;
    uint16_t  extFlags;
    uint16_t  version;
    uint32_t  rootCluster;
    uint16_t  FSinfo;
    uint16_t  BootSecCopy;
    uint8_t   reserved1[12];
    uint8_t   driveNum;
    uint8_t   reserved2;
    uint8_t   BootSig;
    uint32_t  VolID;
    char      VolLabel[11];
    char      FStype[8];
} __attribute__((packed)) BPB32_t;


// file handling
FS_ERROR FAT_fileErase(FAT_file_t* fileptr, uint32_t* fHandle, bool EraseClusters);
FS_ERROR FAT_searchFile(FAT_file_t* fileptrDest, char name[11], uint8_t cmd);
FS_ERROR FAT_fopen(file_t* file, bool create, bool open);
FS_ERROR FAT_fclose(file_t* file);
FS_ERROR FAT_fread(file_t* file, void* dest, size_t count);
FS_ERROR FAT_fwrite(file_t* file, const void* src, size_t count);
FS_ERROR FAT_fseek(file_t* file, long offset, SEEK_ORIGIN whence);
FS_ERROR FAT_remove(const char* fileName, partition_t* part);
FS_ERROR FAT_rename(const char* fileNameOld, const char* fileNameNew, partition_t* part);
FS_ERROR FAT_format(partition_t* part);
FS_ERROR FAT_pinstall(partition_t* part);

// folder handling
FS_ERROR FAT_folderAccess(folder_t* folder, folderAccess_t mode);
void     FAT_folderClose(folder_t* folder);

// analysis functions
void FAT_showDirectoryEntry(FAT_dirEntry_t* dir);


#endif
