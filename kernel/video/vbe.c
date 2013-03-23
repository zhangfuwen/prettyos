/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util/util.h"
#include "paging.h"
#include "tasking/task.h"
#include "tasking/vm86.h"
#include "kheap.h"

// This values are hardcoded adresses from documentation/vidswtch.map
#define VM86_SETDISPLAYSTART   ((void*)0x100)
#define VM86_ENABLE8BITPALETTE ((void*)0x113)
#define VM86_SWITCH_TO_TEXT    ((void*)0x11E)
#define VM86_SWITCH_TO_VIDEO   ((void*)0x12C)
#define VM86_VGAINFOBLOCK      ((void*)0x13B)
#define VM86_MODEINFOBLOCK     ((void*)0x149)


VgaInfoBlock_t vgaIB;


// vm86
static pageDirectory_t* vbe_pd;
extern uintptr_t vidswtch_com_start;
extern uintptr_t vidswtch_com_end;


static void vbe_readVIB(void)
{
    *(char*)0x3400 = 'V';
    *(char*)0x3401 = 'B';
    *(char*)0x3402 = 'E';
    *(char*)0x3403 = '2';
    vm86_executeSync(vbe_pd, VM86_VGAINFOBLOCK);
    memcpy(&vgaIB, (void*)0x3400, sizeof(VgaInfoBlock_t));
}

static void vbe_readMIB(uint16_t mode, ModeInfoBlock_t* mib)
{
    *(uint16_t*)0x3600 = mode;
    vm86_executeSync(vbe_pd, VM86_MODEINFOBLOCK);
    memcpy(mib, (void*)0x3600, sizeof(ModeInfoBlock_t));
}

static void setDisplayStart(uint16_t xpos, uint16_t ypos)
{
    *(uint16_t*)0x1800 = ypos;
    *(uint16_t*)0x1802 = xpos;
    vm86_executeSync(vbe_pd, VM86_SETDISPLAYSTART);
}

// Set a Palette entry
// http://wiki.osdev.org/VGA_Hardware#VGA_Registers
static void Set_DAC_C(videoDevice_t* device, uint8_t PaletteColorNumber, BGRA_t color)
{
    vbe_videoDevice_t* vbeDevice = device->data;
    if (vbeDevice->paletteBitsPerColor == 0)
    {
        if (vgaIB.Capabilities[0] & BIT(0)) // VGA can handle palette with 8 bits per color -> Use it
        {
            vm86_executeSync(vbe_pd, VM86_ENABLE8BITPALETTE);
            vbeDevice->paletteBitsPerColor = 8;
        }
        else
        {
            vbeDevice->paletteBitsPerColor = 6;
        }
    }
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, PaletteColorNumber);
    if (vbeDevice->paletteBitsPerColor == 8)
    {
        outportb(0x03C9, color.red);
        outportb(0x03C9, color.green);
        outportb(0x03C9, color.blue);
    }
    else
    {
        outportb(0x03C9, color.red   >> 2);
        outportb(0x03C9, color.green >> 2);
        outportb(0x03C9, color.blue  >> 2);
    }
}

// Get a Palette entry
void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, PaletteColorNumber);
    *Red   = inportb(0x03C9);
    *Green = inportb(0x03C9);
    *Blue  = inportb(0x03C9);
}

static void vgaDebug(void)
{
  #ifdef _VBE_DEBUG_
    textColor(YELLOW);
    printf("\nVgaInfoBlock:\n");
    textColor(TEXT);
    printf("VESA-Signature:  %c%c%c%c\n", vgaIB.VESASignature[0], vgaIB.VESASignature[1], vgaIB.VESASignature[2], vgaIB.VESASignature[3]);
    printf("VESA-Version:    %u.%u\n",    vgaIB.VESAVersion>>8, vgaIB.VESAVersion&0xFF); // 01 02 ==> 1.2
    printf("Capabilities:    %yh",        vgaIB.Capabilities[0]);
    if (!(vgaIB.Capabilities[0] & BIT(0)))
        printf(" - 8-bit palette not supported.");
    printf("\nVideo Memory:    %u MiB\n", vgaIB.TotalMemory/0x10); // number of 64 KiB blocks of memory on the video card
    printf("Video Modes Ptr: %Xh",        vgaIB.VideoModes);
    waitForKeyStroke();
    putch('\n');
  #endif
}

