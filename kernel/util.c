/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "my_stdarg.h"
#include "sys_speaker.h"

const int32_t INT_MAX = 2147483647;

void sti() { __asm__ volatile ("sti"); }  // Enable interrupts
void cli() { __asm__ volatile ("cli"); }  // Disable interrupts

void nop() { __asm__ volatile ("nop"); }  // Do nothing

oda_t   ODA;
oda_t* pODA = &ODA;

/**********************************************************************/

uint32_t fetchESP()
{
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

uint32_t fetchEBP()
{
    uint32_t ebp;
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
    return ebp;
}

uint32_t fetchSS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ("movl %ss,%eax");
    return eax;
}

uint32_t fetchCS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ("movl %cs,%eax");
    return eax;
}

uint32_t fetchDS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ("movl %ds,%eax");
    return eax;
}

uint64_t rdtsc()
{
    uint64_t val;
    __asm__ volatile ("rdtsc" : "=A"(val)); // "=A" for getting 64 bit value
    return val;
}

/**********************************************************************/

uint8_t inportb(uint16_t port)
{
    uint8_t ret_val;
    __asm__ volatile ("in %%dx,%%al" : "=a"(ret_val) : "d"(port));
    return ret_val;
}

uint16_t inportw(uint16_t port)
{
    uint16_t ret_val;
    __asm__ volatile ("in %%dx,%%ax" : "=a" (ret_val) : "d"(port));
    return ret_val;
}

uint32_t inportl(uint16_t port)
{
    uint32_t ret_val;
    __asm__ volatile ("in %%dx,%%eax" : "=a" (ret_val) : "d"(port));
    return ret_val;
}

void outportb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("out %%al,%%dx" :: "a"(val), "d"(port));
}

void outportw(uint16_t port, uint16_t val)
{
    __asm__ volatile ("out %%ax,%%dx" :: "a"(val), "d"(port));
}

void outportl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}

/**********************************************************************/

void panic_assert(const char* file, uint32_t line, const char* desc) // why char ?
{
    cli();
    printf("ASSERTION FAILED(");
    printf("%s",desc);
    printf(") at ");
    printf("%s",file);
    printf(":");
    printf("%i",line);
    printf("OPERATING SYSTEM HALTED\n");
    // Halt by going into an infinite loop.
    for (;;);
}

/**********************************************************************/

void memshow(void* start, size_t count)
{
    const uint8_t* end = (const uint8_t*)(start+count);
    for (; count != 0; count--) printf("%x ",*(end-count));
}

