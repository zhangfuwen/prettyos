#include "file.h"
#include "flpydsk.h"
#include "fat12.h"
#include "task.h"
#include "kheap.h"

int32_t initCache() /// floppy
{
    int32_t retVal0 = flpydsk_read_ia(0,cache0,TRACK);
    int32_t retVal1 = flpydsk_read_ia(1,cache1,TRACK);

    if(retVal0 || retVal1)
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
    if(retVal)
    {
        settextcolor(12,0);
        printformat("track0 & track1 read error.\n");
        settextcolor(2,0);
    }

    printformat("Load and execute "); settextcolor(14,0); printformat("-->%s.%s<--",name,ext);
    settextcolor(2,0); printformat(" from floppy disk\n");

    firstCluster = search_file_first_cluster(name,ext,&f); // now working with cache
    if(firstCluster==0)
    {
        printformat("file not found in root directory\n");
        return -1;
    }

    /// ********************************** File memory prepared **************************************///

    uint8_t* file = malloc(f.size,PAGESIZE); /// TODO: free allocated memory, if program is finished
    printformat("FileSize: %d Byte, 1st Cluster: %d, Memory: %X\n",f.size, f.firstCluster, file);
    sleepSeconds(3); // show screen output

    ///************** FAT ********************///
    printformat("\nFAT1 parsed 12-bit-wise: ab cd ef --> dab efc\n");
    memcpy((void*)track0, (void*)cache0, 0x2400); /// TODO: avoid cache0 --> track0
    /// TODO: read only entries which are necessary for file_ia
    ///       combine reading FAT entry and data sector
    for(uint32_t i=0;i<FATMAXINDEX;i++)
    {
        read_fat(&fat_entry[i], i, FAT1_SEC, track0);
    }
    // load file to memory
    retVal = file_ia(fat_entry,firstCluster,file); // read sectors of file
    ///************** FAT ********************///

    #ifdef _DIAGNOSIS_
        printformat("\nFile content (start of first 5 clusters): ");
        printformat("\n1st sector:\n"); for(uint16_t i=   0;i<  20;i++) {printformat("%y ",file[i]);}
        printformat("\n2nd sector:\n"); for(uint16_t i= 512;i< 532;i++) {printformat("%y ",file[i]);}
        printformat("\n3rd sector:\n"); for(uint16_t i=1024;i<1044;i++) {printformat("%y ",file[i]);}
        printformat("\n4th sector:\n"); for(uint16_t i=1536;i<1556;i++) {printformat("%y ",file[i]);}
        printformat("\n5th sector:\n"); for(uint16_t i=2048;i<2068;i++) {printformat("%y ",file[i]);}
        printformat("\n\n");
    #endif

    if(!retVal)
    {
        /// START TASK AND INCREASE TASKCOUNTER
        if( elf_exec( file, f.size ) ) // execute loaded file
        {
            userTaskCounter++;         // an additional user-program has been started
            free(file);
        }
        else
        {
            settextcolor(14,0);
            printformat("File has unknown format and will not be executed.\n");
            settextcolor(15,0);
            // other actions?
            free(file); // still needed in kernel?
        }
    }
    else if(retVal==-1)
    {
        printformat("file was not executed due to FAT error.");
    }
    printformat("\n\n");
    flpydsk_control_motor(false);
    sleepSeconds(3); // show screen output
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
    printformat("\n\n1st sector: %d\n",sectornumber);

    uint32_t timeout = 2; // limit
    int32_t  retVal  = 0;
    while( flpydsk_read_ia(sectornumber,a,SECTOR) != 0 )
    {
        retVal = -1;
        timeout--;
        printformat("error read_sector. attempts left: %d\n",timeout);
	    if(timeout<=0)
	    {
	        printformat("timeout\n");
	        break;
	    }
    }
    if(retVal==0)
    {
        /// printformat("success read_sector.\n");
    }

    memcpy( (void*)fileData, (void*)a, 512);

    /// find second cluster and chain in fat /// FAT12 specific
    pos=0;
    i = firstCluster;
    while(fatEntry[i]!=0xFFF)
    {
        printformat("\ni: %d FAT-entry: %x\t",i,fatEntry[i]);
        if( (fatEntry[i]<3) || (fatEntry[i]>MAX_BLOCK))
        {
            printformat("FAT-error.\n");
            return -1;
        }

        // copy data from chain
        pos++;
        sectornumber = fatEntry[i]+ADD;
        printformat("sector: %d\t",sectornumber);

        timeout = 2; // limit
        retVal  = 0;
        while( flpydsk_read_ia(sectornumber,a,SECTOR) != 0 )
        {
            retVal = -1;
            timeout--;
            printformat("error read_sector. attempts left: %d\n",timeout);
	        if(timeout<=0)
	        {
	            printformat("timeout\n");
	            break;
	        }
        }
        if(retVal==0)
        {
            /// printformat("success read_sector.\n");
        }

        memcpy( (void*)(fileData+pos*512), (void*)a, 512);

        // search next cluster of the fileData
        i = fatEntry[i];
    }
    printformat("\n");
    return 0;
}


