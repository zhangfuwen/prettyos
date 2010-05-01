#ifndef MOUSE_H
#define MOUSE_H

#include "os.h"

void mouse_install();
void mouse_uninstall();
void mouse_setsamples(uint8_t samples_per_second);
void mouse_wait(uint8_t a_type);
void mouse_initspecialfeatures();
void mouse_write(int8_t a_write);
char mouse_read();
void mouse_handler(registers_t* a_r);

#endif
