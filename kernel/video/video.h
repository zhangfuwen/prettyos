#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"
#include "storage/devicemanager.h" // HACK


typedef struct
{
    uint16_t x, y;
} position_t;

typedef enum
{
    VM_TEXT, VM_VBE
} VIDEOMODES;

// These alias for colors are mandatory
#define ERROR          LIGHT_RED
#define SUCCESS        GREEN
#define HEADLINE       CYAN
#define TABLE_HEADING  LIGHT_GRAY
#define DATA           BROWN
#define IMPORTANT      YELLOW
#define TEXT           WHITE
#define FOOTNOTE       LIGHT_RED
#define TITLEBAR       LIGHT_RED

enum COLORS
{
    BLACK, BLUE,        GREEN,       CYAN,       RED,       MAGENTA,       BROWN,  LIGHT_GRAY,
    GRAY,  LIGHT_BLUE,  LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED, LIGHT_MAGENTA, YELLOW, WHITE
};


extern VIDEOMODES videomode;


void video_install();
void video_setPixel(uint8_t x, uint8_t y, uint16_t value);
void refreshUserScreen();
void clear_screen();
void kprintf(const char* message, uint32_t line, uint8_t attribute, ...);
uint8_t AsciiToCP437(uint8_t ascii);
void writeInfo(uint8_t line, const char* content, ...);
void saveScreenshot();
void takeScreenshot();
void mt_screenshot();
void video_updateCursor();


#endif
