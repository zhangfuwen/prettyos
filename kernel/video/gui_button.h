#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "videomanager.h"


typedef struct
{
    char* label;
    uint16_t id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    void* data;
} button_t;

void CreateButton(button_t* button, uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* label);
void DrawButton(videoDevice_t* device, button_t* button);


#endif
