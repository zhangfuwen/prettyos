#ifndef OS_H
#define OS_H

#include "types.h"


// Enable/Disable features (Should be enabled per default)
#define _BOOTSCREEN_           // Enables the bootscreen displayed at startup
#define _VIDEOTEST_            // Enables the vbe test at startup
#define _PCI_VEND_PROD_LIST_   // http://www.pcidatabase.com/pci_c_header.php - Increases the size of the kernel significantly
#define _SERIAL_LOG_           // Enables log information over the COM-Ports

// Enable/Disable diagnosis output (Should be disabled per default)
/// #define _DIAGNOSIS_            // Diagnosis-Output - activates prints to the screen about some details and memory use
/// #define _PAGING_DIAGNOSIS_     // Diagnosis-Output about PD, PT etc.
/// #define _MALLOC_FREE_          // shows information about malloc/free and heap expansion
/// #define _MEMLEAK_FIND_         // Provides a counter of all (successful) malloc and free calls showing memory leaks
/// #define _EHCI_DIAGNOSIS_       // Debug EHCI
/// #define _USB_DIAGNOSIS_        // Debug USB 2.0 transfers
/// #define _FAT_DIAGNOSIS_        // only as transition state during implementation of FAT 12/16/32
/// #define _DEVMGR_DIAGNOSIS_     // e.g. sectorRead, sectorWrite
/// #define _READCACHE_DIAGNOSIS_  // read cache logger
/// #define _TASKING_DIAGNOSIS_    // Provides output about tasking and scheduler
/// #define _FLOPPY_DIAGNOSIS_     // Provides information about the floppy(-motor)
/// #define _VM_DIAGNOSIS_         // Provides information about the vm86 task, but critical
/// #define _RAMDISK_DIAGNOSIS_    // Enables additional information about the ramdisk
/// #define _NETWORK_DATA_         // Enables additional information about networking packets
/// #define _NETWORK_DIAGNOSIS_    // Enables additional information about the network adapters
/// #define _ARP_DEBUG_            // Enables additional information about ARP
/// #define _DHCP_DEBUG_           // Enables additional information about DHCP
/// #define _ICMP_DEBUG_           // Enables analysis of ICMP information besides echo request/response
/// #define _UDP_DEBUG_            // Enables additional information about UDP
/// #define _TCP_DEBUG_            // Enables additional information about TCP
/// #define _NETBIOS_DEBUG_        // Enables NetBIOS packet analysis
/// #define _VBE_DEBUG_            // Provides additional output of the VBE driver


extern const char* const version; // PrettyOS Version string
extern system_t system;           // Informations about the operating system
extern struct todoList* kernel_idleTasks;


void textColor(uint8_t color);
size_t vprintf(const char*, va_list);

static inline void kdebug(uint8_t color, const char* args, ...)
{
    #ifdef _DIAGNOSIS_
    if(color != 0x00)
    {
        textColor(color);
    }
    va_list ap;
    va_start(ap, args);
    vprintf(args, ap);
    va_end(ap);
    if(color != 0x00)
    {
        textColor(0x0F);
    }
    #endif
}


#endif
