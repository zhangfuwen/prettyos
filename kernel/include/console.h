#ifndef CONSOLE_H
#define CONSOLE_H

#include "keyboard.h"

typedef struct // Defines the User-Space of the display
{
    char* name;
    uint16_t* vidmem; // memory that stores the content of this console. Size is USER_LINES*COLUMNS
    //uint8_t SCROLL_BEGIN;
    uint8_t SCROLL_END;
    uint8_t  csr_x;
    uint8_t  csr_y;
    uint8_t  attrib;
    keyqueue_t KQ; // Buffer storing keyboard input
} console_t;

extern console_t* reachableConsoles[11]; // All accessible consoles: up to 10 subconsoles + main console
extern uint8_t displayedConsole;
static const uint8_t COLUMNS = 80;
static const uint8_t USER_LINES = 46;

void kernel_console_init();
void console_init(console_t* console, const char* name);
void console_exit(console_t* console);

void clear_console(uint8_t backcolor);
void settextcolor(uint8_t forecolor, uint8_t backcolor);
void putch(char c);
void puts(const char* text);
void printf (const char* args, ...);
void cprintf(const char* message, uint32_t line, uint8_t attribute, ...);
void scroll();
void set_cursor(uint8_t x, uint8_t y);
void update_cursor();
bool changeDisplayedConsole(uint8_t ID);
void setScrollField(uint8_t begin, uint8_t end);

#endif
