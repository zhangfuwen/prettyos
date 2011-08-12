/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/


#include "keyboard.h"
#include "events.h"
#include "video.h"
#include "util.h"
#include "console.h"
#include "kheap.h"
#include "serial.h"
#include "textgui.h"
#include "os.h"

// static uint16_t* vidmem = (uint16_t*)VIDEORAM; // Video memory

#define COLUMNS 80 // Why aren't these in one file?
#define LINES 50 // (found COLUMNS in console.h and LINES in video.c)


char* strstr(const char* str1, const char* str2);
char * stringReplace(char *search, char *replace, char *string);

bool TextGUI_internal_DrawBox(uint16_t width, uint16_t height);
bool TextGUI_internal_DrawGUI(uint16_t width, uint16_t height, uint8_t mode, bool selected);



uint16_t TextGUI_ShowMSG(char* title, char* message) {
    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    void* oldvidmem = malloc(8000, 4, "old_vidmem");
    memcpy(oldvidmem, (void*)console_current->vidmem, 8000);

    // memshow(vidmem);
    // memshow(oldvidmem, 4000, false);

    position_t oldpos;
    getCursor(&oldpos);

    position_t titlepos = {11, 10};
    position_t textpos  = {11, 15};

    message = stringReplace("\n", "(nl)           ", message);
    message = stringReplace("(nl)", "\n", message);

    TextGUI_internal_DrawBox(60, 30);
    setCursor(titlepos);
    textColor(0x0E);
    printf(title);
    setCursor(textpos);
    textColor(TEXT);
    printf(message);
    TextGUI_internal_DrawGUI(60, 30, 1, false);


    uint16_t returnval = TEXTGUI_ABORTED;
    bool exit = false;
    for (;;)
    {


        switch (ev)
        {
            case EVENT_NONE:
            {
                TextGUI_internal_DrawBox(60, 30);
                setCursor(titlepos);
                textColor(0x0E);
                printf(title);
                setCursor(textpos);
                textColor(TEXT);
                printf(message);
                TextGUI_internal_DrawGUI(60, 30, 1, false);

                waitForEvent(50);
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if (*key == KEY_ESC)
                {
                    returnval = TEXTGUI_ABORTED;
                    exit = true;
                    break;
                }
                if (*key == KEY_ENTER)
                {
                    returnval = TEXTGUI_OK;
                    exit = true;
                    break;
                }
                break;
            }
            default:
                break;
        }

        if (exit == true) {
            break;
        }

        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    memcpy((void*)console_current->vidmem, oldvidmem, 8000);
    free(oldvidmem);
    setCursor(oldpos);
    return(returnval);
}


uint16_t TextGUI_AskYN(char* title, char* message, uint8_t defaultselected) {
    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);

    void* oldvidmem = malloc(8000, 4, "old_vidmem");
    memcpy(oldvidmem, (void*)console_current->vidmem, 8000);


    position_t oldpos;
    getCursor(&oldpos);

    position_t titlepos = {11, 10};
    position_t textpos  = {11, 15};

    message = stringReplace("\n", "(nl)           ", message);
    message = stringReplace("(nl)", "\n", message);

    bool selected = false;

    if (defaultselected==TEXTGUI_YES) {
        selected=true;
    } else {
        selected=false;
    }

    TextGUI_internal_DrawBox(60, 30);
    setCursor(titlepos);
    textColor(0x0E);
    printf(title);
    setCursor(textpos);
    textColor(TEXT);
    printf(message);
    TextGUI_internal_DrawGUI(60, 30, 2, selected);


    uint16_t returnval = TEXTGUI_ABORTED;
    bool exit = false;
    for (;;)
    {
        switch (ev)
        {
            case EVENT_NONE:
            {
                TextGUI_internal_DrawBox(60, 30);
                setCursor(titlepos);
                textColor(0x0E);
                printf(title);
                setCursor(textpos);
                textColor(TEXT);
                printf(message);
                TextGUI_internal_DrawGUI(60, 30, 2, selected);

                waitForEvent(50);
                break;
            }

            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if (*key == KEY_ESC)
                {
                    returnval = TEXTGUI_ABORTED;
                    exit = true;
                    break;
                }

                if (*key == KEY_ENTER)
                {
                    if (selected == false) {
                        returnval = TEXTGUI_NO;
                    } else {
                        returnval = TEXTGUI_YES;
                    }

                    exit = true;
                    break;
                }

                if (*key == KEY_Y || *key == KEY_Z) {
                    selected = true;
                }

                if (*key == KEY_N) {
                    selected = false;
                }

                if (*key == KEY_ARRL || *key == KEY_ARRR) {
                    if (selected == false) {
                        selected = true;
                    } else {
                        selected = false;
                    }
                }

                break;
            }
            default:
                break;
        }

        if (exit == true) {
            break;
        }

        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    memcpy((void*)console_current->vidmem, oldvidmem, 8000);
    free(oldvidmem);
    setCursor(oldpos);
    return(returnval);
}


bool TextGUI_internal_DrawBox(uint16_t width, uint16_t height) {

    uint16_t startx = ((COLUMNS/2) - (width/2));
    uint16_t starty = ((LINES/2) - (height/2));
    uint16_t endx   = ((COLUMNS/2) + (width/2));
    uint16_t endy   = ((LINES/2) + (height/2));

    endx = endx + 1;
    endy = endy + 1;
    startx = startx - 1;
    starty = starty + 1;

    // Contentbox
    for (uint16_t y = starty; y < endy; y++) {
        for (uint16_t x = startx; x < endx; x++) {
            vga_setPixel(x, y, (0x00 << 8) | 0xDB);
        }
    }


    return(0);
}

