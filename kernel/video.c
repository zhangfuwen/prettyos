#include "os.h"
#include "my_stdarg.h"

static uint8_t  csr_x  = 0;
static uint8_t  csr_y  = 0;
static uint8_t  saved_csr_x  = 0;
static uint8_t  saved_csr_y  = 0;
static uint8_t  attrib = 0x0F;
static uint8_t  saved_attrib = 0x0F;

static uint16_t* vidmem = (uint16_t*) 0xB8000;
static const uint8_t COLUMNS =  80;
static const uint8_t LINES   =  50;
static const uint8_t SCROLL_LINE = 45; // reserve line for the status bar and ascii-art
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
    // Top 4 bytes: background, bottom 4 bytes: foreground color
    attrib = (backcolor << 4) | (forecolor & 0x0F);
}

void move_cursor_right()
{
    ++csr_x;
    if(csr_x>=COLUMNS)
    {
      ++csr_y;
      csr_x=0;
    }
}

void move_cursor_left()
{
    if(csr_x)
        --csr_x;
    if(!csr_x && csr_y>0)
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
	if(scrollflag)
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

static uint8_t transferFromAsciiToCodepage437(uint8_t ascii)
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
    uint8_t uc = transferFromAsciiToCodepage437((uint8_t)c); // no negative values

    uint16_t* pos;
    uint32_t att = attrib << 8;

    if(uc == 0x08) // backspace: move the cursor one space backwards and delete
    {
        if(csr_x)
        {
            --csr_x;
            putch(' ');
            --csr_x;
        }
        if(!csr_x && csr_y>0)
        {
            csr_x=COLUMNS-1;
            --csr_y;
            putch(' ');
            csr_x=COLUMNS-1;
            --csr_y;
        }
    }
    else if(uc == 0x09) // tab: increment csr_x (divisible by 8)
    {
        csr_x = (csr_x + 8) & ~(8 - 1);
    }
    else if(uc == '\r') // cr: cursor back to the margin
    {
        csr_x = 0;
    }
    else if(uc == '\n') // newline: like 'cr': cursor to the margin and increment csr_y
    {
        csr_x = 0; ++csr_y;
    }

    else if(uc != 0)
    {
        pos = vidmem + (csr_y * COLUMNS + csr_x);
       *pos = uc | att; // character AND attributes: color
        ++csr_x;
    }

    if(csr_x >= COLUMNS) // cursor reaches edge of the screen's width, a new line is inserted
    {
        csr_x = 0;
        ++csr_y;
    }

    // scroll if needed, and finally move the cursor
    if(scrollflag)
    {
        scroll();
    }
    update_cursor();
}

void puts(const char* text)
{
    for(; *text; putch(*text), ++text);
}

void scroll()
{
    uint32_t blank = 0x20 | (attrib << 8);
    if(csr_y >= SCROLL_LINE)
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

// printf(...): supports %u, %d, %x/%X, %s, %c
void printf (const char* args, ...)
{
	va_list ap;
	va_start (ap, args);
	int32_t index = 0, d;
	uint32_t u;
	char  c;
	char* s;
	char buffer[256];

	while (args[index])
	{
		switch (args[index])
		{
		case '%':
			++index;
			switch (args[index])
			{
			case 'u':
				u = va_arg (ap, uint32_t);
				itoa(u, buffer);
				puts(buffer);
				break;
			case 'd':
			case 'i':
				d = va_arg (ap, int32_t);
				itoa(d, buffer);
				puts(buffer);
				break;
			case 'X':
			    d = va_arg (ap, int32_t);
				i2hex(d, buffer,8);
				puts(buffer);
				break;
			case 'x':
			    d = va_arg (ap, int32_t);
				i2hex(d, buffer,4);
				puts(buffer);
				break;
			case 'y':
			    d = va_arg (ap, int32_t);
				i2hex(d, buffer,2);
				puts(buffer);
				break;
			case 's':
				s = va_arg (ap, char*);
				puts(s);
				break;
			case 'c':
				c = (int8_t) va_arg (ap, int32_t);
				putch(c);
				break;
			default:
				putch('%');
				putch('%');
				break;
			}
			break;

		default:
			putch(args[index]); //printf("%c",*(args+index));
			break;
		}
		++index;
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