static void modeDebug(ModeInfoBlock_t* mib)
{
  #ifdef _VBE_DEBUG_
    textColor(YELLOW);
    printf("\nModeInfoBlock:\n");
    textColor(TEXT);
    printf("ModeAttributes:        %xh\n", mib->ModeAttributes);
    printf("WinAAttributes:        %u\n", mib->WinAAttributes);
    printf("WinBAttributes:        %u\n", mib->WinBAttributes);
    printf("WinGranularity:        %u\n", mib->WinGranularity);
    printf("WinSize:               %u\n", mib->WinSize);
    printf("WinASegment:           %u\n", mib->WinASegment);
    printf("WinBSegment:           %u\n", mib->WinBSegment);
    printf("WinFuncPtr:            %Xh\n", mib->WinFuncPtr);
    printf("BytesPerScanLine:      %u\n", mib->BytesPerScanLine);
    printf("XResolution:           %u\n", mib->XResolution);
    printf("YResolution:           %u\n", mib->YResolution);
    printf("XCharSize:             %u\n", mib->XCharSize);
    printf("YCharSize:             %u\n", mib->YCharSize);
    printf("NumberOfPlanes:        %u\n", mib->NumberOfPlanes);
    printf("BitsPerPixel:          %u\n", mib->BitsPerPixel);
    printf("NumberOfBanks:         %u\n", mib->NumberOfBanks);
    printf("MemoryModel:           %u\n", mib->MemoryModel);
    printf("BankSize:              %u\n", mib->BankSize);
    printf("NumberOfImagePages:    %u\n", mib->NumberOfImagePages);
    printf("RedMaskSize:           %u\n", mib->RedMaskSize);
    printf("RedFieldPosition:      %u\n", mib->RedFieldPosition);
    printf("GreenMaskSize:         %u\n", mib->GreenMaskSize);
    printf("GreenFieldPosition:    %u\n", mib->GreenFieldPosition);
    printf("BlueMaskSize:          %u\n", mib->BlueMaskSize);
    printf("BlueFieldPosition:     %u\n", mib->BlueFieldPosition);
    printf("RsvdMaskSize:          %u\n", mib->RsvdMaskSize);
    printf("RsvdFieldPosition:     %u\n", mib->RsvdFieldPosition);
    printf("OffScreenMemOffset:    %u\n", mib->OffScreenMemOffset);
    printf("OffScreenMemSize:      %u\n", mib->OffScreenMemSize);
    printf("DirectColorModeInfo:   %u\n", mib->DirectColorModeInfo);
    printf("Physical Memory Base:  %Xh\n", mib->PhysBasePtr);
    waitForKeyStroke();
  #endif
}

// Interface functions
size_t vbe_detect(void)
{
    vbe_pd = paging_createUserPageDirectory();
    vm86_initPageDirectory(vbe_pd, (void*)0x100, &vidswtch_com_start, (uintptr_t)&vidswtch_com_end - (uintptr_t)&vidswtch_com_start);

    vbe_readVIB();
    vgaDebug();

    return ((strncmp(vgaIB.VESASignature, "VESA", 4) == 0) ? 1 : 0);
}

void vbe_createDevice(videoDevice_t* device)
{
    vbe_videoDevice_t* vbeDevice = malloc(sizeof(vbe_videoDevice_t), 0, "vbe_videoDevice_t");
    device->data = vbeDevice;
    device->videoMode.doubleBuffer = true;
    vbeDevice->memory = 0;
}

void vbe_freeDevice(videoDevice_t* device)
{
    free(device->data);
}

