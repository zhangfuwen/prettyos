#ifndef VIDEO_H
#define VIDEO_H

#include "os.h"

void refreshUserScreen();
void clear_screen();
void kprintf(const char* message, uint32_t line, uint8_t attribute, ...);
uint8_t AsciiToCP437(uint8_t ascii);
int32_t screenshot(char* name);
void screenshot_thread();
void mt_screenshot();

#endif
