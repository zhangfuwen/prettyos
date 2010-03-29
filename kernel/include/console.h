#ifndef CONSOLE_H
#define CONSOLE_H

#include "os.h"

typedef struct {
    uint8_t buffer[KQSIZE];  // circular queue buffer
    uint8_t* pHead;          // pointer to the head of valid data
    uint8_t* pTail;          // pointer to the tail of valid data
    uint32_t count_read;     // number of data read from queue buffer
    uint32_t count_write;    // number of data put into queue buffer
} keyqueue_t;

typedef struct { // Defines the User-Space of the display
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


void console_init(console_t* console, const char* name);
void console_exit(console_t* console);

bool changeDisplayedConsole(uint8_t ID);
void setScrollField(uint8_t begin, uint8_t end);

#endif
