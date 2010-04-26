#ifndef OS_H
#define OS_H

#include "types.h"

// Switches changing the behaviour of PrettyOS
/// #define _DIAGNOSIS_     // Diagnosis-Output - activates prints to the screen about some details and memory use
/// #define _USB_DIAGNOSIS_ // only as transition state during implementation of USB 2.0 transfers
/// #define _SOUND_         // Sound-Messages - activated per default, but they increase the boot-time.

void settextcolor(uint8_t, uint8_t);
void printf(const char*, ...);
static inline void kdebug(int8_t color, ...)
{
    #ifdef _DIAGNOSIS_
    if(color != -1) {
        settextcolor(color, 0);
    }
    va_list args;
    va_start (args, color);
    printf(args);
    if(color != -1) {
        settextcolor(15, 0);
    }
    #endif
}

/// keyboard map
// #define KEYMAP_US // US keyboard
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
// console.h
extern void settextcolor(uint8_t forecolor, uint8_t backcolor);
extern void printf (const char* args, ...);
// timer.h
extern void sleepSeconds (uint32_t seconds);

#endif