void* memcpy(void* dest, const void* src, size_t count)
{
    const uint8_t* sp = (const uint8_t*)src;
    uint8_t* dp = (uint8_t*)dest;
    for (; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void* memset(void* dest, int8_t val, size_t count)
{
    int8_t* temp = (int8_t*)dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

uint16_t* memsetw(uint16_t* dest, uint16_t val, size_t count)
{
    uint16_t* temp = dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

uint32_t* memsetl(uint32_t* dest, uint32_t val, size_t count)
{
    uint32_t* temp = dest;
    for (; count != 0; count--) *temp++ = val;
    return dest;
}

/**********************************************************************/

void sprintf (char *buffer, const char *args, ...)
{
    va_list ap;
    va_start (ap, args);
    int pos = 0;
    char m_buffer[32]; // Larger is not needed at the moment
    buffer[0] = '\0';

    for (; *args; args++)
    {
        switch (*args)
        {
            case '%':
                switch (*(++args))
                {
                    case 'u':
                        itoa(va_arg(ap, uint32_t), m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'f':
                        float2string(va_arg(ap, double), 8, m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'i': case 'd':
                        itoa(va_arg(ap, int32_t), m_buffer);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'X':
                        i2hex(va_arg(ap, int32_t), m_buffer,8);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'x':
                        i2hex(va_arg(ap, int32_t), m_buffer,4);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 'y':
                        i2hex(va_arg(ap, int32_t), m_buffer,2);
                        strcat(buffer, m_buffer);
                        pos += strlen(m_buffer) - 1;
                        break;
                    case 's':
                        strcat(buffer, va_arg (ap, char*));
                        pos = strlen(buffer) - 1;
                        break;
                    case 'c':
                        buffer[pos] = (int8_t)va_arg(ap, int32_t);
                        break;
                    case '%':
                        buffer[pos] = '%';
                        break;
                    default:
                        --args;
                        --pos;
                        break;
                    }
                break;
            default:
                buffer[pos] = (*args);
                break;
        }
        pos++;
        buffer[pos] = '\0';
    }
}

size_t strlen(const char* str)
{
    size_t retval;
    for (retval = 0; *str != '\0'; ++str)
        ++retval;
    return retval;
}

// Compare two strings. Returns -1 if str1 < str2, 0 if they are equal or 1 otherwise.
int32_t strcmp(const char* s1, const char* s2)
{
    while ((*s1) && (*s1 == *s2))
    {
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}

/// http://en.wikipedia.org/wiki/Strcpy
// Copy the NUL-terminated string src into dest, and return dest.
char* strcpy(char* dest, const char* src)
{
   char* save = dest;
   while ((*dest++ = *src++));
   return save;
}

char* strncpy(char* dest, const char* src, unsigned int n) // okay?
{
    if (n != 0)
    {
        char* d       = dest;
        const char* s = src;
        do
        {
            if ((*d++ = *s++) == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                   *d++ = 0;
                break;
            }
        }
        while (--n != 0);
     }
     return (dest);
}


/// http://en.wikipedia.org/wiki/Strcat
char* strcat(char* dest, const char* src)
{
    strcpy(dest + strlen(dest), src);
    return dest;
}

/**********************************************************************/

/// http://en.wikipedia.org/wiki/Itoa
void reverse(char* s)
{
    char c;

    for (int32_t i=0, j=strlen(s)-1; i<j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

int8_t ctoi(char c) {
    if (c < 48 || c > 57) {
        return(-1);
    }
    return(c-48);
}

/// http://en.wikipedia.org/wiki/Itoa
void itoa(int32_t n, char* s)
{
    int32_t i, sign;
    if ((sign = n) < 0)  // record sign
    {
        n = -n;         // make n positive
    }
    i=0;
    do /* generate digits in reverse order */
    {
        s[i++] = n % 10 + '0';  // get next digit
    }
    while ((n /= 10) > 0);     // delete it

    if (sign < 0)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

void i2hex(uint32_t val, char* dest, int32_t len)
{
    char* cp;
    char  x;
    uint32_t n;
    n = val;
    cp = &dest[len];
    while (cp > dest)
    {
        x = n & 0xF;
        n >>= 4;
        *--cp = x + ((x > 9) ? 'A' - 10 : '0');
    }
    dest[len]  ='h';
    dest[len+1]='\0';
}

void float2string(float value, int32_t decimal, char* valuestring) // float --> string
{
    int32_t neg = 0;
    char tempstr[20];
    int32_t i = 0;
    int32_t j = 0;
    int32_t c;
    int32_t val1, val2;
    char* tempstring;

    tempstring = valuestring;
    if (value < 0)
    {
        neg = 1; value = -value;
    }
    for (j=0; j < decimal; ++j)
    {
        value = value * 10;
    }
    val1 = (value * 2);
    val2 = (val1 / 2) + (val1 % 2);
    while (val2 !=0)
    {
        if ((decimal > 0) && (i == decimal))
        {
            tempstr[i] = (char)(0x2E);
            ++i;
        }
        else
        {
            c = (val2 % 10);
            tempstr[i] = (char) (c + 0x30);
            val2 = val2 / 10;
            ++i;
        }
    }
    if (neg)
    {
        *tempstring = '-';
        ++tempstring;
    }
    i--;
    for (;i > -1;i--)
    {
        *tempstring = tempstr[i];
        ++tempstring;
    }
    *tempstring = '\0';
}

uint32_t alignUp(uint32_t val, uint32_t alignment)
{
    if (! alignment)
        return val;
    --alignment;
    return (val+alignment) & ~alignment;
}

uint32_t alignDown(uint32_t val, uint32_t alignment)
{
    if (! alignment)
        return val;
    return val & ~(alignment-1);
}

uint8_t PackedBCD2Decimal(uint8_t PackedBCDVal)
{
    return ((PackedBCDVal >> 4) * 10 + (PackedBCDVal & 0xF));
}

/**********************************************************************/

void reboot()
{
    int32_t temp; // A temporary int for storing keyboard info. The keyboard is used to reboot
    do //flush the keyboard controller
    {
       temp = inportb(0x64);
       if (temp & 1)
         inportb(0x60);
    }
    while (temp & 2);

    // Reboot
    outportb(0x64, 0xFE);
}

// BOOTSCREEN

void bootscreen() {
    printf("\n\n");
    settextcolor(14,0);
    printf("       ");
    settextcolor(8,0);
    printf("######                    ##    ##               #####       ####\n");
    printf("      ");
    settextcolor(15,0);
    printf("######");
    settextcolor(8,0);
    printf("##                  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#              ");
    settextcolor(15,0);
    printf("#####");
    settextcolor(8,0);
    printf("##     ");
    settextcolor(15,0);
    printf("####");
    settextcolor(8,0);
    printf("##\n");
    printf("      ");
    settextcolor(15,0);
    printf("#######");
    settextcolor(8,0);
    printf("#                  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#             ");
    settextcolor(15,0);
    printf("#######");
    settextcolor(8,0);
    printf("##   ");
    settextcolor(15,0);
    printf("######");
    settextcolor(8,0);
    printf("#\n");
    printf("      ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ## ##    ####  #");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("###");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#####    ##  ");
    settextcolor(15,0);
    printf("###   ###");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##\n");
    printf("      ##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  #");
    settextcolor(15,0);
    printf("####");
    settextcolor(8,0);
    printf("##");
    settextcolor(15,0);
    printf("##############");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#     ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("##\n");
    printf("     ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##  #####  #######");
    settextcolor(8,0);
    printf("#");
    settextcolor(15,0);
    printf("##############");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##  ##      ##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("##\n");
    printf("     ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("##");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("####    ##");
    settextcolor(8,0);
    printf("###");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("## ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#      ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("###\n");
    printf("     ");
    settextcolor(15,0);
    printf("#######  ###");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("########  ##");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#      ");
    settextcolor(15,0);
    printf("##     ####");
    settextcolor(8,0);
    printf("#\n");
    printf("     ");
    settextcolor(15,0);
    printf("######   ###    ######    ##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#");
    settextcolor(15,0);
    printf("###  ##");
    settextcolor(8,0);
    printf("#     ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#       ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#\n");
    printf("     ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#      ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#    ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#      ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("##    ");
    settextcolor(15,0);
    printf("##   ");
    settextcolor(8,0);
    printf("##   ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#\n");
    printf("    ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("#      ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#    ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("###### ");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("###");
    settextcolor(15,0);
    printf("###");
    settextcolor(8,0);
    printf("##  ");
    settextcolor(15,0);
    printf("#####   ###");
    settextcolor(8,0);
    printf("####");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("##");
    settextcolor(8,0);
    printf("###");
    settextcolor(15,0);
    printf("###\n");
    printf("    ###      ###     #######");
    settextcolor(8,0);
    printf("# ");
    settextcolor(15,0);
    printf("##### ####");
    settextcolor(8,0);
    printf("#  ");
    settextcolor(15,0);
    printf("####     ########   #######\n");
    printf("    ##       ##       ######   ###   ###   ###       #####      #####\n");
    printf("                                           ##\n");
    printf("                                          ##\n");
    printf("                                         ##\n");    
    printf("\n\n");
    settextcolor(14,0);
    printf("                     Copyright (c) 2010 The PrettyOS Team\n");
    printf("\n\n");
    settextcolor(15,0);
    // Melody
    // C Es F G F Es
    // C E F G F E C
    // http://www.flutepage.de/deutsch/goodies/frequenz.shtml (German)
    // http://www.flutepage.de/englisch/goodies/frequenz.shtml (English)
    beep(523,200); // C
    printf("                     This");
    beep(622,200); // Es
    printf(" bootscreen");
    beep(689,200); // F
    printf(" has");
    beep(784,200); // G
    printf(" been");
    beep(689,200); // F
    printf(" created");
    beep(622,200); // Es
    printf(" by\n\n");
    
    beep(523,200); // C
    printf("                     Cuervo,");
    beep(659,200); // E
    printf(" member");
    beep(689,200); // F
    printf(" of the");
    beep(784,200); // G
    printf(" PrettyOS");
    beep(689,200); // F
    printf(" team");
    beep(659,200); // E
    
    beep(523,1000); // C
    settextcolor(15,0);
    printf("\n\n\n\n\n");
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
