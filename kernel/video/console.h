#ifndef CONSOLE_H
#define CONSOLE_H

#include "keyboard.h"
#include "video.h"

#define KERNELCONSOLE_ID 10

#define COLUMNS 80
#define USER_LINES 46

typedef struct // Defines the User-Space of the display
{
    char* name;
    bool showInfobar;
    uint8_t SCROLL_BEGIN; // TODO: Make use of it
    uint8_t SCROLL_END;
    position_t cursor;
    keyqueue_t KQ;
    struct semaphore* sp;
    uint16_t vidmem[USER_LINES*COLUMNS]; // memory that stores the content of this console. Size is USER_LINES*COLUMNS
} console_t; // ATTENTION: Do not change the order of the members without changing the order of initialization (console.c)

extern console_t* reachableConsoles[11]; // All accessible consoles: up to 10 subconsoles + main console
extern uint8_t displayedConsole;
extern console_t* currentConsole;

void kernel_console_init();
void console_init(console_t* console, const char* name);
void console_exit(console_t* console);

void clear_console(uint8_t backcolor);
void textColor(uint8_t color); // bit 4-7: background; bit 1-3: foreground
uint8_t getTextColor();
void showInfobar(bool show);
void setScrollField(uint8_t begin, uint8_t end);
void putch(char c);
void puts(const char* text);
void printf (const char* args, ...);
void vprintf(const char* args, va_list ap);
void cprintf(const char* message, uint32_t line, uint8_t attribute, ...);
void scroll();
void setCursor(position_t pos);
position_t getCursor();
void update_cursor();
bool changeDisplayedConsole(uint8_t ID);

#endif
