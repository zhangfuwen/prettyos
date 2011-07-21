/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "videomanager.h"
#include "list.h"
#include "kheap.h"
#include "console.h"
#include "keyboard.h"
#include "util.h"
#include "timer.h"
#include "vbe.h"
#include "gui.h"
#include "VBEShell.h"


videoDeviceDriver_t video_drivers[VDD_COUNT] =
{
    { // Renderbuffer
        .driverName = "RenderBuffer", .detect = 0, .createDevice = 0, .freeDevice = 0, .createModeList = 0, .enterVideoMode = 0, .leaveVideoMode = 0,
        .setPixel = &renderBuffer_setPixel, .fillPixels = &renderBuffer_fillPixels, .copyPixels = &renderBuffer_copyPixels, .clear = &renderBuffer_clear, .getPixel = &renderBuffer_getPixel, .flipScreen = &renderBuffer_flipScreen
    },
    { // VGA
        .driverName = "VGA", .detect = 0, .createDevice = 0, .freeDevice = 0, .createModeList = 0, .enterVideoMode = 0, .leaveVideoMode = 0,
        .setPixel = 0, .fillPixels = 0, .copyPixels = 0, .clear = 0, .getPixel = 0, .flipScreen = 0
    },
    { // VBE
        .driverName = "VBE", .detect = &vbe_detect, .createDevice = &vbe_createDevice, .freeDevice = &vbe_freeDevice, .createModeList = &vbe_createModeList, .enterVideoMode = &vbe_enterVideoMode, .leaveVideoMode = &vbe_leaveVideoMode,
        .setPixel = &vbe_setPixel, .fillPixels = &vbe_fillPixels, .copyPixels = 0, .clear = &vbe_clear, .getPixel = &vbe_getPixel, .flipScreen = &vbe_flipScreen
    }
};

static list_t* basicVideoDevices;
videoMode_t* video_currentMode = 0;


void video_install()
{
    basicVideoDevices = list_create();
    for(size_t i = 0; i < VDD_COUNT; i++)
    {
        if(video_drivers[i].detect == 0) continue;

        size_t num = video_drivers[i].detect();
        for(size_t j = 0; j < num; j++)
        {
            videoDevice_t* device = video_createDevice(video_drivers+i);
            device->videoMode.colorMode = 0;
            device->videoMode.data = 0;
            device->videoMode.device = device;
            device->videoMode.doubleBuffer = false;
            device->videoMode.type = VMT_NONE;
            device->videoMode.xRes = 0;
            device->videoMode.yRes = 0;
            list_append(basicVideoDevices, device);
        }
    }
}

