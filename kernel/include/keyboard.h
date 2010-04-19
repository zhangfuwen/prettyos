#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "os.h"

#define KQSIZE    20 // size of key queue

typedef struct
{
    uint8_t buffer[KQSIZE];  // circular queue buffer
    uint8_t* pHead;          // pointer to the head of valid data
    uint8_t* pTail;          // pointer to the tail of valid data
    uint32_t count_read;     // number of data read from queue buffer
    uint32_t count_write;    // number of data put into queue buffer
} keyqueue_t;

void    keyboard_install();
uint8_t FetchAndAnalyzeScancode();
uint8_t ScanToASCII();
void    keyboard_handler(registers_t* r);
uint8_t checkKQ_and_return_char();
bool    testch();

#endif
