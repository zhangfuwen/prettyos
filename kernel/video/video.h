#ifndef VIDEO_H
#define VIDEO_H

#include "os.h"
#include "storage/devicemanager.h" // HACK

typedef struct {
    uint16_t x, y;
} position_t;

typedef enum {
    VM_TEXT, VM_VBE
} VIDEOMODES;

extern VIDEOMODES videomode;

extern diskType_t* ScreenDest; // HACK

void refreshUserScreen();
void clear_screen();
void kprintf(const char* message, uint32_t line, uint8_t attribute, ...);
uint8_t AsciiToCP437(uint8_t ascii);
void writeInfo(uint8_t line, char* content, ...);
void screenshot();
void mt_screenshot();

#endif