void video_test()
{
    extern BMPInfo_t bmp_start;

    textColor(LIGHT_BLUE);
    printf("       >>>>>   Press 's' to skip video test or any key to continue   <<<<<\n\n");
    textColor(TEXT);

    if(getch() == 's')
    {
        return;
    }

    video_install();
    list_t* modelist = list_create();
    video_createModeList(modelist);

    textColor(TABLE_HEADING);
    printf("\nID\tDriver\tResolution\tColor depth\tType\n");
    puts("----------------------------------------------------------------------");
    textColor(TEXT);
    size_t id = 0;
    for(element_t* e = modelist->head; e != 0; e = e->next, id++)
    {
        videoMode_t* mode = e->data;
        printf("\n%u\t%s\t", id, mode->device->driver->driverName);
        if(printf("%ux%u\t", mode->xRes, mode->yRes) <= 8) putch('\t');
        switch(mode->colorMode)
        {
            case CM_2COL:
                puts("2 colors");
                break;
            case CM_16COL:
                puts("16 colors");
                break;
            case CM_256COL:
                puts("256 colors");
                break;
            case CM_15BIT:
                puts("15 bit\t");
                break;
            case CM_16BIT:
                puts("16 bit\t");
                break;
            case CM_24BIT:
                puts("24 bit\t");
                break;
            case CM_32BIT:
                puts("32 bit\t");
                break;
        }
        printf("\t%s", mode->type==VMT_GRAPHIC?"Graphic":"Text");

        if(id % 40 == 0 && id != 0)
            waitForKeyStroke(); // Slow down
    }
    textColor(TABLE_HEADING);
    puts("\n----------------------------------------------------------------------\n\n");
    textColor(TEXT);

    videoMode_t* mode = 0;

    while(true)
    {
        textColor(YELLOW);
        printf("Type in the ID of the mode: ");
        textColor(TEXT);
        char temp[20];
        gets(temp);

        if(strcmp(temp, "exit") == 0) return;

        uint16_t modenumber = atoi(temp);
        element_t* e = list_getElement(modelist, modenumber);
        if(e != 0)
        {
            mode = e->data;
            break;
        }
    }

    printf("\n1. Start Graphical Tests\n");
    printf("2. Start GUI\n");
    printf("3. Start VBE-Shell\n\n");
    uint16_t whatToStart = 0;

    while(whatToStart == 0)
    {
        textColor(YELLOW);
        printf("Type in the number: ");
        textColor(TEXT);
        char num[3];
        gets(num);
        whatToStart = atoi(num);
    }

    video_setMode(mode);

    if(whatToStart == 1)
    {
        uint16_t width = video_currentMode->xRes;
        uint16_t height = video_currentMode->yRes;
        uint16_t radius = min(width, height)/2;
        for(uint16_t i = 0; i < radius; i++)
        {
            BGRA_t color = {i*64/radius, i*256/radius, 128-(i*128/radius), 0}; // Create gradient
            video_drawCartesianCircle(video_currentMode->device, width/2, height/2, radius-i, color); // FPU
            sleepMilliSeconds(5);
        }

        BGRA_t bright_blue = {255, 75, 75, 0};
        video_drawLine(video_currentMode->device, 0, height/2, width, height/2, bright_blue);
        video_drawLine(video_currentMode->device, 0, height/2 + 1, width, height/2 + 1, bright_blue);
        video_drawLine(video_currentMode->device, width/2, 0, width/2, height, bright_blue);
        video_drawLine(video_currentMode->device, width/2+1, 0, width/2+1, height, bright_blue);
        video_drawCartesianCircle(video_currentMode->device, width/2, height/2, height/2, bright_blue); // FPU
        video_drawCartesianCircle(video_currentMode->device, width/2, height/2, height/2-1, bright_blue); // FPU
        waitForKeyStroke();

        video_drawBitmap(video_currentMode->device, 0, 0, &bmp_start);
        waitForKeyStroke();

        video_printPalette(video_currentMode->device);

        video_drawString(video_currentMode->device, "PrettyOS started in March 2009.\nThis hobby OS tries to be a possible access for beginners in this area.", 0, 400);
        waitForKeyStroke();

        video_drawScaledBitmap(video_currentMode->device, width, height, &bmp_start);
        waitForKeyStroke();

        video_clearScreen(video_currentMode->device, black);
        waitForKeyStroke();
    }
    else if(whatToStart == 2)
    {
        StartGUI();
    }
    else if(whatToStart == 3)
    {
        startVBEShell();
    }

    video_setMode(0); // Return to 80x50 text mode
}

void video_setMode(videoMode_t* mode)
{
    if(video_currentMode)
        video_currentMode->device->driver->leaveVideoMode(video_currentMode->device);

    if(mode)
        mode->device->driver->enterVideoMode(mode);
    video_currentMode = mode;
}

void video_createModeList(list_t* list)
{
    for(element_t* e = basicVideoDevices->head; e != 0; e = e->next)
    {
        videoDevice_t* device = e->data;
        device->driver->createModeList(device, list);
    }
}

void video_freeModeList(list_t* list)
{
}

videoDevice_t* video_createDevice(videoDeviceDriver_t* driver)
{
    videoDevice_t* device = malloc(sizeof(videoDevice_t), 0, "videoDevice_t");
    device->driver = driver;
    device->videoMode.data = 0;
    device->videoMode.device = device;
    device->videoMode.palette = 0;
    device->videoMode.type = VMT_GRAPHIC;
    if(device->driver->createDevice)
        device->driver->createDevice(device);
    return(device);
}

void video_freeDevice(videoDevice_t* device)
{
    if(device->driver->freeDevice)
        device->driver->freeDevice(device);
    free(device);
}


// Basic drawing functionality
void video_setPixel(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t color)
{
    device->driver->setPixel(device, x, y, color);
}

void video_fillPixels(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t color, size_t num)
{
    if(device->driver->fillPixels)
        device->driver->fillPixels(device, x, y, color, num);
    else // Fallback
        for(size_t i = 0; i < num; i++)
            device->driver->setPixel(device, x+1, y, color);
}

void video_copyPixels(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t* pixels, size_t num)
{
    if(device->driver->copyPixels)
        device->driver->copyPixels(device, x, y, pixels, num);
    else // Fallback
        for(size_t i = 0; i < num; i++)
            device->driver->setPixel(device, x+i, y, pixels[i]);
}

void video_clearScreen(videoDevice_t* device, BGRA_t color)
{
    if(device->driver->clear)
        device->driver->clear(device, color);
    else
        for(size_t y = 0; y < device->videoMode.yRes; y++)
            video_fillPixels(device, 0, y, color, device->videoMode.xRes);
}

void video_flipScreen(videoDevice_t* device)
{
    device->driver->flipScreen(device);
    video_clearScreen(device, black);
}

BGRA_t video_getPixel(videoDevice_t* buffer, uint32_t x, uint32_t y)
{
    return(buffer->driver->getPixel(buffer, x, y));
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
