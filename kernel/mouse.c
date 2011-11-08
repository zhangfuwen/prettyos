/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

// http://houbysoft.com/download/ps2mouse.html
// http://forum.osdev.org/viewtopic.php?t=10247
// http://lowlevel.brainsware.org/wiki/index.php/Programmierung_der_PS/2-Maus

#include "mouse.h"
#include "util/util.h"
#include "irq.h"
#include "video/console.h"
#include "video/vbe.h"
#include "video/video.h"
#include "tasking/task.h"
#include "events.h"


enum {NORMAL, WHEEL, WHEELS5BUTTON} mousetype = NORMAL;

int32_t mouse_x = 10;
int32_t mouse_y = 10;
int32_t mouse_zv = 0; // vertical mousewheel
int32_t mouse_zh = 0; // horizontal mousewheel
mouse_button_t mouse_buttons = 0; // Status of mouse buttons


static void mouse_wait(uint8_t type);
static void mouse_write(int8_t data);
static char mouse_read();
static void mouse_handler(registers_t* a_r);


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

    // Setup the mouse handler
    irq_installHandler(IRQ_MOUSE, mouse_handler);

    // Enable the mouse
    mouse_write(0xF4);
    mouse_read();
}

static void mouse_buttonEvent(bool pressed, mouse_button_t button)
{
    if(((mouse_buttons&button) != 0) != pressed)
    {
        if(pressed)
        {
            for (dlelement_t* e = console_displayed->tasks->head; e != 0; e = e->next)
                event_issue(((task_t*)(e->data))->eventQueue, EVENT_MOUSE_BUTTON_DOWN, &button, sizeof(button));
            mouse_buttons = mouse_buttons | button;
        }
        else
        {
            for (dlelement_t* e = console_displayed->tasks->head; e != 0; e = e->next)
                event_issue(((task_t*)(e->data))->eventQueue, EVENT_MOUSE_BUTTON_UP, &button, sizeof(button));
            mouse_buttons = mouse_buttons & ~button;
        }

    }
}

static void mouse_wheelEvent(int8_t v, int8_t h)
{
    if(v != 0 || h != 0)
    {
        mouse_zh += h;
        mouse_zv += v;

        int16_t composedMovement = (((int16_t)h)<<8) | v;
        for (dlelement_t* e = console_displayed->tasks->head; e != 0; e = e->next)
            event_issue(((task_t*)(e->data))->eventQueue, EVENT_MOUSE_MOVE, &composedMovement, sizeof(composedMovement));
    }
}

static void mouse_moveEvent(int8_t x, int8_t y)
{
    if(y != 0 || x != 0)
    {
        mouse_y += y;
        mouse_x += x;

        int16_t composedMovement = (((int16_t)x)<<8) | y;
        for (dlelement_t* e = console_displayed->tasks->head; e != 0; e = e->next)
            event_issue(((task_t*)(e->data))->eventQueue, EVENT_MOUSE_MOVE, &composedMovement, sizeof(composedMovement));

        if (videomode == VM_VBE)
        {
            mouse_y = max(0, min(mouse_y, video_currentMode->yRes-1)); // clamp mouse position to height of screen
            mouse_x = max(0, min(mouse_x, video_currentMode->xRes-1)); // same with width
        }
    }
}

