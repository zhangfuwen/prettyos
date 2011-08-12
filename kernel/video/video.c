/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "video.h"
#include "console.h"
#include "util.h"
#include "paging.h"
#include "filesystem/fsmanager.h"
#include "synchronisation.h"

#define SCREENSHOT_BYTES 4102

static uint16_t* vidmem = (uint16_t*)0xB8000;

VIDEOMODES videomode = VM_TEXT;

static char infoBar[3][81]; // Infobar with 3 lines and 80 columns

static const uint8_t LINES      = 50;
static const uint8_t USER_BEGIN =  2; // Reserving  Titlebar + Separation
static const uint8_t USER_END   = 48; // Reserving Statusbar + Separation

static position_t cursor = {0, 0};

static mutex_t* videoLock = 0;


void vga_install()
{
    videoLock = mutex_create(1);
    vidmem = paging_acquirePciMemory(0xB8000, 2);
}

void vga_setPixel(uint8_t x, uint8_t y, uint16_t value)
{
    mutex_lock(videoLock);
    vidmem[y*COLUMNS + x] = value;
    mutex_unlock(videoLock);
}

void clear_screen()
{
    mutex_lock(videoLock);
    memset(vidmem, 0x00, COLUMNS * LINES * 2);
    mutex_unlock(videoLock);
}

// http://de.wikipedia.org/wiki/Codepage_437
uint8_t AsciiToCP437(uint8_t ascii)
{
    switch (ascii)
    {
        case 0xE4:  return 0x84;  // ä
        case 0xF6:  return 0x94;  // ö
        case 0xFC:  return 0x81;  // ü
        case 0xDF:  return 0xE1;  // ß
        case 0xA7:  return 0x15;  // §
        case 0xB0:  return 0xF8;  // °
        case 0xC4:  return 0x8E;  // Ä
        case 0xD6:  return 0x99;  // Ö
        case 0xDC:  return 0x9A;  // Ü
        case 0xB2:  return 0xFD;  // ²
        case 0xB3:  return 0x00;  // ³ <-- not available
        case 0x80:  return 0xEE;  // € <-- Greek epsilon used
        case 0xB5:  return 0xE6;  // µ
        case 0xB6:  return 0x14;  // ¶
        case 0xC6:  return 0x92;  // æ
        case 0xE6:  return 0x91;  // Æ
        case 0xF1:  return 0xA4;  // ñ
        case 0xD1:  return 0xA5;  // Ñ
        case 0xE7:  return 0x87;  // ç
        case 0xC7:  return 0x80;  // Ç
        case 0xBF:  return 0xA8;  // ¿
        case 0xA1:  return 0xAD;  // ¡
        case 0xA2:  return 0x0B;  // ¢
        case 0xA3:  return 0x9C;  // £
        case 0xBD:  return 0xAB;  // ½
        case 0xBC:  return 0xAC;  // ¼
        case 0xA5:  return 0x9D;  // ¥
        case 0xF7:  return 0xF6;  // ÷
        case 0xAC:  return 0xAA;  // ¬
        case 0xAB:  return 0xAE;  // «
        case 0xBB:  return 0xAF;  // »
        default:    return ascii; // to be checked for more deviations
    }
}

static void kputch(char c, uint8_t attrib)
{
    uint8_t uc = AsciiToCP437((uint8_t)c); // no negative values

    uint16_t* pos;
    uint16_t att = attrib << 8;

    mutex_lock(videoLock);
    switch (uc)
    {
        case 0x08: // backspace: move the cursor one space backwards and delete
            if (cursor.x)
            {
                --cursor.x;
                kputch(' ', attrib);
                --cursor.x;
            }
            else if (cursor.y>0)
            {
                cursor.x=COLUMNS-1;
                --cursor.y;
                kputch(' ', attrib);
                cursor.x=COLUMNS-1;
                --cursor.y;
            }
            break;
        case 0x09: // tab: increment csr_x (divisible by 8)
            cursor.x = (cursor.x + 8) & ~(8 - 1);
            break;
        case '\r': // cr: cursor back to the margin
            cursor.x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment csr_y
            cursor.x = 0; ++cursor.y;
            break;
        default:
            if (uc != 0)
            {
                pos = vidmem + (cursor.y * COLUMNS + cursor.x);
                *pos = uc | att; // character AND attributes: color
                ++cursor.x;
            }
            break;
    }

    if (cursor.x >= COLUMNS) // cursor reaches edge of the screen's width, a new line is inserted
    {
        cursor.x = 0;
        ++cursor.y;
    }
    mutex_unlock(videoLock);
}

static void kputs(const char* text, uint8_t attrib)
{
    mutex_lock(videoLock);
    for (; *text; kputch(*text, attrib), ++text);
    mutex_unlock(videoLock);
}

