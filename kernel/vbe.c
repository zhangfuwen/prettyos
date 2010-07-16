#include "vbe.h"

ModeInfoBlock_t modeInfoBlock;
ModeInfoBlock_t* mib = &modeInfoBlock;

uint8_t* SCREEN = (uint8_t*) 0xA0000;

void initGraphics(uint32_t x, uint32_t y, uint32_t pixelwidth)
{
    mib->XResolution  = x;
    mib->YResolution  = y;
    mib->BitsPerPixel = pixelwidth; // 256 colors
}

void setPixel(uint32_t x, uint32_t y, uint32_t color)
{
    SCREEN[y * mib->XResolution + x * mib->BitsPerPixel/8] = color;
}
