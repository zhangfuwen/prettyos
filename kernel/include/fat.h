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

#include "os.h"

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
    struct disk* disk;          // The disk on which the partition is
    uint8_t* buffer;            // buffer equal to one sector
    uint32_t sectorSize;        // byte per sector
    uint32_t firsts;            // LBA of 1st sector
    uint32_t fat;               // LBA of FAT
    uint32_t root;              // LBA of root directory
    uint32_t data;              // LBA of data area
    uint32_t maxroot;           // max entries in root dir
    uint32_t maxcls;            // max data clusters
    uint32_t fatsize;           // sectors in FAT
    uint8_t  fatcopy;           // copies of FAT
    uint8_t  SecPerClus;        // sectors per cluster
    uint8_t  type;              // FAT type (FAT16, FAT32)
    bool     mount;             // 0: not mounted  1: mounted
    uint32_t FatRootDirCluster;
    char     serialNumber[13];  // serial number for identification
} partition_t;

// File

typedef struct
{
    bool write;        // file is opened for writing
    bool read;         // file is opened for reading
    bool FileWriteEOF; // writing process has reached end of file
} FileFlags;

typedef struct
{
    partition_t* volume;    // volume containing the file
    uint32_t  cluster;      // first cluster
    uint32_t  ccls;         // current cluster
    uint16_t  sec;          // current sector in the current cluster
    uint16_t  pos;          // current byte in the current sectors
    uint32_t  seek;         // current byte in the file
    uint32_t  size;         // file size
    FileFlags Flags;        // write mode, EOF
    uint16_t time;          // last update time
    uint16_t date;          // last update date
    char     name[FILE_NAME_SIZE];
    uint32_t entry;         // file's entry position in its directory
    uint16_t chk;           // checksum = ~(entry+name[0])
    uint16_t attributes;    // file's attributes
    uint32_t dirclus;       // first cluster of the file's directory
    uint32_t dirccls;       // current cluster of the file's directory
} FILE;

typedef struct
{
    char DIR_Name[DIR_NAMESIZE];
    char DIR_Extension[DIR_EXTENSION];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;  // tenths of second portion
    uint16_t DIR_CrtTime;      // created
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;   // last access
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;      // last update
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} _FILEROOTDIRECTORYENTRY;
typedef _FILEROOTDIRECTORYENTRY* FILEROOTDIRECTORYENTRY;


/************************************************************************/
//
// file operations
//

typedef
enum
{
    LOOK_FOR_EMPTY_ENTRY = 0,
    LOOK_FOR_MATCHING_ENTRY
} SEARCH_TYPE;

// http://www.google.de/url?sa=t&source=web&ct=res&cd=2&ved=0CBwQFjAB&url=http%3A%2F%2Fwww.schmalzhaus.com%2FUBW32%2FFW%2FMicrochip_v2.5b%2FInclude%2FMDD%2520File%2520System%2FFSDefs.h&rct=j&q=%23define+CE_ERASE_FAIL&ei=X7L1S9XFN4f9OczL1dYI&usg=AFQjCNHZbqrCWoeIXQ2g6aZ2-jC1rfx7Ig
typedef enum
{
    CE_GOOD = 0,                    // No error
    CE_ERASE_FAIL,                  // An erase failed
    CE_NOT_PRESENT,                 // No device was present
    CE_NOT_FORMATTED,               // The disk is of an unsupported format
    CE_BAD_PARTITION,               // The boot record is bad

    CE_UNSUPPORTED_FS,              // The file system type is unsupported
    CE_INIT_ERROR,                  // An initialization error has occured
    CE_NOT_INIT,                    // An operation was performed on an uninitialized device
    CE_BAD_SECTOR_READ,             // A bad read of a sector occured
    CE_WRITE_ERROR,                 // Could not write to a sector

    CE_INVALID_CLUSTER,             // Invalid cluster value > maxcls
    CE_FILE_NOT_FOUND,              // Could not find the file on the device
    CE_DIR_NOT_FOUND,               // Could not find the directory
    CE_BAD_FILE,                    // File is corrupted
    CE_DONE,                        // No more files in this directory

    CE_COULD_NOT_GET_CLUSTER,       // Could not load/allocate next cluster in file
    CE_FILENAME_2_LONG,             // A specified file name is too long to use
    CE_FILENAME_EXISTS,             // A specified filename already exists on the device
    CE_INVALID_FILENAME,            // Invalid file name
    CE_DELETE_DIR,                  // The user tried to delete a directory with FSremove

    CE_DIR_FULL,                    // All root dir entry are taken
    CE_DISK_FULL,                   // All clusters in partition are taken
    CE_DIR_NOT_EMPTY,               // This directory is not empty yet, remove files before deleting
    CE_NONSUPPORTED_SIZE,           // The disk is too big to format as FAT16
    CE_WRITE_PROTECTED,             // Card is write protected

    CE_FILENOTOPENED,               // File not opened for the write
    CE_SEEK_ERROR,                  // File location could not be changed successfully
    CE_BADCACHEREAD,                // Bad cache read
    CE_CARDFAT32,                   // FAT 32 - card not supported
    CE_READONLY,                    // The file is read-only

    CE_WRITEONLY,                   // The file is write-only
    CE_INVALID_ARGUMENT,            // Invalid argument
    CE_TOO_MANY_FILES_OPEN,         // Too many files are already open
    CE_UNSUPPORTED_SECTOR_SIZE,     // Unsupported sector size

    CE_TIMEOUT,                     // Timout while trying to access

    CE_FAT_EOF = 60,                // Read try beyond FAT's EOF
    CE_EOF                          // EOF
} FS_ERROR;

// interface functions
FS_ERROR singleSectorRead(uint32_t sector_addr, uint8_t* buffer, partition_t* part);
FS_ERROR singleSectorWrite(uint32_t sector_addr, uint8_t* buffer, partition_t* part);

// file handling
FS_ERROR createFileEntry(FILE* fileptr, uint32_t *fHandle, uint8_t mode);
FS_ERROR searchFile(FILE* fileptrDest, FILE* fileptrTest, uint8_t cmd, uint8_t mode);
FILE* fopenFileName(const char* fileName, const char* mode);
FS_ERROR fopen(FILE* fileptr, uint32_t* fHandle, char type);
FS_ERROR fclose(FILE* fileptr);
FS_ERROR fread(FILE* fileptr, void* dest, uint32_t count);
int32_t fseek(FILE* fileptr, long offset, int whence); // return values should be adapted to FS_ERROR types
uint32_t fwrite(const void* ptr, uint32_t size, uint32_t n, FILE* stream);

// analysis functions
void showDirectoryEntry(FILEROOTDIRECTORYENTRY dir);

//additional functions
uint32_t checksum(char* ShortFileName);

#endif