static void mouse_handler(registers_t* r)
{
    static bool erroroccurred = false;
    static uint8_t bytecounter = 0;
    static int8_t bytes[4];

    bytes[bytecounter] = inportb(0x60); // Receive byte
    switch (bytecounter)
    {
        case 0: // First byte: Left Button | Right Button | Middle Button | 1 | X sign | Y sign | X overflow | Y overflow
            if (bytes[0] & BIT(3)) // Only if this is really the first byte!
            {
                mouse_buttonEvent(bytes[0] & BIT(0), BUTTON_LEFT);
                mouse_buttonEvent(bytes[0] & BIT(1), BUTTON_RIGHT);
                mouse_buttonEvent(bytes[0] & BIT(2), BUTTON_MIDDLE);
            }
            else
            {
                bytecounter = 0;
                if (erroroccurred == false) // Ignore error on the first time due to some emulators, TODO: Why?
                    erroroccurred = true;
                else
                {
                    textColor(ERROR);
                    printf(" => ERROR (mouse.c, 159): Mouse sent unknown package (%yh)!\n", bytes[0]);
                    textColor(TEXT);
                }
                return;
            }
            break;
        case 1: // Second byte: X Movement. Evaluation delayed until third byte
            break;
        case 2: // Third byte: Y Movement
            mouse_moveEvent(bytes[1], -bytes[2]);
            break;
        case 3: // Fourth byte: Z movement (4 bits) | 4th Button | 5th Button | 0 | 0
            switch (mousetype)
            {
                case WHEEL:
                    mouse_wheelEvent(bytes[3], 0);
                    break;
                case WHEELS5BUTTON:
                    switch (bytes[3] & 0xF)
                    {
                        case 0xE:
                            mouse_wheelEvent(0, -1);
                            break;
                        case 0xF:
                            mouse_wheelEvent(-1, 0);
                            break;
                        case 0x1:
                            mouse_wheelEvent(1, 0);
                            break;
                        case 0x2:
                            mouse_wheelEvent(0, 1);
                            break;
                    }

                    mouse_buttonEvent(bytes[3] & BIT(4), BUTTON_4);
                    mouse_buttonEvent(bytes[3] & BIT(5), BUTTON_5);
                    break;
                default: // We do not expect a fourth byte in this case
                    bytecounter--;

                    textColor(ERROR);
                    printf(" ERROR (mouse.c, 201): Mouse sent unknown package (%u)!\n", bytes[0]);
                    textColor(TEXT);
                    break;
            }
            break;
    }


    bytecounter++;
    switch (mousetype) // reset packetcounter when received all expected packets
    {
        case WHEEL: case WHEELS5BUTTON:
            if (bytecounter > 3)
                bytecounter = 0;
            break;
        case NORMAL: default:
            if (bytecounter > 2)
                bytecounter = 0;
            break;
    }


    // Print mouse on screen
    if (videomode != VM_VBE) // In VBE mode the application draws the mouse to work in double buffer modes as well.
    {
        switch (mousetype)
        {
            case NORMAL:
                writeInfo(1, "Mouse: X: %d  Y: %d  Z: -   buttons: L: %d  M: %d  R: %d",
                    mouse_x, mouse_y, (mouse_buttons&BUTTON_LEFT) != 0, (mouse_buttons&BUTTON_MIDDLE) != 0, (mouse_buttons&BUTTON_RIGHT) != 0);
                break;
            case WHEEL:
                writeInfo(1, "Mouse: X: %d  Y: %d  Z: %d   buttons: L: %d  M: %d  R: %d",
                    mouse_x, mouse_y, mouse_zv, (mouse_buttons&BUTTON_LEFT) != 0, (mouse_buttons&BUTTON_MIDDLE) != 0, (mouse_buttons&BUTTON_RIGHT) != 0);
                break;
            case WHEELS5BUTTON:
                writeInfo(1, "Mouse: X: %d  Y: %d  Zv: %d  Zh: %d   buttons: L: %d  M: %d  R: %d  4th: %d  5th: %d",
                    mouse_x, mouse_y, mouse_zv, mouse_zh,
                    (mouse_buttons&BUTTON_LEFT) != 0, (mouse_buttons&BUTTON_MIDDLE) != 0, (mouse_buttons&BUTTON_RIGHT) != 0, (mouse_buttons&BUTTON_4) != 0, (mouse_buttons&BUTTON_5) != 0);
                break;
        }
    }
}

static void mouse_wait(uint8_t type) // Data: 0, Signal: 1
{
    unsigned int time_out = 100000;

    while (time_out--) // Data
    {
        if ((inportb(0x64) & (type+1))==(type==0?1:0))
        {
            return;
        }
    }
}

static void mouse_write(int8_t data)
{
    // Wait to be able to send a command
    mouse_wait(1);
    // Tell the mouse we are sending a command
    outportb(0x64, 0xD4);
    // Wait for the final part
    mouse_wait(1);
    // Finally write
    outportb(0x60, data);
    // If necessary, wait for ACK
    if (data != 0xEB && data != 0xEC && data != 0xF2 && data != 0xFF)
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
        case 10: case 20: case 40: case 60: case 80: case 100: case 200:
            mouse_write(samples_per_second);
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
