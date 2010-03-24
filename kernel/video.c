/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "my_stdarg.h"

static uint8_t  csr_x        = 0;
static uint8_t  csr_y        = 0;
static uint8_t  saved_csr_x  = 0;
static uint8_t  saved_csr_y  = 0;
static uint8_t  attrib       = 0x0F;
static uint8_t  saved_attrib = 0x0F;

static uint16_t* vidmem = (uint16_t*) 0xB8000;

static const uint8_t COLUMNS = 80;
static const uint8_t LINES   = 50;

// reserve 5 lines:
// one line:     separation,
// three lines:  info area,
// one line:     status bar
static const uint8_t SCROLL_LINE = 45;

static bool scrollflag = true;

void clear_screen()
{
    memsetw (vidmem, 0x20 | (attrib << 8), COLUMNS * LINES);
    csr_x = 0;
    csr_y = 0;
    update_cursor();
}

void clear_userscreen(uint8_t backcolor)
{
    attrib = (backcolor << 4) | 0x0F;
    memsetw (vidmem, 0x20 | (attrib << 8), COLUMNS * (SCROLL_LINE));
    csr_x = 0;
    csr_y = 0;
    update_cursor();
}

void settextcolor(uint8_t forecolor, uint8_t backcolor)
{
    // Hi 4 bit: background, low 4 bit: foreground
    attrib = (backcolor << 4) | (forecolor & 0x0F);
}

void move_cursor_right()
{
    ++csr_x;
    if (csr_x>=COLUMNS)
    {
      ++csr_y;
      csr_x=0;
    }
}

void move_cursor_left()
{
    if (csr_x)
        --csr_x;
    if (!csr_x && csr_y>0)
    {
        csr_x=COLUMNS-1;
        --csr_y;
    }
}

void move_cursor_home()
{
    csr_x = 0; update_cursor();
}

void move_cursor_end()
{
    csr_x = COLUMNS-1; update_cursor();
}

void set_cursor(uint8_t x, uint8_t y)
{
    csr_x = x; csr_y = y; update_cursor();
}

void update_cursor()
{
    uint16_t position;
    if (scrollflag)
    {
        position = csr_y * COLUMNS + csr_x;
    }
    else
    {
        position = saved_csr_y * COLUMNS + saved_csr_x;
    }
    // cursor HIGH port to vga INDEX register
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (uint8_t)((position>>8)&0xFF));
    // cursor LOW port to vga INDEX register
    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (uint8_t)(position&0xFF));
}

static uint8_t AsciiToCP437(uint8_t ascii)
{
    switch ( ascii )
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
        default:    return ascii; // to be checked for more deviations
    }
}

void putch(char c)
{
    uint8_t uc = AsciiToCP437((uint8_t)c); // no negative values

    uint16_t* pos;
    uint32_t att = attrib << 8;

    if (uc == 0x08) // backspace: move the cursor one space backwards and delete
    {
        if (csr_x)
        {
            --csr_x;
            putch(' ');
            --csr_x;
        }
        if (!csr_x && csr_y>0)
        {
            csr_x=COLUMNS-1;
            --csr_y;
            putch(' ');
            csr_x=COLUMNS-1;
            --csr_y;
        }
    }
    else if (uc == 0x09) // tab: increment csr_x (divisible by 8)
    {
        csr_x = (csr_x + 8) & ~(8 - 1);
    }
    else if (uc == '\r') // cr: cursor back to the margin
    {
        csr_x = 0;
    }
    else if (uc == '\n') // newline: like 'cr': cursor to the margin and increment csr_y
    {
        csr_x = 0; ++csr_y;
    }

    else if (uc != 0)
    {
        pos = vidmem + (csr_y * COLUMNS + csr_x);
       *pos = uc | att; // character AND attributes: color
        ++csr_x;
    }

    if (csr_x >= COLUMNS) // cursor reaches edge of the screen's width, a new line is inserted
    {
        csr_x = 0;
        ++csr_y;
    }

    // scroll if needed, and finally move the cursor
    if (scrollflag)
    {
        scroll();
    }
    update_cursor();
}

void puts(const char* text)
{
    for (; *text; putch(*text), ++text);
}

void scroll()
{
    uint32_t blank = 0x20 | (attrib << 8);
    if (csr_y >= SCROLL_LINE)
    {
        uint8_t temp = csr_y - SCROLL_LINE + 1;
        memcpy (vidmem, vidmem + temp * COLUMNS, (SCROLL_LINE - temp) * COLUMNS * 2);
        memsetw (vidmem + (SCROLL_LINE - temp) * COLUMNS, blank, COLUMNS);
        csr_y = SCROLL_LINE - 1;
    }
}

void kprintf(const char* message, uint32_t line, uint8_t attribute)
{
    save_cursor();
    // Top 4 bytes: background, bottom 4 bytes: foreground color
    settextcolor(attribute & 0x0F, attribute >> 4);
    csr_x = 0; csr_y = line;
    update_cursor();
    scrollflag = false;
    puts(message);
    scrollflag = true;
    restore_cursor();
};

/// TODO: make it standardized !
// printf(...): supports %u, %d/%i, %f, %y/%x/%X, %s, %c
void printf (const char* args, ...)
{
    va_list ap;
    va_start (ap, args);
    char buffer[32]; // Larger is not needed at the moment

    for (; *args; args++)
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
                putch(*args); //printf("%c",*(args+index));
                break;
        }
    }
}

void save_cursor()
{
    pODA->ts_flag=0;
    saved_csr_x  = csr_x;
    saved_csr_y  = csr_y;
    saved_attrib = attrib;
}

void restore_cursor()
{
    csr_x  = saved_csr_x;
    csr_y  = saved_csr_y;
    attrib = saved_attrib;
    pODA->ts_flag=1;
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
