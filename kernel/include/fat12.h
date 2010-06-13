#ifndef _FAT12_H
#define _FAT12_H

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
#define ROOT_DIR_ENTRIES 224
#define MAX_DIR     10
#define MAX_FILE    10
#define FATMAXINDEX MAX_BLOCK ///TEST

// FAT entries
#define LAST_ENTRY_IN_FAT_CHAIN 0xFFF
#define BAD_CLUSTER             0xFF7


struct boot_sector
{
    char     jumpBoot[3];
    char     SysName[8];
    uint16_t charsPerSector;
    uint8_t  SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t  FATcount;
    uint16_t MaxRootEntries;
    uint16_t TotalSectors1;
    uint8_t  MediaDescriptor;
    uint16_t SectorsPerFAT;
    uint16_t SectorsPerTrack;
    uint16_t HeadCount;
    uint32_t HiddenSectors;
    uint32_t TotalSectors2;
    uint8_t  DriveNumber;
    uint8_t  Reserved1;
    uint8_t  ExtBootSignature;
    uint32_t VolumeSerial;
    char     VolumeLabel[11];
    char     Reserved2[8];
}__attribute__((packed));

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
 }__attribute__((packed));

 struct file
 {
   uint32_t firstCluster;
   uint32_t size;
 }__attribute__((packed));


// cache memory for tracks 0 and 1
extern uint8_t track0[9216], track1[9216];

// how to handle memory for the file?
extern int32_t fat_entry[FATMAXINDEX];

int32_t flpydsk_read_directory();
int32_t flpydsk_format(char* vlab); // VolumeLabel
int32_t read_fat(int32_t* fat_entry, int32_t index, int32_t st_sec, uint8_t* buffer);
int32_t write_fat(int32_t fat, int32_t index, int32_t st_sec, uint8_t* buffer);
void parse_fat(int32_t* fat_entry, int32_t fat1, int32_t fat2, int32_t in);

#endif
