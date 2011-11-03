/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "textgui.h"
#include "keyboard.h"
#include "events.h"
#include "util/util.h"
#include "console.h"
#include "kheap.h"


static void drawBox(uint16_t width, uint16_t height);
static void drawGUI(uint16_t width, uint16_t height, uint8_t mode, bool selected);


void drawTitleAndMessage(const char* title, position_t titlepos, const char* message, position_t messagepos)
{
    setCursor(titlepos);
    textColor(0x0E);
    puts(title);
    setCursor(messagepos);
    textColor(TEXT);
    for(; *message != 0; message++)
    {
        if(*message == '\n')
            puts("\n           ");
        else
            putch(*message);
    }
}

uint16_t TextGUI_ShowMSG(const char* title, const char* message)
{
    void* oldvidmem = malloc(8000, 4, "old_vidmem");
    memcpy(oldvidmem, (void*)console_current->vidmem, 8000);
    position_t oldpos;
    getCursor(&oldpos);

    const position_t titlepos = {11, 10};
    const position_t textpos  = {11, 15};

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    uint16_t returnval = TEXTGUI_ABORTED;
    bool exit = false;
    for (;;)
    {
        drawBox(60, 30);
        drawTitleAndMessage(title, titlepos, message, textpos);
        drawGUI(60, 30, 1, false);
        switch (ev)
        {
            case EVENT_NONE:
                waitForEvent(50);
                break;
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if (*key == KEY_ESC)
                {
                    returnval = TEXTGUI_ABORTED;
                    exit = true;
                }
                else if (*key == KEY_ENTER)
                {
                    returnval = TEXTGUI_OK;
                    exit = true;
                }
                break;
            }
            default:
                break;
        }

        if (exit == true)
            break;

        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    memcpy((void*)console_current->vidmem, oldvidmem, 8000);
    free(oldvidmem);
    setCursor(oldpos);
    return (returnval);
}


uint16_t TextGUI_AskYN(const char* title, const char* message, uint8_t defaultselected)
{
    void* oldvidmem = malloc(8000, 4, "old_vidmem");
    memcpy(oldvidmem, (void*)console_current->vidmem, 8000);
    position_t oldpos;
    getCursor(&oldpos);

    const position_t titlepos = {11, 10};
    const position_t textpos  = {11, 15};

    bool selected = defaultselected==TEXTGUI_YES;

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    uint16_t returnval = TEXTGUI_ABORTED;
    bool exit = false;
    for (;;)
    {
        drawBox(60, 30);
        drawTitleAndMessage(title, titlepos, message, textpos);
        drawGUI(60, 30, 2, selected);
        switch (ev)
        {
            case EVENT_NONE:
                waitForEvent(50);
                break;
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                switch(*key)
                {
                    case KEY_ESC:
                        returnval = TEXTGUI_ABORTED;
                        exit = true;
                        break;
                    case KEY_ENTER:
                        if (selected == false)
                            returnval = TEXTGUI_NO;
                        else
                            returnval = TEXTGUI_YES;

                        exit = true;
                        break;
                    case KEY_Y: case KEY_Z:
                        selected = true;
                        break;
                    case KEY_N:
                        selected = false;
                        break;
                    case KEY_ARRL: case KEY_ARRR:
                        selected = !selected;
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }

        if (exit == true)
            break;

        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    memcpy((void*)console_current->vidmem, oldvidmem, 8000);
    free(oldvidmem);
    setCursor(oldpos);
    return (returnval);
}


static void drawBox(uint16_t width, uint16_t height)
{
    uint16_t startx = (COLUMNS/2) - (width/2) - 1;
    uint16_t starty = (LINES/2) - (height/2) - 1;
    uint16_t endx   = (COLUMNS/2) + (width/2) + 1;
    uint16_t endy   = (LINES/2) + (height/2) - 1;

    // Contentbox
    for (uint16_t y = starty; y < endy; y++)
        for (uint16_t x = startx; x < endx; x++)
            console_setPixel(x, y, 0);
}

static void drawGUI(uint16_t width, uint16_t height, uint8_t mode, bool selected)
{
    uint16_t startx = (COLUMNS/2) - (width/2) - 1;
    uint16_t starty = (LINES/2) - (height/2) - 1;
    uint16_t endx   = (COLUMNS/2) + (width/2) + 1;
    uint16_t endy   = (LINES/2) + (height/2) - 1;

    // Corners
    /*
     1=========2
     |         |
     |         |
     3=========4
    */
    console_setPixel((startx - 1), (starty - 1), (0x0F << 8) | 0xC9); // 1
    console_setPixel((endx - 1), (starty - 1), (0x0F << 8) | 0xBB);   // 2
    console_setPixel((startx - 1), (endy - 1), (0x0F << 8) | 0xC8);   // 3
    console_setPixel((endx - 1), (endy - 1), (0x0F << 8) | 0xBC);     // 4

    for (uint16_t y = starty; y < (endy - 1); y++)
    {
        // Y-borders
        console_setPixel(startx-1, y, (0x0F << 8) | 0xBA);
        console_setPixel(endx-1, y, (0x0F << 8) | 0xBA);
    }
    for (uint16_t x = startx; x < (endx - 1); x++)
    {
        // X-borders
        console_setPixel(x, starty-1, (0x0F << 8) | 0xCD);
        console_setPixel(x, endy-1, (0x0F << 8) | 0xCD);

        // Separations
        console_setPixel(x, endy-5, (0x0F << 8) | 0xCD);
        console_setPixel(x, starty+3, (0x0F << 8) | 0xCD);
    }

    // Corners of separations
    console_setPixel((startx - 1), endy - 5, (0x0F << 8) | 0xCC);
    console_setPixel((endx - 1), endy - 5, (0x0F << 8) | 0xB9);

    console_setPixel((startx - 1), starty+3, (0x0F << 8) | 0xCC);
    console_setPixel((endx - 1), starty+3, (0x0F << 8) | 0xB9);

    position_t buttonpos = { (startx+1), (endy-3) };
    setCursor(buttonpos);

    switch (mode)
    {
        case 1: // OK-Box
            // OK-Button
            textColor(LIGHT_GREEN);
            puts("<OK>");
            break;
        case 2: // Yes/No-Box
            // YES-Button
            if (selected == false)
                textColor(LIGHT_RED);
            else
                textColor(LIGHT_GREEN);
            puts("<YES>");
            textColor(TEXT);
            puts("  -  ");

            // NO-Button
            if (selected == true)
                textColor(LIGHT_RED);
            else
                textColor(LIGHT_GREEN);
            puts("<NO>");

            break;
        default:
            break;
    }
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
