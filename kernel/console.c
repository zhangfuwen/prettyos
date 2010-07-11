/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "console.h"
#include "util.h"
#include "kheap.h"
#include "synchronisation.h"
#include "video.h"

console_t* reachableConsoles[11]; // Mainconsole + up to KERNELCONSOLE_ID Subconsoles
uint8_t displayedConsole = KERNELCONSOLE_ID; // Currently visible console (KERNELCONSOLE_ID per default)

extern uint16_t* vidmem;
bool scroll_flag = true;

void kernel_console_init()
{
    current_console = malloc(sizeof(console_t), 0); // Reserving space for the kernels console
    console_init(current_console, "");
    reachableConsoles[KERNELCONSOLE_ID] = current_console;
    current_console->SCROLL_END = 39;
    current_console->showInfobar = true;
}

void console_init(console_t* console, const char* name)
{
    console->name         = malloc(strlen(name)+1, 0);
    console->vidmem       = malloc(COLUMNS*USER_LINES*2, 0);
    console->csr_x        = 0;
    console->csr_y        = 0;
    //console->SCROLL_BEGIN = 0;
    console->SCROLL_END   = USER_LINES;
    console->showInfobar  = false;
    console->sp = semaphore_create(1);
    strcpy(console->name, name);
    memsetw(console->vidmem, 0x00, COLUMNS * USER_LINES);
    // Setup the keyqueue
    memset(console->KQ.buffer, 0, KQSIZE);
    console->KQ.pHead = console->KQ.buffer;
    console->KQ.pTail = console->KQ.buffer;
    console->KQ.count_read  = 0;
    console->KQ.count_write = 0;
}
void console_exit(console_t* console)
{
    free(console->vidmem);
    free(console->name);
    semaphore_delete(console->sp);
}

bool changeDisplayedConsole(uint8_t ID)
{
    // Changing visible console, returning false, if this console is not available.
    if (ID > 11 || reachableConsoles[ID] == 0)
    {
        return(false);
    }
    displayedConsole = ID;
    refreshUserScreen();
    return(true);
}
void setScrollField(uint8_t begin, uint8_t end)
{
    //current_console->SCROLL_BEGIN = begin;
    current_console->SCROLL_END = end;
    scroll();
}

void showInfobar(bool show)
{
    current_console->showInfobar = show;
    current_console->SCROLL_END = min(current_console->SCROLL_END, 42);
    refreshUserScreen();
}

void clear_console(uint8_t backcolor)
{
    // Erasing the content of the active console
    currentTask->attrib = (backcolor << 4) | 0x0F;
    memsetw(current_console->vidmem, 0x20 | (currentTask->attrib << 8), COLUMNS * USER_LINES);
    current_console->csr_x = 0;
    current_console->csr_y = 0;
    if (current_console == reachableConsoles[displayedConsole]) // If it is also displayed at the moment, refresh screen
    {
        refreshUserScreen();
    }
}

void textColor(uint8_t color) // bit 0-3: background; bit 4-7: foreground
{
    currentTask->attrib = color;
}

void move_cursor_right()
{
    ++current_console->csr_x;
    if (current_console->csr_x >= COLUMNS)
    {
        ++current_console->csr_y;
        current_console->csr_x = 0;
        scroll();
    }
}

void move_cursor_left()
{
    if (current_console->csr_x)
        --current_console->csr_x;
    if (!current_console->csr_x && current_console->csr_y>0)
    {
        current_console->csr_x=COLUMNS-1;
        --current_console->csr_y;
    }
}

void move_cursor_home()
{
    current_console->csr_x = 0;
    update_cursor();
}

void move_cursor_end()
{
    current_console->csr_x = COLUMNS-1;
    update_cursor();
}

