/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://houbysoft.com/download/ps2mouse.html
// http://forum.osdev.org/viewtopic.php?t=10247
// http://lowlevel.brainsware.org/wiki/index.php/Programmierung_der_PS/2-Maus
// 'ported' and debugged by Cuervo

#include "mouse.h"
#include "util.h"
#include "video/console.h"
#include "irq.h"
#include "video/vbe.h"
#include "video/gui_window.h"

enum {NORMAL, WHEEL, WHEELS5BUTTON} mousetype = NORMAL;

int32_t mouse_x=10; // Mouse X
int32_t mouse_y=10; // Mouse Y
int32_t mouse_zv=0; // Mouse Zv (vertical mousewheel)
int32_t mouse_zh=0; // Mouse Zh (horizontal mousewheel)
bool mouse_lm=0;    // Mouse Left Button
bool mouse_mm=0;    // Mouse Middle Button
bool mouse_rm=0;    // Mouse Right Button
bool mouse_b4=0;    // Mouse Button 4
bool mouse_b5=0;    // Mouse button 5

extern BMPInfo_t cursor_start;
extern BMPInfo_t cursor_end;


static void mouse_wait(uint8_t a_type);
static void mouse_write(int8_t a_write);
static char mouse_read();


void mouse_install()
{
    // Enable the auxiliary mouse device
    mouse_wait(1);
    outportb(0x64, 0xA8);

    // Enable the interrupts
    mouse_wait(1);
    outportb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = (inportb(0x60) | 2);
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, status);

    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read();


    // Wheel-Mode test
    mouse_setsamples(200);
    mouse_setsamples(100);
    mouse_setsamples(80);
    mouse_write(0xF2);
    if (mouse_read() == 0x03)
    {
        mousetype = WHEEL;

        // Wheels-and-5-Button-Mode test
        mouse_setsamples(200);
        mouse_setsamples(200);
        mouse_setsamples(80);
        mouse_write(0xF2);
        if (mouse_read() == 0x04)
        {
            mousetype = WHEELS5BUTTON;
        }
    }


    // Enable the mouse
    mouse_write(0xF4);
    mouse_read();

    // Setup the mouse handler
    irq_installHandler(IRQ_MOUSE, mouse_handler);
}

