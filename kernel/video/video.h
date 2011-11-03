#ifndef VIDEO_H
#define VIDEO_H

#include "storage/devicemanager.h" // HACK


#define COLUMNS 80
#define LINES   50
#define USER_BEGIN 2 // Reserving  Titlebar + Separation
#define USER_END   48 // Reserving Statusbar + Separation

//CRTC Address Register and the CRTC Data Register
#define CRTC_ADDR_REGISTER           0x3D4
#define CRTC_DATA_REGISTER           0x3D5

// Cursor Location Registers
#define CURSOR_LOCATION_HI_REGISTER  0xE
#define CURSOR_LOCATION_LO_REGISTER  0xF

#define VIDEORAM  0xB8000

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
extern bool autoRefresh;

void vga_install();
void vga_setPixel(uint8_t x, uint8_t y, uint16_t value);
void refreshUserScreen();
void vga_clearScreen();
void kprintf(const char* message, uint32_t line, uint8_t attribute, ...);
void vga_updateCursor();
uint8_t AsciiToCP437(uint8_t ascii);
void writeInfo(uint8_t line, const char* content, ...);
void saveScreenshot();
void takeScreenshot();
void mt_screenshot();


#endif
