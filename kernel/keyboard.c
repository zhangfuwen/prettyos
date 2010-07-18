/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "keyboard.h"
#include "util.h"
#include "task.h"
#include "console.h"
#include "event_list.h"
#include "video.h"
#include "irq.h"
#include "scheduler.h"
#include "devicemanager.h" // HACK

#ifdef KEYMAP_GER
#include "keyboard_GER.h"
#else //US-Keyboard if nothing else is defined
#include "keyboard_US.h"
#endif

bool ShiftKeyDown = false;  // variable for Shift Key Down
bool CtrlKeyDown  = false;  // variable for Ctrl Key Down
bool AltKeyDown   = false;  // variable for Alt Key Down
bool AltGrKeyDown = false;  // variable for AltGr Key Down
bool KeyPressed   = false;  // variable for Key Pressed
uint8_t curScan   = 0;      // current scan code from Keyboard
uint8_t prevScan  = 0;      // previous scan code

bool VKPressed[170]; // for monitoring pressed keys

void keyboard_install()
{
    irq_install_handler(32+1, keyboard_handler); // Installs 'keyboard_handler' to IRQ1

    while (inportb(0x64) & 1) // wait until buffer is empty
    {
        inportb(0x60);
    }
}

uint8_t FetchAndAnalyzeScancode()
{
    if (inportb(0x64)&1)
        curScan = inportb(0x60);   // 0x60: get scan code from the keyboard

    // ACK: toggle bit 7 at port 0x61
    uint8_t port_value = inportb(0x61);
    outportb(0x61,port_value |  0x80); // 0->1
    outportb(0x61,port_value &~ 0x80); // 1->0

    if (curScan & 0x80) // Key released? Check bit 7 (10000000b = 0x80) of scan code for this
    {
        curScan &= 0x7F; // Key was released, compare only low seven bits: 01111111b = 0x7F
        VKPressed[(uint8_t)toUpper(asciiNonShift[curScan])] = KeyPressed = false;

        if (curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT) // A key was released, shift key up?
        {
            VKPressed[VK_SHIFT] = ShiftKeyDown = KeyPressed; // yes, it is up --> NonShift
            if (curScan == KRLEFT_SHIFT)
            {
                 VKPressed[VK_LSHIFT] = KeyPressed;
            }
            else
            {
                 VKPressed[VK_RSHIFT] = KeyPressed;
            }
        }
        if ((curScan == 0x38) && (prevScan == 0x60))
        {
            AltGrKeyDown = false;
        }
        else if (curScan == 0x38)
        {
            AltKeyDown = false;
        }
        if (curScan == 0x1D)
        {
            CtrlKeyDown = false;
        }
    }
    else // Key was pressed
    {
        VKPressed[(uint8_t)toUpper(asciiNonShift[curScan])] = KeyPressed = true;

        if (curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT)
        {
            VKPressed[VK_SHIFT] = ShiftKeyDown = KeyPressed; // It is down, use asciiShift characters
            if (curScan == KRLEFT_SHIFT)
            {
                 VKPressed[VK_LSHIFT] = KeyPressed;
            }
            else
            {
                 VKPressed[VK_RSHIFT] = KeyPressed;
            }
        }
        if ((curScan == 0x38) && (prevScan == 0x60))
        {
            AltGrKeyDown = true;
        }
        else if (curScan == 0x38)
        {
            AltKeyDown = true;
        }
        if (curScan == 0x1D)
        {
            CtrlKeyDown = true;
        }
    }
    prevScan = curScan;
    return curScan;
}

uint8_t ScanToASCII()
{
    curScan = FetchAndAnalyzeScancode();  // Grab scancode, and get the position of the shift key

    // filter Shift Key and Key Release
    if (((curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT)) || (KeyPressed == false))
    {
        return 0;
    }

    uint8_t retchar = 0;  // The character that returns the scan code to ASCII code

    if (AltGrKeyDown)
    {
        if (ShiftKeyDown)
        {
            retchar = asciiShiftAltGr[curScan];
        }
        if (!ShiftKeyDown || retchar == 0) // if just shift is pressed or if there is no key specified for ShiftAltGr
        {
            retchar = asciiAltGr[curScan];
        }
    }
    if (!AltGrKeyDown || retchar == 0)
    {
        if (ShiftKeyDown)
        {
            retchar = asciiShift[curScan];
        }
        if (!ShiftKeyDown || retchar == 0)
        {
            retchar = asciiNonShift[curScan]; // (Lower) Non-Shift Codes
        }
    }

    // filter Special Keys
    if (AltKeyDown) // Console-Switching
    {
        if (retchar == 'm')
        {
            changeDisplayedConsole(10);
            return(0);
        }
        if (ctoi(retchar) != -1)
        {
            changeDisplayedConsole(ctoi(retchar));
            return 0;
        }
    }

    if(CtrlKeyDown)
    {
        if(retchar == 's') // Taking a screenshot (FLOPPY); Should be changed to the Print-Screen-Key (not available because of bugs in keyboard-headers)
        {
            ScreenDest = &FLOPPYDISK; // HACK
            addEvent(&VIDEO_SCREENSHOT);
            return 0;
        }
        if(retchar == 'u') // Taking a screenshot (USB); Should be changed to the Print-Screen-Key (not available because of bugs in keyboard-headers)
        {
            ScreenDest = &USB_MSD; // HACK
            addEvent(&VIDEO_SCREENSHOT);
            return 0;
        }
        if(retchar == 'd') // Prints the Port- and Disklist
        {
            showPortList();
            showDiskList();
            return 0;
        }
        if(retchar == 't') // If you want to test something
        {
            scheduler_log();
            return 0;
        }
    }

    return retchar; // ASCII version
}

void keyboard_handler(registers_t* r)
{
   uint8_t KEY = ScanToASCII();
   if (KEY)
   {
       *reachableConsoles[displayedConsole]->KQ.pTail = KEY;
       ++reachableConsoles[displayedConsole]->KQ.count_write;

       if (reachableConsoles[displayedConsole]->KQ.pTail > reachableConsoles[displayedConsole]->KQ.buffer)
       {
           --reachableConsoles[displayedConsole]->KQ.pTail;
       }
       if (reachableConsoles[displayedConsole]->KQ.pTail == reachableConsoles[displayedConsole]->KQ.buffer)
       {
           reachableConsoles[displayedConsole]->KQ.pTail = reachableConsoles[displayedConsole]->KQ.buffer + KQSIZE - 1;
       }
   }
}

uint8_t keyboard_getChar() // get a character <--- TODO: make it POSIX like
{
   /// TODO: should only return character, if keystroke was entered

   if (current_console->KQ.count_write > current_console->KQ.count_read)
   {
       cli();
       uint8_t KEY = *current_console->KQ.pHead;
       ++current_console->KQ.count_read;

       if (current_console->KQ.pHead > current_console->KQ.buffer)
       {
           --current_console->KQ.pHead;
       }
       if (current_console->KQ.pHead == current_console->KQ.buffer)
       {
           current_console->KQ.pHead = current_console->KQ.buffer + KQSIZE - 1;
       }
       sti();
       return KEY;
   }
   return 0;
}

char getch()
{
    char retVal = keyboard_getChar();
    while(retVal == 0)
    {
        sti();
        __asm__ volatile ("hlt");
        retVal = keyboard_getChar();
    }
    return(retVal);
}

bool keyPressed(VK Key)
{
    return(VKPressed[Key]);
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
