/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "gui_window.h"
#include "vbe.h"
#include "kheap.h"

extern ModeInfoBlock_t mib;

extern BMPInfo_t bmp_start;
extern BMPInfo_t bmp_end;
extern BMPInfo_t cursor_start;
extern BMPInfo_t cursor_end;

uint32_t* double_buffer;
// extern u32int* double_buffer;

BGRA_t WINDOW_COLOUR = {2, 255, 57, 0};
BGRA_t WINDOW_COLOUR_BACKGROUND = {191, 227, 197, 0};
BGRA_t WINDOW_COLOUR_BORDER = { 2, 125, 57, 0};
BGRA_t WINDOW_COLOUR_TOPBAR = {253, 100, 100, 0};
BGRA_t WINDOW_COLOUR_FOCUS_TOPBAR = {127, 255, 0, 0};

#define MAX_WINDOWS 256

volatile window_t current_window;
volatile window_t* window_list;

void init_window_manager()
{
    // We need to initialise the Desktop
    static window_t window;
    window.name = "Desktop";
    window.x = 0;
    window.y = 0;
    window.z = 0;
    window.width = mib.XResolution;
    window.height = mib.YResolution;
    window.parentid = 0;
    window.id = HWND_DESKTOP;
    window.data = double_buffer;

    current_window = window;

    window_list = (window_t*)malloc(sizeof(window_t)*MAX_WINDOWS, 0, "Window List");
    window_list[window.id] = current_window;
}

static uint16_t getnewwid()
{
    static uint32_t wid = 0;
    wid++;
    return wid;
}

void DestroyWindow(uint16_t id)
{
    window_list[id].data = 0; // free(window_list[id].data)
    window_list[id].name = 0;
    window_list[id].id = 0;
    window_list[id].x = 0;
    window_list[id].y = 0;
    window_list[id].z = 0;
    window_list[id].width = 0;
    window_list[id].height = 0;
    window_list[id].parentid = 0;
}

void CreateWindow(char* windowname, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t parentid)
{
    static window_t window;
    window.name = windowname;
    window.x = x;
    window.y = y;
    window.z = 1;
    window.width = width;
    window.height = height;
    window.parentid = parentid;
    window.id = getnewwid();

    window.data = malloc((width*height)*(mib.BitsPerPixel/8), 0, "Window buffer"); // Creates buffer for window
	
    // Fill
    vbe_drawRectFilled(window.x, window.y+20, window.x+window.width, window.y+window.height, WINDOW_COLOUR_BACKGROUND);

    // Topbar
    vbe_drawRectFilled(window.x, window.y, window.x+window.width, window.y+20, WINDOW_COLOUR_TOPBAR);

    // Border
    vbe_drawRect(window.x, window.y, window.x+window.width, window.y+window.height, WINDOW_COLOUR_BORDER);

    // Title
    vbe_drawString(windowname, window.x+1, window.y+1);

	// Data
	// vbe_drawBitmap(window.x, window.y+20, (BMPInfo_t*)window_list[0].data);
	// vbe_drawBitmap(window.x, window.y+20, &bmp_start);

    // And set window focus
    current_window = window;
    window_list[window.id] = current_window;
}

void reDrawWindow(uint16_t id)
{
	// Fill
    vbe_drawRectFilled(window_list[id].x, window_list[id].y+20, window_list[id].x+window_list[id].width, window_list[id].y+window_list[id].height, WINDOW_COLOUR_BACKGROUND);

    // Topbar
    vbe_drawRectFilled(window_list[id].x, window_list[id].y, window_list[id].x+window_list[id].width, window_list[id].y+20, WINDOW_COLOUR_TOPBAR);

    // Border
    vbe_drawRect(window_list[id].x, window_list[id].y, window_list[id].x+window_list[id].width, window_list[id].y+window_list[id].height, WINDOW_COLOUR_BORDER);

    // Title
    vbe_drawString(window_list[id].name, window_list[id].x+1, window_list[id].y+1);

	// Data
	vbe_drawBitmap(window_list[id].x, window_list[id].y+20, (BMPInfo_t*)window_list[id].data);
	vbe_drawString("redraw", window_list[id].x+30, window_list[id].y+20);
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