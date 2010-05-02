#ifndef OS_H
#define OS_H

#include "types.h"

// Switches changing the behaviour of PrettyOS
/// #define _DIAGNOSIS_     // Diagnosis-Output - activates prints to the screen about some details and memory use
/// #define _USB_DIAGNOSIS_ // only as transition state during implementation of USB 2.0 transfers
/// #define _SOUND_         // Sound-Messages - deactivated per default, because they increase the boot-time

void settextcolor(uint8_t, uint8_t);
void printf(const char*, ...);
static inline void kdebug(int8_t color, const char* args, ...)
{
    #ifdef _DIAGNOSIS_
    if(color != -1) 
    {
        settextcolor(color, 0);
    }
    va_list ap;
    va_start (ap, args);
    vprintf(args, ap);
    if(color != -1) 
    {
        settextcolor(15, 0);
    }
    #endif
}

// keyboard map
/// #define KEYMAP_US // US keyboard
#define KEYMAP_GER   // German keyboard


// PrettyOS Version string
extern const char* version;

// operatings system common data area
extern oda_t ODA;


// Declared here, because a header would be a waste of space
// fpu.c
void set_fpu_cw(const uint16_t ctrlword);
void fpu_install();
// elf.c
bool elf_exec(const void* elf_file, uint32_t elf_file_size, const char* programName);

#endif
