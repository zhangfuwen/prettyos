#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "vbe.h"

typedef struct
{
    char *label;
    uint16_t id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uintptr_t *data;
} button_t;

button_t CreateButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* label);
void DrawButton(button_t* button);

#endif