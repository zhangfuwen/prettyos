#ifndef CONSOLE_H
#define CONSOLE_H

#include "video.h"
#include "synchronisation.h"
#include "list.h"


#define KERNELCONSOLE_ID 0

#define COLUMNS 80
#define USER_LINES 46


typedef struct // Defines the User-Space of the display
{
    uint8_t    ID; // Number of the console. Used to access it via the reachableConsoles array
    char*      name;
    bool       showInfobar;
    uint8_t    scrollBegin;
    uint8_t    scrollEnd;
    position_t cursor;
    mutex_t*   mutex;
    list_t*    tasks;
    uint16_t   vidmem[USER_LINES*COLUMNS]; // Memory that stores the content of this console. Size is USER_LINES*COLUMNS
} console_t;


extern console_t* reachableConsoles[11]; // All accessible consoles: up to 10 subconsoles + main console
extern volatile console_t* console_displayed;
extern volatile console_t* console_current;
extern console_t kernelConsole;


void kernel_console_init();
void console_init(console_t* console, const char* name);
void console_exit(console_t* console);
bool console_display(uint8_t ID);

void console_clear(uint8_t backcolor);
void textColor(uint8_t color); // bit 4-7: background; bit 1-3: foreground
uint8_t getTextColor();
void showInfobar(bool show);
void setScrollField(uint8_t begin, uint8_t end);
void console_setPixel(uint8_t x, uint8_t y, uint16_t value);
void putch(char c);
void puts(const char* text);
size_t printf (const char* args, ...);
size_t vprintf(const char* args, va_list ap);
size_t cprintf(const char* message, uint32_t line, uint8_t attribute, ...);
void setCursor(position_t pos);
void getCursor(position_t* pos);


#endif