bool TextGUI_internal_DrawGUI(uint16_t width, uint16_t height, uint8_t mode, bool selected) {

    uint16_t startx = ((COLUMNS/2) - (width/2));
    uint16_t starty = ((LINES/2) - (height/2));
    uint16_t endx   = ((COLUMNS/2) + (width/2));
    uint16_t endy   = ((LINES/2) + (height/2));


    endx = endx + 1;
    endy = endy + 1;
    startx = startx - 1;
    starty = starty + 1;


    // X-borders
    for (uint16_t x = startx; x < (endx - 1); x++) {
        vga_setPixel(x, starty-1, (0x0F << 8) | 0xCD);
        vga_setPixel(x, endy-1, (0x0F << 8) | 0xCD);
    }

    // Y-borders
    for (uint16_t y = starty; y < (endy - 1); y++) {
        vga_setPixel(startx-1, y, (0x0F << 8) | 0xBA);
        vga_setPixel(endx-1, y, (0x0F << 8) | 0xBA);
    }


    // Corners
    /*
     1=========2
     |         |
     |         |
     3=========4
     */

    // 1:
    vga_setPixel((startx - 1), (starty - 1), (0x0F << 8) | 0xC9);

    // 2:
    vga_setPixel((endx - 1), (starty - 1), (0x0F << 8) | 0xBB);

    // 3:
    vga_setPixel((startx - 1), (endy - 1), (0x0F << 8) | 0xC8);

    // 4:
    vga_setPixel((endx - 1), (endy - 1), (0x0F << 8) | 0xBC);

    position_t buttonpos = { (startx+1), (endy-5) };

    switch (mode) {
        case 1: // OK-Box
            // X-Seperation
            for (uint16_t x = startx; x < (endx - 1); x++) {
                vga_setPixel(x, endy-5, (0x0F << 8) | 0xCD);
            }

            vga_setPixel((startx - 1), endy - 5, (0x0F << 8) | 0xCC);
            vga_setPixel((endx - 1), endy - 5, (0x0F << 8) | 0xB9);

            for (uint16_t x = startx; x < (endx - 1); x++) {
                vga_setPixel(x, starty+3, (0x0F << 8) | 0xCD);
            }

            vga_setPixel((startx - 1), starty+3, (0x0F << 8) | 0xCC);
            vga_setPixel((endx - 1), starty+3, (0x0F << 8) | 0xB9);

            // OK-Button:
            setCursor(buttonpos);

            textColor(LIGHT_GREEN);
            printf("<OK>");

            break;
        case 2: // Yes/No-Box
            // X-Seperation
            for (uint16_t x = startx; x < (endx - 1); x++) {
                vga_setPixel(x, endy-5, (0x0F << 8) | 0xCD);
            }

            vga_setPixel((startx - 1), endy - 5, (0x0F << 8) | 0xCC);
            vga_setPixel((endx - 1), endy - 5, (0x0F << 8) | 0xB9);

            for (uint16_t x = startx; x < (endx - 1); x++) {
                vga_setPixel(x, starty+3, (0x0F << 8) | 0xCD);
            }

            vga_setPixel((startx - 1), starty+3, (0x0F << 8) | 0xCC);
            vga_setPixel((endx - 1), starty+3, (0x0F << 8) | 0xB9);

            // OK-Button:

            setCursor(buttonpos);

            if (selected == false) {
                textColor(LIGHT_RED);
                printf("<YES>");
                textColor(TEXT);
                printf("  -  ");
                textColor(LIGHT_GREEN);
                printf("<NO>");
            } else {
                textColor(LIGHT_GREEN);
                printf("<YES>");
                textColor(TEXT);
                printf("  -  ");
                textColor(LIGHT_RED);
                printf("<NO>");
            }


            break;
        default:
            break;
    }

    return(0);
}


char* strstr(const char* str1, const char* str2)
{
    const char* p1 = str1;
    const char* p2;
    while (*str1)
    {
        p2 = str2;
        while (*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }
        if (*p2 == 0)
        {
            return (char*)str1;
        }
        ++str1;
        p1 = str1;
    }
    return 0;
}

// http://www.c-howto.de/tutorial-strings-zeichenketten-uebungen-loesung-teil4-string-replace.html
// Modified by Cuervo
char * stringReplace(char *search, char *replace, char *string) {
    char *tempString, *searchStart;
    int len=0;


    // preuefe ob Such-String vorhanden ist
    searchStart = strstr(string, search);
    if (searchStart == NULL) {
        return string;
    }

    // Speicher reservieren
    tempString = (char*) malloc(strlen(string) * sizeof(char), 0, "stringReplace");
    if (tempString == NULL) {
        return NULL;
    }

    // temporaere Kopie anlegen
    strcpy(tempString, string);

    // ersten Abschnitt in String setzen
    len = searchStart - string;
    string[len] = '\0';

    // zweiten Abschnitt anhaengen
    strcat(string, replace);

    // dritten Abschnitt anhaengen
    len += strlen(search);
    strcat(string, (char*)tempString+len);

    // Speicher freigeben
    free(tempString);


    // Rekursives Aufrufen (neu von Cuervo)
    searchStart = strstr(string, search);
    if (searchStart == NULL) {
        return string;
    } else {
        return stringReplace(search, replace, string);
    }
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
