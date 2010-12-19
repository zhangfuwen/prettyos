#ifndef _FAT12_H
#define _FAT12_H


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

// Numbers
#define ROOT_DIR_ENTRIES 224


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
} __attribute__((packed));

int32_t flpydsk_read_directory();
int32_t flpydsk_format(char* vlab); // VolumeLabel

#endif
