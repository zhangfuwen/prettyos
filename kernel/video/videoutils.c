/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "videoutils.h"
#include "util/util.h"
#include "video.h"
#include "font.h"
#include "kheap.h"


const BGRA_t black = {0, 0, 0, 0xFF};
const BGRA_t white = {0xFF, 0xFF, 0xFF, 0xFF};

position_t curPos = {0, 0};


// Advanced and formatted drawing functionality
void video_drawLine(videoDevice_t* buffer, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, BGRA_t color)
{
    uint32_t dxabs = x2>x1 ? (x2-x1):(x1-x2); // the absolute horizontal distance of the line
    uint32_t dyabs = y2>y1 ? (y2-y1):(y1-y2); // the absolute vertical distance of the line
    uint32_t sdx = x2>x1 ? 1 : -1; // sign of the horizontal distance of the line
    uint32_t sdy = y2>y1 ? 1 : -1; // sign of the horizontal distance of the line
    uint32_t x = dyabs/2;
    uint32_t y = dxabs/2;

    video_setPixel(buffer, x1, y1, color);

    if (dxabs >= dyabs) // the line is more horizontal than vertical
    {
        for (uint32_t i=0;i<dxabs;i++)
        {
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                y1+=sdy;
            }
            x1+=sdx;
            video_setPixel(buffer, x1,y1,color);
        }
    }
    else // the line is more vertical than horizontal
    {
        for (uint32_t i=0;i<dyabs;i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                x1+=sdx;
            }
            y1+=sdy;
            video_setPixel(buffer, x1,y1,color);
        }
    }
}

void video_drawRect(videoDevice_t* buffer, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color)
{
    // Sort edges
    if (top>bottom)
    {
        uint32_t temp=top;
        top=bottom;
        bottom=temp;
    }
    if (left>right)
    {
        uint32_t temp=left;
        left=right;
        right=temp;
    }

    video_fillPixels(buffer, left, top, color, right-left);
    video_fillPixels(buffer, left, bottom, color, right-left);
    for (uint32_t i=top; i<=bottom; i++)
    {
        video_setPixel(buffer, left, i, color);
        video_setPixel(buffer, right, i, color);
    }
}

void video_drawRectFilled(videoDevice_t* buffer, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color)
{
    for (uint32_t j = top; j < bottom; j++)
    {
        video_fillPixels(buffer, left, j, color, right-left);
    }
}

void video_drawCartesianCircle(videoDevice_t* buffer, uint32_t xm, uint32_t ym, uint32_t radius, BGRA_t color)
{
    for (uint32_t i = 0; i <= radius; i++)
    {
        uint32_t ydiff = sqrt(radius*radius - i*i);
        video_setPixel(buffer, xm - i, ym + ydiff, color);
        video_setPixel(buffer, xm - i, ym - ydiff, color);
        video_setPixel(buffer, xm + i, ym + ydiff, color);
        video_setPixel(buffer, xm + i, ym - ydiff, color);
    }
}

void video_drawChar(videoDevice_t* buffer, char c)
{
    uint8_t uc = AsciiToCP437((uint8_t)c); // no negative values

    switch (uc)
    {
        case 0x08: // backspace: move the cursor one space backwards and delete
            /*move_cursor_left();
            putch(' ');
            move_cursor_left();*/
            break;
        case 0x09: // tab: increment cursor.x (divisible by 8)
            curPos.x = (curPos.x + fontWidth*8) & ~(fontWidth*8 - 1);
            //if (curPos.x+fontWidth >= buffer->width) vbe_drawChar(buffer, '\n');
            break;
        case '\r': // r: cursor back to the margin
            curPos.x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment cursor.y
            curPos.x = 0;
            curPos.y += fontHeight;
            break;
        default:
            //if (curPos.x+fontWidth >= buffer->width) vbe_drawChar(buffer, '\n');
            if (uc != 0)
            {
                for (uint32_t y = 0; y < fontHeight; y++)
                {
                    for (uint32_t x = 0; x < fontWidth; x++)
                    {
                        BGRA_t temp = {font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], 0};
                        video_setPixel(buffer, curPos.x+x, curPos.y+y, temp);
                    }
                }
            }
            curPos.x += 8;
            break;
    }
}