// Mouse functions
void mouse_handler(registers_t* a_r)
{
    static uint8_t bytecounter = 0;
    static int8_t bytes[4];
    
    bytes[bytecounter] = inportb(0x60); // Receive byte
    switch(bytecounter)
    {
        case 0: // First byte: Left Button | Right Button | Middle Button | 1 | X sign | Y sign | X overflow | Y overflow
            if(bytes[0] & BIT(3)) // Only if this is really the first byte!
            {
                mouse_lm = (bytes[0] & BIT(0));
                mouse_rm = (bytes[0] & BIT(1))>>1;
                mouse_mm = (bytes[0] & BIT(2))>>2;
            }
            else
            {
                bytecounter--;
                printf("Mouse sent unknown package!\n");
            }
            break;
        case 1: // Second byte: X Movement (8 bits)
            mouse_x += bytes[1];
            break;
        case 2: // Third byte: Y Movement (8 bits)
            mouse_y -= bytes[2]; // Attention: Y-movement is counted from bottom!
            break;
        case 3: // Fourth byte: Z movement (4 bits) | 4th Button | 5th Button | 0 | 0
            switch(mousetype)
            {
                case WHEEL:
                    mouse_zv += bytes[3];
                    break;
                case WHEELS5BUTTON:
                    switch(bytes[3] & 0xF)
                    {
                        case 0xE:
                            mouse_zh -= 1;
                            break;
                        case 0xF:
                            mouse_zv -= 1;
                            break;
                        case 0x1:
                            mouse_zv += 1;
                            break;
                        case 0x2:
                            mouse_zh += 1;
                            break;
                    }
                    mouse_b4 = (bytes[3] & BIT(4)) >> 4;
                    mouse_b5 = (bytes[3] & BIT(5)) >> 5;
                    break;
                default: // We do not expect a fourth byte in this case
                    bytecounter--;
                    printf("Mouse sent unknown package!\n");
                    break;
            }
            break;
    }


    bytecounter++;
    switch(mousetype) // reset packetcounter when received all expected packets
    {
        case WHEEL: case WHEELS5BUTTON:
            if(bytecounter > 3)
                bytecounter = 0;
            break;
        case NORMAL: default:
            if(bytecounter > 2)
                bytecounter = 0;
            break;
    }


    // Print mouse on screen
    if(videomode == VM_VBE)
    {
        mouse_x = max(0, min(mouse_x, getCurrentMIB()->XResolution-1)); // clamp mouse position to width of screen
        mouse_y = max(0, min(mouse_y, getCurrentMIB()->YResolution-1)); // same with height
        vbe_drawBitmapTransparent(mouse_x, mouse_y, &cursor_start);
    }
    else
    {
        switch(mousetype)
        {
            case NORMAL:
                writeInfo(1, "Mouse: X: %d  Y: %d  Z: -   buttons: L: %d  M: %d  R: %d",
                    mouse_x, mouse_y, mouse_lm, mouse_mm, mouse_rm);
                break;
            case WHEEL:
                writeInfo(1, "Mouse: X: %d  Y: %d  Z: %d   buttons: L: %d  M: %d  R: %d",
                    mouse_x, mouse_y, mouse_zv, mouse_lm, mouse_mm, mouse_rm);
                break;
            case WHEELS5BUTTON:
                writeInfo(1, "Mouse: X: %d  Y: %d  Zv: %d  Zh: %d   buttons: L: %d  M: %d  R: %d  4th: %d  5th: %d",
                    mouse_x, mouse_y, mouse_zv, mouse_zh, mouse_lm, mouse_mm, mouse_rm, mouse_b4, mouse_b5);
                break;
        }
    }
}

static void mouse_wait(uint8_t a_type)
{
    unsigned int time_out = 100000;
    if (a_type==0)
    {
        while (time_out--) // Data
        {
            if ((inportb(0x64) & 1)==1)
            {
                return;
            }
        }
    }
    else
    {
        while (time_out--) // Signal
        {
            if ((inportb(0x64) & 2)==0)
            {
                return;
            }
        }
    }
}

static void mouse_write(int8_t a_write)
{
    // Wait to be able to send a command
    mouse_wait(1);
    // Tell the mouse we are sending a command
    outportb(0x64, 0xD4);
    // Wait for the final part
    mouse_wait(1);
    // Finally write
    outportb(0x60, a_write);
    // If necessary, wait for ACK
    if (a_write != 0xEB && a_write != 0xEC && a_write != 0xF2 && a_write != 0xFF)
    {
        if (mouse_read() != 0xFA)
        {
            // No ACK!!!!
        }
    }
}

static char mouse_read()
{
    // Get response from mouse
    mouse_wait(0);
    return inportb(0x60);
}

void mouse_setsamples(uint8_t samples_per_second)
{
    mouse_write(0xF3);
    switch (samples_per_second)
    {
        case 10:
            mouse_write(0x0A);
            break;
        case 20:
            mouse_write(0x14);
            break;
        case 40:
            mouse_write(0x28);
            break;
        case 60:
            mouse_write(0x3C);
            break;
        case 80:
            mouse_write(0x50);
            break;
        case 100:
            mouse_write(0x64);
            break;
        case 200:
            mouse_write(0xC8);
            break;
        default: // Sorry, mouse just has 10/20/40/60/80/100/200 Samples/sec, so we go back to 80..
            mouse_setsamples(80);
            break;
    }
}

void mouse_uninstall()
{
    irq_uninstallHandler(IRQ_MOUSE);
    mouse_write(0xFF);
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
