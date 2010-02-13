


#ifndef _FAT12_H
#define _FAT12_H

#include "os.h"

// Error
#define E_DISK -1

// Attributes in Root Directory
#define ATTR_READ_ONLY       0x01
#define ATTR_HIDDEN          0x02
#define ATTR_SYSTEM          0x04
#define ATTR_VOLUME_ID       0x08
#define ATTR_DIRECTORY       0x10
#define ATTR_ARCHIVE         0x20
#define ATTR_LONG_NAME       0x0F
#define ATTR_LONG_NAME_MASK  0x3F

// Start Sector Values
#define BOOT_SEC      0
#define FAT1_SEC      1
#define FAT2_SEC     10
#define ROOT_SEC     19
#define DATA_SEC     33
#define MAX_BLOCK  2849
#define MAX_SECTOR 2880

// Numbers
#define DIR_ENTRIES 16
#define MAX_DIR     10
#define MAX_FILE    10
#define FATMAXINDEX 300 ///TEST

struct boot_sector
{
    char    jumpBoot[3];
    char    SysName[8];
    int32_t charsPerSector;
    char    SectorsPerCluster;
    int32_t ReservedSectors;
    char    FATcount;
    int32_t MaxRootEntries;
    int32_t TotalSectors1;
    char    MediaDescriptor;
    int32_t SectorsPerFAT;
    int32_t SectorsPerTrack;
    int32_t HeadCount;
    int32_t HiddenSectors;
    int32_t TotalSectors2;
    char    DriveNumber;
    char    Reserved1;
    char    ExtBootSignature;
    int32_t VolumeSerial;
    char    VolumeLabel[11];
    char    Reserved2[8];
};

struct dir_entry
{
  char    Filename[8];
  char    Extension[3];
  char    Attributes;
  char    NTRes;
  char    CrtTimeTenth;
  int32_t CrtTime;
  int32_t CrtDate;
  int32_t LstAccDate;
  int32_t FstClusHI;
  int32_t WrtTime;
  int32_t WrtDate;
  int32_t FstClusLO;
  int32_t FileSize;
 };

 struct file
 {
   uint32_t firstCluster;
   uint32_t size;
 };


/*************** functions ******************/

int32_t flpydsk_read_directory();

int32_t flpydsk_prepare_boot_sector(struct boot_sector *bs);
int32_t flpydsk_format(char* vlab); //VolumeLabel

int32_t flpydsk_write_sector_ia( int32_t i, void* a);
int32_t flpydsk_write_track_ia ( int32_t track, void* trackbuffer);
int32_t flpydsk_read_sector_ia ( int32_t i, void* a);
int32_t flpydsk_read_track_ia  ( int32_t track, void* trackbuffer);

int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* file);
int32_t flpydsk_load(char* name, char* ext);
int32_t read_fat(int32_t* fat_entry, int32_t index, int32_t st_sec, uint8_t* buffer);

void    parse_dir(uint8_t* a, int32_t in, struct dir_entry* rs);
void    print_dir(struct dir_entry* rs);
int32_t read_dir(struct dir_entry* rs, int32_t in, int32_t st_sec, bool flag);
uint32_t search_file_first_cluster(char* name, char* ext, struct file* f);
void parse_fat(int32_t* fat_entry, int32_t fat1, int32_t fat2, int32_t in);



//int32_t flpydsk_write_dir(struct dir_entry* rs, int32_t in, int32_t st_sec);


#endif