void vbe_createModeList(videoDevice_t* device, list_t* list)
{
    ModeInfoBlock_t mib;
    for (uint16_t i=0; i < 256; i++)
    {
        if (vgaIB.VideoModes[i] == 0xFFFF) break; // End of modelist
        vbe_readMIB(vgaIB.VideoModes[i], &mib);
        if (!(mib.ModeAttributes & BIT(0)) || !(mib.ModeAttributes & BIT(7))) continue; // If bit 0 is not set, the mode is not supported due to the present hardware configuration, if bit 7 is not set, linear frame buffer is not supported

        // Usable mode. Add it to list.
        videoMode_t* mode = malloc(sizeof(videoMode_t), 0, "videoMode_t");
        mode->colorMode = mib.BitsPerPixel;
        mode->data = (void*)(uintptr_t)vgaIB.VideoModes[i];
        mode->device = device;
        mode->doubleBuffer = true;
        mode->type = (mib.ModeAttributes & BIT(4)) ? VMT_GRAPHIC:VMT_TEXT;
        mode->xRes = mib.XResolution;
        mode->yRes = mib.YResolution;
        mode->palette = 0;
        list_append(list, mode);
    }
}

void vbe_freeMode(videoMode_t* mode)
{
    free(mode->palette);
    free(mode);
}

void vbe_enterVideoMode(videoMode_t* mode)
{
    mode->device->videoMode = *mode;
    mode = &mode->device->videoMode;

    vbe_videoDevice_t* vbeDevice = mode->device->data;

    uint16_t modenumber = (uint16_t)(uintptr_t)mode->data;
    ModeInfoBlock_t mib;
    vbe_readMIB(modenumber, &mib);
    modeDebug(&mib);

    // Initalize screen
    if (mode->palette != 0)
        free(mode->palette);
    if (mode->colorMode == CM_256COL)
    {
        mode->palette = malloc(sizeof(BGRA_t)*256, 0, "vbe palette");
        for (size_t i = 0; i < 256; i++)
        {
            BGRA_t color = {(i&0x3)<<6, (i&0x1C)<<3, i&0xE0};
            Set_DAC_C(mode->device, i, color);
            mode->palette[i] = color;
        }
    }

    // Allocate memory
    if (vbeDevice->memory == 0)
    {
        uint32_t numberOfPages = vgaIB.TotalMemory * 0x10000 / PAGESIZE;
        vbeDevice->memory = paging_acquirePciMemory(mib.PhysBasePtr, numberOfPages);

        printf("\nVidmem (phys): %Xh  Vidmem (virt): %Xh\n", mib.PhysBasePtr, vbeDevice->memory);
        printf("\nVideo Ram %u MiB, numOfPages: %u\n", vgaIB.TotalMemory/0x10, numberOfPages);
    }

    // Switch to video mode
    *(uint16_t*)0x3600 = 0xC1FF&(0xC000|modenumber); // Bits 9-13 may not be set, bits 14-15 should be set always
    vm86_executeSync(vbe_pd, VM86_SWITCH_TO_VIDEO);

    if (!(modenumber&BIT(15))) // We clear the Videoscreen manually, because the VGA is not reliable
        memset(vbeDevice->memory, 0, mode->xRes*mode->yRes*(mode->colorMode % 8 == 0 ? mode->colorMode/8 : mode->colorMode/8 + 1));

    vbeDevice->paletteBitsPerColor = 0; // Has to be reinitializated
    videomode = VM_VBE;

    vbeDevice->buffer1 = vbeDevice->memory;
    vbeDevice->buffer2 = vbeDevice->memory + mib.XResolution*mib.YResolution*(mib.BitsPerPixel/8 + (mib.BitsPerPixel % 8 == 0 ? 0 : 1)); // Set Double buffer pointer
}

void vbe_leaveVideoMode(videoDevice_t* device)
{
    vm86_executeSync(vbe_pd, VM86_SWITCH_TO_TEXT); // Needed until VGA-Text driver is written
    videomode = VM_TEXT;
    refreshScreen();
}

