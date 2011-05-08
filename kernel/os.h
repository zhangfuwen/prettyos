#ifndef OS_H
#define OS_H

#include "types.h"

// These switches change the behavior of PrettyOS, useful for analyzing tasks:

/// #define _DIAGNOSIS_            // Diagnosis-Output - activates prints to the screen about some details and memory use
/// #define _MALLOC_FREE_          // shows information about malloc/free and heap expansion
/// #define _MEMLEAK_FIND_         // Provides a counter of all (successful) malloc and free calls showing memory leaks
/// #define _USB_DIAGNOSIS_        // only as transition state during implementation of USB 2.0 transfers
/// #define _FAT_DIAGNOSIS_        // only as transition state during implementation of FAT 12/16/32
/// #define _DEVMGR_DIAGNOSIS_     // e.g. sectorRead, sectorWrite
/// #define _READCACHE_DIAGNOSIS_  // read cache logger
/// #define _TASKING_DIAGNOSIS_    // Provides output about tasking and scheduler
/// #define _FLOPPY_DIAGNOSIS_     // Provides information about the floppy(-motor)
/// #define _VM_DIAGNOSIS_         // Provides information about the vm86 task, but critical
/// #define _SERIAL_LOG_           // Enables Log information over the COM-Ports
/// #define _RAMDISK_DIAGNOSIS_    // Enables additional information about the ramdisk
#define _BEEP_                 // Enables sound with the pc-speaker in PrettyOS that is used in the bootscreen. Disabled per default.
#define _PCI_VEND_PROD_LIST_   // http://www.pcidatabase.com/pci_c_header.php - Increases the size of the kernel significantly

/// #define KEYMAP US     // US keyboard
#define KEYMAP GER    // German keyboard

extern const char* const version; // PrettyOS Version string
extern system_t system;           // Informations about the operating system
extern struct todoList* delayedInitTasks; // HACK (see ckernel.c)


void textColor(uint8_t color);
void vprintf(const char*, va_list);

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
