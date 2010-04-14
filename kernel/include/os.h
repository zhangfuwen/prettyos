#ifndef OS_H
#define OS_H

#include "types.h"

/// Diagnosis-Output - activates prints to the screen about some details and memory use
// #define _DIAGNOSIS_

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

#endif