void video_drawString(videoDevice_t* buffer, const char* text, uint32_t xpos, uint32_t ypos)
{
    curPos.x = xpos;
    curPos.y = ypos;
    for (; *text; video_drawChar(buffer, *text), ++text);
}

void video_drawBitmap(videoDevice_t* buffer, uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap)
{
    uint8_t* pixel = ((uint8_t*)bitmap) + bitmap->header.Offset + bitmap->header.Width * bitmap->header.Height - 1;
    for (uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for (uint32_t x=bitmap->header.Width; x>0; x--)
        {
            video_setPixel(buffer, xpos+x, ypos+y, bitmap->bmicolors[*pixel]);
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void video_drawBitmapTransparent(videoDevice_t* buffer, uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap, BGRA_t colorKey)
{
    uint8_t* pixel = ((uint8_t*)bitmap) + bitmap->header.Offset + bitmap->header.Width * bitmap->header.Height - 1;
    for (uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for (uint32_t x=bitmap->header.Width; x>0; x--)
        {
            if (bitmap->bmicolors[*pixel].red != colorKey.red || bitmap->bmicolors[*pixel].green != colorKey.green || bitmap->bmicolors[*pixel].blue != colorKey.blue)
            {
                video_setPixel(buffer, xpos+x, ypos+y, bitmap->bmicolors[*pixel]);
            }
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void video_drawScaledBitmap(videoDevice_t* buffer, uint32_t newSizeX, uint32_t newSizeY, BMPInfo_t* bitmap)
{
    uint8_t* pixel = ((uint8_t*)bitmap) + bitmap->header.Offset + bitmap->header.Width * bitmap->header.Height - 1;

    float FactorX = (float)newSizeX / (float)bitmap->header.Width;
    float FactorY = (float)newSizeY / (float)bitmap->header.Height;

    float OverflowX = 0, OverflowY = 0;
    for (uint32_t y = 0; y < newSizeY; y += (uint32_t)FactorY)
    {
        for (int32_t x = newSizeX-1; x >= 0; x -= (int32_t)FactorX)
        {
            pixel -= bitmap->header.BitsPerPixel/8;

            uint16_t rowsX = (uint16_t)FactorX + (OverflowX >= 1 ? 1:0);
            uint16_t rowsY = (uint16_t)FactorY + (OverflowY >= 1 ? 1:0);
            for (uint16_t i = 0; i < rowsX; i++)
            {
                for (uint16_t j = 0; j < rowsY; j++)
                {
                    if (x-i > 0)
                    {
                        video_setPixel(buffer, x-i, y+j, bitmap->bmicolors[*pixel]);
                    }
                }
            }

            if (OverflowX >= 1)
            {
                OverflowX--;
                x--;
            }
            OverflowX += FactorX-(int)FactorX;
        }
        if (OverflowY >= 1)
        {
            OverflowY--;
            y++;
        }
        OverflowY += FactorY-(int)FactorY;
    }
}

void video_printPalette(videoDevice_t* device)
{
    if (device->videoMode.colorMode != CM_256COL) return;

    uint16_t xpos = 0;
    uint16_t ypos = 0;
    for (uint16_t j=0; j<256; j++)
    {
        video_drawRectFilled(device, xpos, ypos, xpos+5, ypos+5, device->videoMode.palette[j]);
        xpos +=5;
        if (xpos >= 255)
        {
            ypos += 5;
            xpos = 0;
        }
    }
}


// Renderbuffer
renderBuffer_t* renderBuffer_create(uint16_t width, uint16_t height, uint8_t bitsPerPixel)
{
    renderBuffer_t* buffer = malloc(sizeof(renderBuffer_t), 0, "renderBuffer_t");
    buffer->device = video_createDevice(video_drivers + VDD_RENDERBUFFER);
    buffer->device->videoMode.colorMode = bitsPerPixel;
    buffer->device->videoMode.xRes = width;
    buffer->device->videoMode.yRes = height;
    buffer->device->data = buffer;
    buffer->buffer = malloc(width*height*bitsPerPixel/8, 0, "renderBuffer_t::buffer");
    buffer->buffer2 = 0;
    return (buffer);
}

void renderBuffer_free(renderBuffer_t* buffer)
{
    free(buffer->buffer);
    free(buffer->buffer2);
    video_freeDevice(buffer->device);
    free(buffer);
}

void renderBuffer_render(videoDevice_t* destination, renderBuffer_t* buffer, uint16_t x, uint16_t y)
{
    for (size_t i = 0; i < buffer->device->videoMode.yRes; i++)
    {
        video_copyPixels(destination, x, y+i, buffer->buffer + 4*buffer->device->videoMode.xRes*i, buffer->device->videoMode.xRes); // 32-bit hack
    }
}

void renderBuffer_setPixel(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color)
{
    renderBuffer_t* buffer = device->data;
    switch (device->videoMode.colorMode)
    {
        case CM_15BIT:
            ((uint16_t*)buffer->buffer)[y * device->videoMode.xRes + x] = BGRAtoBGR15(color);
            break;
        case CM_16BIT:
            ((uint16_t*)buffer->buffer)[y * device->videoMode.xRes + x] = BGRAtoBGR16(color);
            break;
        case CM_24BIT:
            *(uint16_t*)(buffer->buffer + (y * device->videoMode.xRes + x) * 3) = *(uint16_t*)&color; // Performance Hack - copying 16 bits at once should be faster than copying 8 bits twice
            ((uint8_t*)buffer->buffer)[(y * device->videoMode.xRes + x) * 3 + 2] = color.red;
            break;
        case CM_32BIT:
            ((uint32_t*)buffer->buffer)[y * device->videoMode.xRes + x] = *(uint32_t*)&color;
            break;
        case CM_256COL: default:
            ((uint8_t*)buffer->buffer)[y * device->videoMode.xRes + x] = BGRAtoBGR8(color);
            break;
    }
}

void renderBuffer_fillPixels(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color, size_t num)
{
    renderBuffer_t* buffer = device->data;
    switch (device->videoMode.colorMode)
    {
        case CM_15BIT:
            memsetw(((uint16_t*)buffer->buffer) + y * device->videoMode.xRes + x, BGRAtoBGR15(color), num);
            break;
        case CM_16BIT:
            memsetw(((uint16_t*)buffer->buffer) + y * device->videoMode.xRes + x, BGRAtoBGR16(color), num);
            break;
        case CM_24BIT:
        {
            void* vidmemBase = buffer->buffer + (y * device->videoMode.xRes + x) * 3;
            for (size_t i = 0; i < num; i++)
            {
                *(uint16_t*)(vidmemBase + i*3) = *(uint16_t*)&color; // Performance Hack - copying 16 bits at once should be faster than copying 8 bits twice
                ((uint8_t*)vidmemBase)[i*3 + 2] = color.red;
            }
            break;
        }
        case CM_32BIT:
            memsetl(((uint32_t*)buffer->buffer) + y * device->videoMode.xRes + x, *(uint32_t*)&color, num);
            break;
        case CM_256COL: default:
            memset(buffer->buffer + y * device->videoMode.xRes + x, BGRAtoBGR8(color), num);
            break;
    }
}

void renderBuffer_copyPixels(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t* colors, size_t num)
{
    renderBuffer_t* buffer = device->data;
    switch (device->videoMode.colorMode)
    {
        case CM_32BIT:
            memcpy(buffer->buffer + 4*device->videoMode.xRes*y + x*4, colors, num);
            break;
        case CM_15BIT: case CM_16BIT: case CM_24BIT: case CM_256COL: default:
            for (size_t i = 0; i < num; i++)
                renderBuffer_setPixel(device, x + i, y, colors[i]);
            break;
    }
}

void renderBuffer_clear(videoDevice_t* device, BGRA_t color)
{
    renderBuffer_fillPixels(device, 0, 0, color, device->videoMode.xRes*device->videoMode.yRes);
}

BGRA_t renderBuffer_getPixel(videoDevice_t* device, uint16_t x, uint16_t y)
{
    renderBuffer_t* buffer = device->data;
    return ((BGRA_t*)buffer->buffer)[y * device->videoMode.colorMode + x]; // HACK
}

void renderBuffer_flipScreen(videoDevice_t* device)
{
    renderBuffer_t* buffer = device->data;
    if (buffer->buffer2 == 0)
        buffer->buffer2 = malloc(device->videoMode.xRes*device->videoMode.yRes*device->videoMode.colorMode/8, 0, "renderBuffer_t::buffer2");

    void* temp = buffer->buffer2;
    buffer->buffer2 = buffer->buffer;
    buffer->buffer = temp;
}

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
