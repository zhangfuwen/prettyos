#ifndef MOUSE_H
#define MOUSE_H

#include "util/util.h"

typedef enum
{
    BUTTON_LEFT = BIT(0),
    BUTTON_MIDDLE = BIT(1),
    BUTTON_RIGHT = BIT(2),
    BUTTON_4 = BIT(3),
    BUTTON_5 = BIT(4)
} mouse_button_t;


extern int32_t mouse_x;
extern int32_t mouse_y;
extern int32_t mouse_zv; // vertical mousewheel
extern int32_t mouse_zh; // horizontal mousewheel
extern mouse_button_t mouse_buttons; // Status of mouse buttons


void mouse_install(void);
void mouse_uninstall(void);
void mouse_setsamples(uint8_t samples_per_second);


#endif