void set_cursor(uint8_t x, uint8_t y)
{
    current_console->csr_x = x; current_console->csr_y = y;
    update_cursor();
}
void putch(char c)
{
    uint8_t uc = AsciiToCP437((uint8_t)c); // no negative values

    switch (uc) {
        case 0x08: // backspace: move the cursor one space backwards and delete
            if (current_console->csr_x)
            {
                --current_console->csr_x;
                putch(' ');
                --current_console->csr_x;
            }
            else if (current_console->csr_y > 0)
            {
                current_console->csr_x = COLUMNS-1;
                --current_console->csr_y;
                putch(' ');
                current_console->csr_x = COLUMNS-1;
                --current_console->csr_y;
            }
            break;
        case 0x09: // tab: increment csr_x (divisible by 8)
            current_console->csr_x = (current_console->csr_x + 8) & ~(8 - 1);
            if (current_console->csr_x>=COLUMNS)
            {
                ++current_console->csr_y;
                current_console->csr_x=0;
                scroll();
            }
            break;
        case '\r': // cr: cursor back to the margin
            move_cursor_home();
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment csr_y
            ++current_console->csr_y; scroll(); move_cursor_home();
            scroll();
            break;
        default:
            if (uc != 0)
            {
                uint32_t att = currentTask->attrib << 8;
                if (reachableConsoles[displayedConsole] == current_console) { //print to screen
                    *(vidmem + (current_console->csr_y+2) * COLUMNS + current_console->csr_x) = uc | att; // character AND attributes: color
                }
                *(current_console->vidmem + current_console->csr_y * COLUMNS + current_console->csr_x) = uc | att; // character AND attributes: color
                move_cursor_right();
            }
            break;
    }
}

void puts(const char* text)
{
    for (; *text; putch(*text), ++text);
}

void scroll()
{
    uint8_t scroll_end = min(USER_LINES, current_console->SCROLL_END);
    if (scroll_flag && current_console->csr_y >= scroll_end)
    {
        uint8_t temp = current_console->csr_y - scroll_end + 1;
        memcpy(current_console->vidmem, current_console->vidmem + temp * COLUMNS, (scroll_end - temp) * COLUMNS * sizeof(uint16_t));
        memsetw(current_console->vidmem + (scroll_end - temp) * COLUMNS, currentTask->attrib << 8, COLUMNS);
        current_console->csr_y = scroll_end - 1;
        refreshUserScreen();
    }
}

/// TODO: make it standardized !
// vprintf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c and the PrettyOS-specific %v
void vprintf(const char* args, va_list ap)
{
    uint8_t attribute = currentTask->attrib;
    char buffer[32]; // Larger is not needed at the moment

    // semaphore_lock(currentTask->console->sp); // TEST
    for (; *args; ++args)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        utoa(va_arg(ap, uint32_t), buffer);
                        puts(buffer);
                        break;
                    case 'f':
                        ftoa(va_arg(ap, double), buffer);
                        puts(buffer);
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), buffer);
                        puts(buffer);
                        break;
                    case 'X': /// TODO: make it standardized
                        i2hex(va_arg(ap, int32_t), buffer, 8);
                        puts(buffer);
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), buffer, 4);
                        puts(buffer);
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), buffer, 2);
                        puts(buffer);
                        break;
                    case 's':
                        puts(va_arg (ap, char*));
                        break;
                    case 'c':
                        putch((int8_t)va_arg(ap, int32_t));
                        break;
                    case 'v':
                        currentTask->attrib = (attribute >> 4) | (attribute << 4);
                        putch(*(++args));
                        currentTask->attrib = attribute;
                        break;
                    case '%':
                        putch('%');
                        break;
                    default:
                        --args;
                        break;
                }
                break;
            default:
                putch(*args);
                break;
        }
    }
    // semaphore_unlock(currentTask->console->sp); // TEST
}
void printf(const char* args, ...)
{
    va_list ap;
    va_start (ap, args);
    vprintf(args, ap);
}

void cprintf(const char* message, uint32_t line, uint8_t attribute, ...)
{
    uint8_t old_attrib = currentTask->attrib;
    uint8_t c_x = current_console->csr_x;
    uint8_t c_y = current_console->csr_y;
    scroll_flag = false;

    currentTask->attrib = attribute;
    current_console->csr_x = 0; current_console->csr_y = line;

    // Call usual printf routines
    va_list ap;
    va_start (ap, attribute);
    vprintf(message, ap);

    scroll_flag = true;
    currentTask->attrib = old_attrib;
    current_console->csr_x = c_x;
    current_console->csr_y = c_y;
}

void update_cursor()
{
    uint16_t position = (current_console->csr_y+2) * COLUMNS + current_console->csr_x;
    // cursor HIGH port to vga INDEX register
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (uint8_t)((position>>8)&0xFF));
    // cursor LOW port to vga INDEX register
    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (uint8_t)(position&0xFF));
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