void vbe_setPixel(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color)
{
    vbe_videoDevice_t* vbeDevice = device->data;
    switch (device->videoMode.colorMode)
    {
        case CM_15BIT:
            ((uint16_t*)vbeDevice->memory)[y * device->videoMode.xRes + x] = BGRAtoBGR15(color);
            break;
        case CM_16BIT:
            ((uint16_t*)vbeDevice->memory)[y * device->videoMode.xRes + x] = BGRAtoBGR16(color);
            break;
        case CM_24BIT:
            *(uint16_t*)(vbeDevice->memory + (y * device->videoMode.xRes + x) * 3) = *(uint16_t*)&color; // Performance Hack - copying 16 bits at once should be faster than copying 8 bits twice
            ((uint8_t*)vbeDevice->memory)[(y * device->videoMode.xRes + x) * 3 + 2] = color.red;
            break;
        case CM_32BIT:
            ((uint32_t*)vbeDevice->memory)[y * device->videoMode.xRes + x] = *(uint32_t*)&color;
            break;
        case CM_256COL: default:
            ((uint8_t*)vbeDevice->memory)[y * device->videoMode.xRes + x] = BGRAtoBGR8(color);
            break;
    }
}

void vbe_fillPixels(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color, size_t num)
{
    vbe_videoDevice_t* vbeDevice = device->data;
    switch (device->videoMode.colorMode)
    {
        case CM_15BIT:
            memsetw(((uint16_t*)vbeDevice->memory) + y * device->videoMode.xRes + x, BGRAtoBGR15(color), num);
            break;
        case CM_16BIT:
            memsetw(((uint16_t*)vbeDevice->memory) + y * device->videoMode.xRes + x, BGRAtoBGR16(color), num);
            break;
        case CM_24BIT:
        {
            void* vidmemBase = vbeDevice->memory + (y * device->videoMode.xRes + x) * 3;
            for (size_t i = 0; i < num; i++)
            {
                *(uint16_t*)(vidmemBase + i*3) = *(uint16_t*)&color; // Performance Hack - copying 16 bits at once should be faster than copying 8 bits twice
                ((uint8_t*)vidmemBase)[i*3 + 2] = color.red;
            }
            break;
        }
        case CM_32BIT:
            memsetl(((uint32_t*)vbeDevice->memory) + y * device->videoMode.xRes + x, *(uint32_t*)&color, num);
            break;
        case CM_256COL: default:
            memset(vbeDevice->memory + y * device->videoMode.xRes + x, BGRAtoBGR8(color), num);
            break;
    }
}

BGRA_t vbe_getPixel(videoDevice_t* device, uint16_t x, uint16_t y) // TODO: Fix it. It now ignores mib.BitsPerPixel
{
    vbe_videoDevice_t* vbeDevice = device->data;
    return ((BGRA_t*)vbeDevice->memory)[y * device->videoMode.colorMode + x]; // HACK
}

void vbe_clear(videoDevice_t* device, BGRA_t color)
{
    vbe_videoDevice_t* vbeDevice = device->data;
    memset(vbeDevice->memory, 0, device->videoMode.xRes*device->videoMode.yRes*(device->videoMode.colorMode % 8 == 0 ? device->videoMode.colorMode/8 : device->videoMode.colorMode/8 + 1)); // HACK: Color ignored
}

void vbe_flipScreen(videoDevice_t* device)
{
    vbe_videoDevice_t* vbeDevice = device->data;

    // Wait for a vertical retrace
    while (inportb(0x03da) & 0x08) {}
    while (!(inportb(0x03da) & 0x08)) {}

    // Set display start during vertical retrace.
    if (vbeDevice->memory == vbeDevice->buffer1)
    {
        setDisplayStart(0, 0);
        vbeDevice->memory = vbeDevice->buffer2;
    }
    else
    {
        setDisplayStart(0, device->videoMode.yRes);
        vbeDevice->memory = vbeDevice->buffer1;
    }
}

/*
* Copyright (c) 2010-2013 The PrettyOS Project. All rights reserved.
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
