#ifndef CONSOLE_H
#define CONSOLE_H

#include "os.h"

typedef struct { // Defines the User-Space of the display
    char* name;
    uint16_t* vidmem; // memory that stores the content of this console. Size is USER_LINES*COLUMNS
    uint8_t  csr_x;
    uint8_t  csr_y;
    uint8_t  attrib;
} console_t;

extern console_t* reachableConsoles[11]; // All accessible consoles: up to 10 subconsoles + mainconsolé
extern uint8_t displayedConsole;

void console_init(volatile console_t* console, const char* name);
void console_exit(volatile console_t* console);
bool changeDisplayedConsole(uint8_t ID);

#endif