void kprintf(const char* message, uint32_t line, uint8_t attribute, ...)
{
    cursor.x = 0; cursor.y = line;

    va_list ap;
    va_start(ap, attribute);
    char buffer[32]; // Larger is not needed at the moment

    mutex_lock(videoLock);
    for (; *message; message++)
    {
        switch (*message)
        {
            case '%':
                switch (*(++message))
                {
                    case 'u':
                        utoa(va_arg(ap, uint32_t), buffer);
                        kputs(buffer, attribute);
                        break;
                    case 'f':
                        ftoa(va_arg(ap, double), buffer);
                        kputs(buffer, attribute);
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), buffer);
                        kputs(buffer, attribute);
                        break;
                    case 'X': /// TODO: make it standardized
                        i2hex(va_arg(ap, int32_t), buffer, 8);
                        kputs(buffer, attribute);
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), buffer, 4);
                        kputs(buffer, attribute);
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), buffer, 2);
                        kputs(buffer, attribute);
                        break;
                    case 's':
                        kputs(va_arg (ap, char*), attribute);
                        break;
                    case 'c':
                        kputch((int8_t)va_arg(ap, int32_t), attribute);
                        break;
                    case 'v':
                        kputch(*(++message), (attribute >> 4) | (attribute << 4));
                        break;
                    case '%':
                        kputch('%', attribute);
                        break;
                    default:
                        --message;
                        break;
                }
                break;
            default:
                kputch(*message, attribute);
                break;
        }
    }
    mutex_unlock(videoLock);
    va_end(ap);
}

static void refreshInfoBar()
{
    memset(vidmem + (USER_BEGIN + USER_LINES - 3) * COLUMNS, 0, 3 * COLUMNS * 2); // Clearing info-area
    kprintf(infoBar[0], 45, 14);
    kprintf(infoBar[1], 46, 14);
    kprintf(infoBar[2], 47, 14);
}

void writeInfo(uint8_t line, const char* args, ...)
{
    va_list ap;
    va_start(ap, args);
    vsnprintf(infoBar[line], 81, args, ap);
    va_end(ap);

    if (console_displayed->showInfobar)
    {
        refreshInfoBar();
    }
}

void refreshUserScreen()
{
    if (console_displayed->autorefresh == true)
    {
        mutex_lock(videoLock);

        // Printing titlebar
        kprintf("PrettyOS [Version %s]                                                            ", 0, TITLEBAR, version);

        if (console_displayed->ID == KERNELCONSOLE_ID)
        {
            cursor.x = COLUMNS - 5;
            kputs("Shell", 0x0C);
        }
        else
        {
            char Buffer[70];
            snprintf(Buffer, 70, "Console %u: %s", console_displayed->ID-1, console_displayed->name);
            cursor.x = COLUMNS - strlen(Buffer);
            cursor.y = 0;
            kputs(Buffer, 0x0C);
        }
        kprintf("--------------------------------------------------------------------------------", 1, 7); // Separation
        if (console_displayed->showInfobar)
        {
            // copying content of visible console to the video-ram
            memcpy(vidmem + USER_BEGIN * COLUMNS, (void*)console_displayed->vidmem, COLUMNS * (USER_LINES-4) * 2);
            kprintf("--------------------------------------------------------------------------------", 44, 7); // Separation
            refreshInfoBar();
        }
        else
        {
            // copying content of visible console to the video-ram
            memcpy(vidmem + USER_BEGIN * COLUMNS, (void*)console_displayed->vidmem, COLUMNS * USER_LINES*2);
        }
        kprintf("--------------------------------------------------------------------------------", 48, 7); // Separation

        cursor.y = console_displayed->cursor.y;
        cursor.x = console_displayed->cursor.x;
        mutex_unlock(videoLock);
        video_updateCursor();
    }
}

void video_updateCursor()
{
    uint16_t position = (console_displayed->cursor.y+2) * COLUMNS + console_displayed->cursor.x;
    // cursor HIGH port to vga INDEX register
    outportb(CRTC_ADDR_REGISTER, CURSOR_LOCATION_HI_REGISTER);
    outportb(CRTC_DATA_REGISTER, BYTE2(position));
    // cursor LOW port to vga INDEX register
    outportb(CRTC_ADDR_REGISTER, CURSOR_LOCATION_LO_REGISTER);
    outportb(CRTC_DATA_REGISTER, BYTE1(position));
}

static uint8_t screenCache[SCREENSHOT_BYTES];
void takeScreenshot()
{
    puts("\nTake screenshot. ");
    int32_t NewLine = 0;

    mutex_lock(videoLock);
    for (uint16_t i=0; i<4000; i++)
    {
        uint16_t j=i+2*NewLine;
        screenCache[j] = *(char*)(vidmem+i); // only signs, no attributes
        if (i%80 == 79)
        {
            screenCache[j+1]= 0xD; // CR
            screenCache[j+2]= 0xA; // LF
            NewLine++;
        }
    }
    mutex_unlock(videoLock);

    // additional newline at the end of the screenshot
    screenCache[SCREENSHOT_BYTES-2]= 0xD; // CR
    screenCache[SCREENSHOT_BYTES-1]= 0xA; // LF
}

extern disk_t* disks[DISKARRAYSIZE]; // HACK
extern bool    readCacheFlag;        // HACK
diskType_t*    ScreenDest = &FLOPPYDISK; // HACK
void saveScreenshot()
{
    readCacheFlag = false; // TODO: solve this problem!
    char Pfad[20];
    for (int i = 0; i < DISKARRAYSIZE; i++) // HACK
    {
        if (disks[i] && disks[i]->type == ScreenDest && (disks[i]->partition[0]->subtype == FS_FAT12 || disks[i]->partition[0]->subtype == FS_FAT16 || disks[i]->partition[0]->subtype == FS_FAT32))
        {
            snprintf(Pfad, 20, "%u:/screen.txt", i+1);
            break;
        }
    }

    file_t* file = fopen(Pfad, "a+");
    if (file) // check for NULL pointer, otherwise #PF
    {
        fwrite(screenCache, 1, SCREENSHOT_BYTES, file);
        fclose(file);
    }
    else
    {
        puts("\nError: file could not be opened!");
    }
    readCacheFlag = true;
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
