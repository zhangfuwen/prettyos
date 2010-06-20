#ifndef OS_H
#define OS_H

#include "types.h"

// These switches change the behavior of PrettyOS, useful for analyzing tasks:

/// #define _DIAGNOSIS_         // Diagnosis-Output - activates prints to the screen about some details and memory use
/// #define _USB_DIAGNOSIS_     // only as transition state during implementation of USB 2.0 transfers
/// #define _FAT_DIAGNOSIS_     // only as transition state during implementation of FAT 12/16/32
/// #define _FAT_READ_WRITE_TO_SECTOR_DIAGNOSIS_ // only as transition state during implementation of FAT 12/16/32
/// #define _TASKING_DIAGNOSIS_ // Provides optional output about tasking and scheduler
#define _FLOPPY_DIAGNOSIS_  // Provides optional information about the floppy(-motor)
/// #define _SOUND_             // Sound-Messages  - deactivation makes sense during development, because of better boot-time

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
    va_start (ap, args);
    vprintf(args, ap);
    if(color != 0x00)
    {
        textColor(0x0F);
    }
    #endif
}

// keyboard map
/// #define KEYMAP_US // US keyboard
#define KEYMAP_GER    // German keyboard


// PrettyOS Version string
extern const char* version;

// Informations about the system
extern system_t system;


// Declared here, because a header would be a waste of space
// elf.c
bool elf_exec(const void* elf_file, uint32_t elf_file_size, const char* programName);

#endif
