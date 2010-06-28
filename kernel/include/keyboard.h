#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "os.h"

#define KQSIZE 20 // size of key queue

typedef enum {/*VK_LBUTTON=0x01, VK_RBUTTON, VK_CANCEL, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2,*/  
              VK_BACK=0x08,    VK_TAB,     
              VK_CLEAR=0x0C,   VK_RETURN,
              VK_SHIFT=0x10,   VK_CONTROL, VK_MENU, VK_PAUSE, VK_CAPITAL, VK_KANA,
              VK_JUNJA=0x17,   VK_FINAL,   VK_KANJI,  
              VK_ESCAPE=0x1B,  VK_CONVERT, VK_NONCONVERT, VK_ACCEPT, VK_MODECHANGE,
              VK_SPACE, VK_PRIOR, VK_NEXT, VK_END, VK_HOME, VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, VK_SELECT, VK_PRINT, VK_EXECUTE, VK_SNAPSHOT, VK_INSERT, VK_DELETE, VK_HELP,
              VK_0, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
              VK_A=0x41, VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N, VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z, 
              VK_LWIN=0x5B, VK_RWIN, VK_APPS,
              VK_SLEEP=0x5F, VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, 
              VK_MULTIPLY=0x6A, VK_ADD, VK_SEPARATOR, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,
              VK_F1=0x70, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, 
              VK_NUMLOCK=0x90, VK_SCROLL, 
              VK_LSHIFT=0xA0, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU
} VK;

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
uint8_t keyboard_getChar();
bool keyPressed(VK Key); 

#endif
