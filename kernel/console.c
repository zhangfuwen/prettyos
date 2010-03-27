/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "console.h"
#include "kheap.h"
#include "task.h"
#include "my_stdarg.h"

console_t* reachableConsoles[11]; // Mainconsole + up to 10 Subconsoles
uint8_t displayedConsole = 10;    // Currently visible console (10 per default, because console 10 is the kernels console)

extern uint16_t* vidmem;

void console_init(volatile console_t* console, const char* name)
{
    console->name         = malloc(strlen(name), PAGESIZE);
    console->vidmem       = malloc(COLUMNS*USER_LINES*2, PAGESIZE);
    console->csr_x        = 0;
    console->csr_y        = 0;
    console->attrib       = 0x0F;
    strcpy(console->name, name);
    memsetw (console->vidmem, 0x20 | (console->attrib << 8), COLUMNS * USER_LINES * 2);
}
void console_exit(volatile console_t* console)
{
    free(console->vidmem);
    free(console->name);
}

bool changeDisplayedConsole(uint8_t ID)
{
    // Chaning visible console, returning false, if this console is not available.
    if(ID > 11 || reachableConsoles[ID] == 0) {
        return(false);
    }
    displayedConsole = ID;
    refreshUserScreen();
    update_cursor();
    return(true);
}

void clear_console(uint8_t backcolor)
{
    // Erasing the content of the active console
    current_console->attrib = (backcolor << 4) | 0x0F;
    memsetw (current_console->vidmem, 0x20 | (current_console->attrib << 8), COLUMNS * USER_LINES * 2);
    current_console->csr_x = 0;
    current_console->csr_y = 0;
    if(current_console == reachableConsoles[displayedConsole]) // If it is also displayed at the moment, refresh screen
    {
        refreshUserScreen();
    }
}

void settextcolor(uint8_t forecolor, uint8_t backcolor)
{
    // Hi 4 bit: background, low 4 bit: foreground
    current_console->attrib = (backcolor << 4) | (forecolor & 0x0F);
}

void move_cursor_right()
{
    ++current_console->csr_x;
    if (current_console->csr_x>=COLUMNS)
    {
      ++current_console->csr_y;
      current_console->csr_x=0;
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

    switch(uc) {
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
            break;
        case '\r': // cr: cursor back to the margin
            current_console->csr_x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment csr_y
            current_console->csr_x = 0; ++current_console->csr_y;
            break;
        default:
            if (uc != 0)
            {
                uint32_t att = current_console->attrib << 8;
                if(reachableConsoles[displayedConsole] == current_console) { //print to screen
                    *(vidmem + (current_console->csr_y+2) * COLUMNS + current_console->csr_x) = uc | att; // character AND attributes: color
                }
                *(current_console->vidmem + current_console->csr_y * COLUMNS + current_console->csr_x) = uc | att; // character AND attributes: color
                ++current_console->csr_x;
            }
            break;
    }

    if (current_console->csr_x >= COLUMNS) // cursor reaches edge of the screen's width, a new line is inserted
    {
        current_console->csr_x = 0;
        ++current_console->csr_y;
    }

    scroll();
    update_cursor();
}

void puts(const char* text)
{
    for (; *text; putch(*text), ++text);
}

void scroll()
{
    uint32_t blank = 0x20 | (current_console->attrib << 8);
    if (current_console->csr_y >= USER_LINES)
    {
        uint8_t temp = current_console->csr_y - USER_LINES + 1;
        memcpy (current_console->vidmem, current_console->vidmem + temp * COLUMNS, (USER_LINES - temp) * COLUMNS * sizeof(uint16_t));
        memsetw (current_console->vidmem + (USER_LINES - temp) * COLUMNS, blank, COLUMNS);
        current_console->csr_y = USER_LINES - 1;
        refreshUserScreen();
    }
}

/// TODO: make it standardized !
// printf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c
void printf (const char* args, ...)
{
    va_list ap;
    va_start (ap, args);
    char buffer[32]; // Larger is not needed at the moment

    for (; *args; ++args)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        itoa(va_arg(ap, uint32_t), buffer);
                        puts(buffer);
                        break;
                    case 'f':
                        float2string(va_arg(ap, double), 10, buffer);
                        puts(buffer);
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), buffer);
                        puts(buffer);
                        break;
                    case 'X':
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
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
