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
#include "video/video.h"
#include "irq.h"

char mouseid;
int32_t mouse_x=10; // Mouse X
int32_t mouse_y=10; // Mouse Y
int32_t mouse_z=0;  // Mouse Z (Mousewheel)
char mouse_lm=0;    // Mouse Left Button
char mouse_mm=0;    // Mouse Middle Button
char mouse_rm=0;    // Mouse Right Button
char mouse_b4=0;    // Mouse Button 4
char mouse_b5=0;    // Mouse button 5

char mouse_cycle=0; // MouseHandler help
char mouse_byte[4]; // MouseHandler bytes


void mouse_install()
{
    // Enable the auxiliary mouse device
    mouse_wait(1);
    outportb(0x64, 0xA8);

    // Enable the interrupts
    mouse_wait(1);
    outportb(0x64, 0x20);
    mouse_wait(0);
    uint8_t _status=(inportb(0x60) | 2);
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, _status);

    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read();


    // Check mouse for special features (mousewheel (up/down), button 4+5)
    mouse_initspecialfeatures();


    // Enable the mouse
    mouse_write(0xF4);
    mouse_read();

    // Setup the mouse handler
    irq_installHandler(IRQ_MOUSE, mouse_handler);
}

// Mouse functions
void mouse_handler(registers_t* a_r) // struct regs *a_r (not used but just there)
{
    switch (mouse_cycle)
    {
        case 0:
            mouse_byte[0] = inportb(0x60);
            if (mouse_byte[0] & (1<<3)) // Only if this is really the first Byte!
            {
                mouse_cycle++;
                mouse_lm=(mouse_byte[0] & 0x1);//<< 0);
                mouse_rm=(mouse_byte[0] & 0x2);//<< 1);
                mouse_mm=(mouse_byte[0] & 0x4);//<< 2);
                if (mouse_rm==2)
                    mouse_rm=1;
                if (mouse_mm==4)
                    mouse_mm=1;
            }
            else
            {
                printf("Mouse sent unknown package!\n");
            }
            break;
        case 1:
            mouse_byte[1]=inportb(0x60);
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2]=inportb(0x60);
            if (mouseid == 0x0)
            {
                if (!(mouse_byte[0] & 0x20))
                    mouse_byte[2] |= 0xFFFFFF00; // delta-y is a negative value
                if (!(mouse_byte[0] & 0x10))
                    mouse_byte[1] |= 0xFFFFFF00; // delta-x is a negative value

                mouse_x = mouse_x+mouse_byte[1];
                mouse_y = mouse_y+mouse_byte[2];
                writeInfo(1, "Mouse: X:%d Y:%d Z:No Mousewheel found LM:%d MM:%d RM:%d id:%y\n",
                    mouse_x, mouse_y,
                    mouse_lm, mouse_mm, mouse_rm,
                    mouseid);
                mouse_cycle=0;
            }
            else
            {
                mouse_cycle++;
            }
            break;
        case 3:
            mouse_byte[3]=inportb(0x60);
            if (!(mouse_byte[0] & 0x20))
                mouse_byte[2] |= 0xFFFFFF00; // delta-y is a negative value
            if (!(mouse_byte[0] & 0x10))
                mouse_byte[1] |= 0xFFFFFF00; // delta-x is a negative value
            if (mouseid == 1) // Mouse has 'only' a scrollwheel
            {
                mouse_x = mouse_x+mouse_byte[1];
                mouse_y = mouse_y+mouse_byte[2];
                mouse_z = mouse_z+mouse_byte[3];

                writeInfo(1, "Mouse: X:%d Y:%d Z:%d LM:%d MM:%d RM:%d id:%y\n",
                       mouse_x, mouse_y, mouse_z,
                       mouse_lm, mouse_mm, mouse_rm,
                       mouseid);

                mouse_cycle=0;
            }
            else // Mouse has also Buttons 4+5
            {
                mouse_b4 = mouse_byte[3] & 0x16;
                mouse_b5 = mouse_byte[3] & 0x32;
                mouse_x = mouse_x+mouse_byte[1];
                mouse_y = mouse_y+mouse_byte[2];
                mouse_z = mouse_z+(mouse_byte[3] & 0xF);
                printf("%y: %y\n",mouse_byte[3],(mouse_byte[3] & 0xF));
                /*
                printf("Mouse: X:%d Y:%d Z:%d LM:%d MM:%d RM:%d id:%y\n",
                       mouse_x,mouse_y,mouse_z,
                       mouse_lm,mouse_mm,mouse_rm,
                       mouseid);
                */
                mouse_cycle=0;
            }
            break;
    }
}

inline void mouse_wait(uint8_t a_type)
{
    unsigned int _time_out = 100000;
    if (a_type==0)
    {
        while (_time_out--) // Data
        {
            if ((inportb(0x64) & 1)==1)
            {
                return;
            }
        }
    }
    else
    {
        while (_time_out--) // Signal
        {
            if ((inportb(0x64) & 2)==0)
            {
                return;
            }
        }
    }
}

inline void mouse_write(int8_t a_write)
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
    if (a_write != 0xEB && a_write != 0xEC && a_write != 0xF2 && a_write != 0xFF) {
        if (mouse_read() != 0xFA) {
            // No ACK!!!!
        }
    }
}

char mouse_read()
{
    // Get's response from mouse
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

void mouse_initspecialfeatures()
{
    // Wheel-Mode test
    mouse_setsamples(200);
    mouse_setsamples(100);
    mouse_setsamples(80);
    mouse_write(0xF2);
    mouseid=mouse_read();
    if (mouseid != 0x0)
    {
        mouseid=1;
    }
    else
    {
        return;
    }

    // Sorry, 5-Buttons does not work, so here we
    return;

    // Wheel-and-5-Button-Mode test
    mouse_setsamples(200);
    mouse_setsamples(200);
    mouse_setsamples(80);
    mouse_write(0xF2);
    mouseid=mouse_read();
    if (mouseid != 0x0 && mouseid != 0x01)
    {
        mouseid=2;
    }

    return;
}

void mouse_uninstall()
{
    irq_uninstallHandler(IRQ_MOUSE);
    mouse_write(0xFF);
}


/*
void mouse_handler(struct regs* r)
{
    //printf("Mouse_handler called!\n");
    static unsigned char cycle = 0;
    static char mouse_bytes[3];
    mouse_bytes[cycle++] = inportb(0x60);

    if (cycle == 3) { // if we have all the 3 bytes...
        cycle = 0; // reset the counter
        // do what you wish with the bytes, this is just a sample
        if ((mouse_bytes[0] & 0x80) || (mouse_bytes[0] & 0x40))
            return; // the mouse only sends information about overflowing, do not care about it and return
        if (!(mouse_bytes[0] & 0x20))
            mouse_bytes[2] |= 0xFFFFFF00; //delta-y is a negative value
        if (!(mouse_bytes[0] & 0x10))
            mouse_bytes[1] |= 0xFFFFFF00; //delta-x is a negative value
        if (mouse_bytes[0] & 0x4)
            printf("Middle button is pressed!\n");
        if (mouse_bytes[0] & 0x2)
            printf("Right button is pressed!\n");
        if (mouse_bytes[0] & 0x1)
            printf("Left button is pressed!\n");
        // do what you want here, just replace the puts's to execute an action for each button
        // to use the coordinate data, use mouse_bytes[1] for delta-x, and mouse_bytes[2] for delta-y
        mouse_x=mouse_bytes[1];
        mouse_y=mouse_bytes[2];
        printf("Mouse_x: %d - Mouse_y: %d\n",mouse_x,mouse_y);
  }
}
*/


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
