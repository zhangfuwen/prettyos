/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "gui_window.h"
#include "vbe.h"
#include "kheap.h"
#include "util.h"

extern ModeInfoBlock_t mib;

extern BMPInfo_t bmp_start;
extern BMPInfo_t bmp_end;
extern BMPInfo_t cursor_start;
extern BMPInfo_t cursor_end;

BGRA_t* double_buffer;
// extern u32int* double_buffer;

static const BGRA_t WINDOW_COLOUR = {2, 255, 57, 0};
static const BGRA_t WINDOW_COLOUR_BACKGROUND = {191, 227, 197, 0};
static const BGRA_t WINDOW_COLOUR_BORDER = { 2, 125, 57, 0};
static const BGRA_t WINDOW_COLOUR_TOPBAR = {253, 100, 100, 0};
static const BGRA_t WINDOW_COLOUR_FOCUS_TOPBAR = {127, 255, 0, 0};

static volatile window_t* current_window = 0;
volatile window_t* window_list[MAX_WINDOWS];

void init_window_manager()
{
    window_t* desktop = malloc(sizeof(window_t), 0, "desktop window");
    // We need to initialise the Desktop
    desktop->name = "Desktop";
    desktop->x = 0;
    desktop->y = 0;
    desktop->z = 0;
    desktop->width = mib.XResolution;
    desktop->height = mib.YResolution;
    desktop->parentid = 0;
    desktop->id = HWND_DESKTOP;
    desktop->data = double_buffer;

    window_list[desktop->id] = desktop;

    current_window = desktop;
}

static uint16_t getnewwid()
{
    static uint32_t wid = 0;
    wid++;
    return wid;
}

void DestroyWindow(uint16_t id)
{
    if(id != HWND_DESKTOP)
        free(window_list[id]->data);
    free((void*)window_list[id]);
    window_list[id] = 0;
}

void CreateWindow(char* windowname, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t parentid)
{
    window_t* window = malloc(sizeof(window_t), 0, "window");
    window->name = windowname;
    window->x = x;
    window->y = y;
    window->z = 1;
    window->width = width;
    window->height = height;
    window->parentid = parentid;
    window->id = getnewwid();

    window->data = malloc((width*height)*(mib.BitsPerPixel/8), 0, "Window buffer"); // Creates buffer for window

    window->CloseButton = CreateButton((window->x + window->width - 20), (window->y + 1), 18, 18, "X");

    // And set window focus
    current_window = window;
    window_list[window->id] = window;
}

void DrawWindow(uint16_t id)
{
    // Fill
    vbe_drawRectFilled(window_list[id]->x, window_list[id]->y+20, window_list[id]->x+window_list[id]->width, window_list[id]->y+window_list[id]->height, WINDOW_COLOUR_BACKGROUND);

    // Topbar
    vbe_drawRectFilled(window_list[id]->x, window_list[id]->y, window_list[id]->x+window_list[id]->width, window_list[id]->y+20, WINDOW_COLOUR_TOPBAR);

    // Border
    vbe_drawRect(window_list[id]->x, window_list[id]->y, window_list[id]->x+window_list[id]->width, window_list[id]->y+window_list[id]->height, WINDOW_COLOUR_BORDER);

    // Title
    vbe_drawString(window_list[id]->name, window_list[id]->x+2, window_list[id]->y+2);

    // Data
    DrawButton(&window_list[id]->CloseButton);

    vbe_drawBitmap(window_list[id]->x, window_list[id]->y+20, (BMPInfo_t*)window_list[id]->data);
    vbe_drawString("redraw", window_list[id]->x+30, window_list[id]->y+20);
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
