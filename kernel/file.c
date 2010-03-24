#include "file.h"
#include "flpydsk.h"
#include "fat12.h"
#include "task.h"
#include "kheap.h"

int32_t initCache() /// floppy
{
    int32_t retVal0 = flpydsk_read_ia(0,track0,TRACK);
    int32_t retVal1 = flpydsk_read_ia(1,track1,TRACK);

    if (retVal0 || retVal1)
        return -1;
    else
        return 0;
}

int32_t flpydsk_load(const char* name, const char* ext) /// load file <--- TODO: make it general!
{
    int32_t retVal;
    struct file f;
    uint32_t firstCluster = 0;

    flpydsk_control_motor(true);
    retVal = initCache();
    if (retVal)
    {
        settextcolor(12,0);
        printf("track0 & track1 read error.\n");
        settextcolor(2,0);
    }

    printf("Load and execute "); settextcolor(14,0); printf("-->%s.%s<--",name,ext);
    settextcolor(2,0); printf(" from floppy disk\n");

    firstCluster = search_file_first_cluster(name,ext,&f); // now working with cache
    if (firstCluster==0)
    {
        printf("file not found in root directory\n");
        flpydsk_control_motor(false);
        return -1;
    }

    /// ********************************** File memory prepared **************************************///

    uint8_t* file = malloc(f.size,PAGESIZE); /// TODO: free allocated memory, if program is finished
    printf("FileSize: %d Byte, 1st Cluster: %d, Memory: %X\n",f.size, f.firstCluster, file);
    sleepSeconds(3); // show screen output

    ///************** FAT ********************///
    printf("\nFAT1 parsed 12-bit-wise: ab cd ef --> dab efc\n");

    /// TODO: read only entries which are necessary for file_ia
    ///       combine reading FAT entry and data sector
    for (uint32_t i=0;i<FATMAXINDEX;i++)
    {
        read_fat(&fat_entry[i], i, FAT1_SEC, track0);
    }
    // load file to memory
    retVal = file_ia(fat_entry,firstCluster,file); // read sectors of file
    ///************** FAT ********************///

    #ifdef _DIAGNOSIS_
        printf("\nFile content (start of first 5 clusters): ");
        printf("\n1st sector:\n"); for (uint16_t i=   0;i<  20;i++) {printf("%y ",file[i]);}
        printf("\n2nd sector:\n"); for (uint16_t i= 512;i< 532;i++) {printf("%y ",file[i]);}
        printf("\n3rd sector:\n"); for (uint16_t i=1024;i<1044;i++) {printf("%y ",file[i]);}
        printf("\n4th sector:\n"); for (uint16_t i=1536;i<1556;i++) {printf("%y ",file[i]);}
        printf("\n5th sector:\n"); for (uint16_t i=2048;i<2068;i++) {printf("%y ",file[i]);}
        printf("\n\n");
    #endif

    if (!retVal)
    {
        /// START TASK AND INCREASE TASKCOUNTER
        if ( elf_exec( file, f.size ) ) // execute loaded file
        {
            userTaskCounter++;         // an additional user-program has been started
            free(file);
        }
        else
        {
            settextcolor(14,0);
            printf("File has unknown format and will not be executed.\n");
            settextcolor(15,0);
            // other actions?
            free(file); // still needed in kernel?
        }
    }
    else if (retVal==-1)
    {
        printf("file was not executed due to FAT error.");
    }    
    flpydsk_control_motor(false);
    sleepSeconds(3); // show screen output
    return 0;
}

int32_t flpydsk_write(const char* name, const char* ext, void* memory, uint32_t size)
{
    printf("file save routine not yet implemented \n");

    

    // how many floppy disc sectors are needed?
    
    uint32_t neededSectors=0;
    if ( size%512 == 0)
    {
        neededSectors = (size/512);
    }
    else
    {
        neededSectors = (size/512)+1;
    }

    // TODO:
    // search "neededSectors" free sectors
    // write name, extension, first cluster into root directory
    // define and write FAT chain to FAT1 and FAT2
    // divide file in memory into sectors sizes; file: address and size needed
    // write sectors from memory to floppy disk
    // free memory, if necessary
    return 0;
}

int32_t file_ia(int32_t* fatEntry, uint32_t firstCluster, void* fileData) /// load file
{
    uint8_t a[512];
    uint32_t sectornumber;
    uint32_t i, pos;  // i for FAT-index, pos for data position
    const uint32_t ADD = 31;

    // copy first cluster
    sectornumber = firstCluster+ADD;
    printf("\n\n1st sector: %d\n",sectornumber);

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;
    while ( flpydsk_read_ia(sectornumber,a,SECTOR) != 0 )
    {
        retVal = -1;
        timeout--;
        printf("error read_sector. attempts left: %d\n",timeout);
        if (timeout<=0)
        {
            printf("timeout\n");
            break;
        }
    }
    if (retVal==0)
    {
        /// printf("success read_sector.\n");
    }

    memcpy( (void*)fileData, (void*)a, 512);

    /// find second cluster and chain in fat /// FAT12 specific
    pos=0;
    i = firstCluster;
    while (fatEntry[i]!=0xFFF)
    {
        printf("\ni: %d FAT-entry: %x\t",i,fatEntry[i]);
        if ( (fatEntry[i]<3) || (fatEntry[i]>MAX_BLOCK))
        {
            printf("FAT-error.\n");
            return -1;
        }

        // copy data from chain
        pos++;
        sectornumber = fatEntry[i]+ADD;
        printf("sector: %d\t",sectornumber);

        timeout = 2; // limit
        retVal  = 0;
        while ( flpydsk_read_ia(sectornumber,a,SECTOR) != 0 )
        {
            retVal = -1;
            timeout--;
            printf("error read_sector. attempts left: %d\n",timeout);
            if (timeout<=0)
            {
                printf("timeout\n");
                break;
            }
        }
        if (retVal==0)
        {
            /// printf("success read_sector.\n");
        }

        memcpy( (void*)(fileData+pos*512), (void*)a, 512);

        // search next cluster of the fileData
        i = fatEntry[i];
    }
    printf("\n");
    return 0;
}


