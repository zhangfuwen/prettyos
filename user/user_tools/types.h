#ifndef TYPES_H
#define TYPES_H

typedef unsigned int        size_t;
typedef unsigned long long  uint64_t;
typedef unsigned long       uint32_t;
typedef unsigned short      uint16_t;
typedef unsigned char       uint8_t;
typedef signed long long    int64_t;
typedef signed long         int32_t;
typedef signed short        int16_t;
typedef signed char         int8_t;
typedef uint32_t            uintptr_t;
#ifndef __bool_true_false_are_defined
  typedef _Bool             bool;
  #define true  1
  #define false 0
  #define __bool_true_false_are_defined 1
#endif

#define NULL (void*)0

typedef __builtin_va_list va_list;
#define va_start(ap, X)    __builtin_va_start(ap, X)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dest,src)  __builtin_va_copy(dest,src)


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

    CE_FAT_EOF = 60,                // Read try beyond FAT's EOF
    CE_EOF                          // EOF
} FS_ERROR;

typedef enum
{
	SHUTDOWN, REBOOT
} SYSTEM_CONTROL;

#endif
