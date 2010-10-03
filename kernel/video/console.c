/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "console.h"
#include "util.h"
#include "kheap.h"
#include "task.h"
#include "synchronisation.h"

console_t* reachableConsoles[11]; // Mainconsole + up to KERNELCONSOLE_ID Subconsoles
uint8_t displayedConsole = KERNELCONSOLE_ID; // Currently visible console (KERNELCONSOLE_ID per default)

console_t kernelConsole = {0, true, 0, 39, {0, 0}};

// The console of the active task
console_t* currentConsole = &kernelConsole;

extern uint16_t* vidmem;
static bool scroll_flag = true;

static void scroll();

uint8_t getTextColor()
{
    if(currentTask == 0)
        return(0x0F);
    return(currentTask->attrib);
}

void kernel_console_init()
{
    kernelConsole.SCROLL_END = 39;
    kernelConsole.showInfobar = true;
    kernelConsole.sp = semaphore_create(1);
    memsetw(kernelConsole.vidmem, 0x00, COLUMNS * USER_LINES);

    keyboard_initKQ(&kernelConsole.KQ);

    reachableConsoles[KERNELCONSOLE_ID] = &kernelConsole;
}

void console_init(console_t* console, const char* name)
{
    console->name         = malloc(strlen(name)+1, 0, "console-name");
    console->cursor.x     = 0;
    console->cursor.y     = 0;
    console->SCROLL_BEGIN = 0;
    console->SCROLL_END   = USER_LINES;
    console->showInfobar  = false;
    console->sp = semaphore_create(1);
    strcpy(console->name, name);
    memsetw(console->vidmem, 0x00, COLUMNS * USER_LINES);

    keyboard_initKQ(&console->KQ);

    for (uint8_t i = 0; i < 10; i++)
    { // The next free place in our console-list will be filled with the new console
        if (reachableConsoles[i] == 0)
        {
            reachableConsoles[i] = console;
            changeDisplayedConsole(i); //Switching to the new console
            break;
        }
    }
}
void console_exit(console_t* console)
{
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
    //currentConsole->SCROLL_BEGIN = begin;
    currentConsole->SCROLL_END = end;
    scroll();
}

void showInfobar(bool show)
{
    currentConsole->showInfobar = show;
    currentConsole->SCROLL_END = min(currentConsole->SCROLL_END, 42);
    refreshUserScreen();
}

void clear_console(uint8_t backcolor)
{
    // Erasing the content of the active console
    textColor((backcolor << 4) | 0x0F);
    memsetw(currentConsole->vidmem, 0x20 | (getTextColor() << 8), COLUMNS * USER_LINES);
    currentConsole->cursor.x = 0;
    currentConsole->cursor.y = 0;
    if (currentConsole == reachableConsoles[displayedConsole]) // If it is also displayed at the moment, refresh screen
    {
        refreshUserScreen();
    }
}

void textColor(uint8_t color) // bit 0-3: background; bit 4-7: foreground
{
    if(currentTask)
        currentTask->attrib = color;
}

static void move_cursor_right()
{
    ++currentConsole->cursor.x;
    if (currentConsole->cursor.x >= COLUMNS)
    {
        ++currentConsole->cursor.y;
        currentConsole->cursor.x = 0;
        scroll();
    }
}

static void move_cursor_left()
{
    if (currentConsole->cursor.x)
        --currentConsole->cursor.x;
    else if (currentConsole->cursor.y > 0)
    {
        currentConsole->cursor.x=COLUMNS-1;
        --currentConsole->cursor.y;
    }
}

static void move_cursor_home()
{
    currentConsole->cursor.x = 0;
    update_cursor();
}

void setCursor(position_t pos)
{
    currentConsole->cursor = pos;
    update_cursor();
}

position_t getCursor()
{
    return(currentConsole->cursor);
}

void putch(char c)
{
    uint8_t uc = AsciiToCP437((uint8_t)c); // no negative values

    switch (uc) {
        case 0x08: // backspace: move the cursor one space backwards and delete
            move_cursor_left();
            putch(' ');
            move_cursor_left();
            break;
        case 0x09: // tab: increment cursor.x (divisible by 8)
            currentConsole->cursor.x = (currentConsole->cursor.x + 8) & ~(8 - 1);
            if (currentConsole->cursor.x>=COLUMNS)
            {
                ++currentConsole->cursor.y;
                currentConsole->cursor.x=0;
                scroll();
            }
            break;
        case '\r': // cr: cursor back to the margin
            move_cursor_home();
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment cursor.y
            ++currentConsole->cursor.y; scroll(); move_cursor_home();
            scroll();
            break;
        default:
            if (uc != 0)
            {
                uint32_t att = getTextColor() << 8;
                if (reachableConsoles[displayedConsole] == currentConsole) { //print to screen
                    *(vidmem + (currentConsole->cursor.y+2) * COLUMNS + currentConsole->cursor.x) = uc | att; // character AND attributes: color
                }
                *(currentConsole->vidmem + currentConsole->cursor.y * COLUMNS + currentConsole->cursor.x) = uc | att; // character AND attributes: color
                move_cursor_right();
            }
            break;
    }
}

void puts(const char* text)
{
    for (; *text; putch(*text), ++text);
}

static void scroll()
{
    uint8_t scroll_end = min(USER_LINES, currentConsole->SCROLL_END);
    if (scroll_flag && currentConsole->cursor.y >= scroll_end)
    {
        uint8_t temp = currentConsole->cursor.y - scroll_end + 1;
        memcpy(currentConsole->vidmem, currentConsole->vidmem + temp * COLUMNS, (scroll_end - temp) * COLUMNS * sizeof(uint16_t));
        memsetw(currentConsole->vidmem + (scroll_end - temp) * COLUMNS, getTextColor() << 8, COLUMNS);
        currentConsole->cursor.y = scroll_end - 1;
        refreshUserScreen();
    }
}

/// TODO: make it standardized !
// vprintf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c and the PrettyOS-specific %v
void vprintf(const char* args, va_list ap)
{
    uint8_t attribute = getTextColor();
    char buffer[32]; // Larger is not needed at the moment

    //semaphore_lock(currentTask->console->sp); // TEST
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
                        textColor((attribute >> 4) | (attribute << 4));
                        putch(*(++args));
                        textColor(attribute);
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
    //semaphore_unlock(currentTask->console->sp); // TEST
}
void printf(const char* args, ...)
{
    va_list ap;
    va_start(ap, args);
    vprintf(args, ap);
    va_end(ap);
}

void cprintf(const char* message, uint32_t line, uint8_t attribute, ...)
{
    uint8_t old_attrib = getTextColor();
    uint8_t c_x = currentConsole->cursor.x;
    uint8_t c_y = currentConsole->cursor.y;
    scroll_flag = false;

    textColor(attribute);
    currentConsole->cursor.x = 0; currentConsole->cursor.y = line;

    // Call usual printf routines
    va_list ap;
    va_start(ap, attribute);
    vprintf(message, ap);
    va_end(ap);

    scroll_flag = true;
    textColor(old_attrib);
    currentConsole->cursor.x = c_x;
    currentConsole->cursor.y = c_y;
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
