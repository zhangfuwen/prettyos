#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include "vbe.h"

#define HWND_DESKTOP 0
#define MAX_WINDOWS 256

typedef struct
{
    char *name;
    uint16_t id;
    uint16_t parentid;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    BGRA_t* data;
    uint16_t z;
} __attribute__((packed)) window_t;

void init_window_manager();
void CreateWindow(char *windowname, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t parentid);
void DestroyWindow(uint16_t id);
void DrawWindow(uint16_t id);

#endif