/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "keyboard.h"
#include "util.h"
#include "task.h"
#include "irq.h"
#include "network/network.h"
#include "netprotocol/udp.h"
#include "netprotocol/dhcp.h"

#if KEYMAP == GER
#include "keyboard_GER.h"
#else //US-Keyboard if nothing else is defined
#include "keyboard_US.h"
#endif

static bool AltKeyDown   = false; // variable for Alt Key Down
static bool AltGrKeyDown = false; // variable for AltGr Key Down
static bool KeyPressed   = false; // variable for Key Pressed
static uint8_t curScan   = 0;     // current scan code from Keyboard
static uint8_t prevScan  = 0;     // previous scan code

static bool VKPressed[170]; // for monitoring pressed keys


void keyboard_install()
{
    irq_installHandler(IRQ_KEYBOARD, keyboard_handler); // Installs 'keyboard_handler' to IRQ_KEYBOARD

    while (inportb(0x64) & 1) // wait until buffer is empty
    {
        inportb(0x60);
    }
}

void keyboard_initKQ(keyqueue_t* KQ)
{
    memset(KQ->buffer, 0, KQSIZE);
    KQ->pHead = KQ->buffer;
    KQ->pTail = KQ->buffer;
    KQ->count = 0;
    KQ->mutex = mutex_create();
}

static uint8_t getScancode()
{
    uint8_t scancode = 0;
    if (inportb(0x64)&1)
        scancode = inportb(0x60);   // 0x60: get scan code from the keyboard
    return(scancode);
}

static void setKeyState(uint8_t scanCode, bool pressed)
{
    KeyPressed = pressed;

    VKPressed[(uint8_t)toUpper(asciiNonShift[scanCode])] = pressed;
    if (scanCode == KRLEFT_ALT || scanCode == KRRIGHT_ALT) // ALT: We do not differenciate between left and right
    {
        if(prevScan == 0x60)
            AltGrKeyDown = KeyPressed;
        else
            AltKeyDown = KeyPressed;
    }
    else if (scanCode == KRLEFT_SHIFT) // SHIFT
    {
        VKPressed[VK_SHIFT] = KeyPressed;
        VKPressed[VK_LSHIFT] = KeyPressed;
    }
    else if (scanCode == KRRIGHT_SHIFT)
    {
        VKPressed[VK_SHIFT] = KeyPressed;
        VKPressed[VK_RSHIFT] = KeyPressed;
    }
    else if (scanCode == KRLEFT_CTRL) // CONTROL
    {
        VKPressed[VK_LCONTROL] = KeyPressed;
        VKPressed[VK_CONTROL] = KeyPressed;
    }
    else if (scanCode == KRRIGHT_CTRL)
    {
        VKPressed[VK_RCONTROL] = KeyPressed;
        VKPressed[VK_CONTROL] = KeyPressed;
    }
}

static uint8_t FetchAndAnalyzeScancode()
{
    curScan = getScancode();

    if (curScan & 0x80) // Key released? Check bit 7 (10000000b = 0x80) of scan code for this
    {
        curScan &= 0x7F; // Key was released, compare only low seven bits: 01111111b = 0x7F
        setKeyState(curScan, false);
    }
    else // Key was pressed
    {
        setKeyState(curScan, true);
    }
    prevScan = curScan;
    return curScan;
}

uint8_t ScanToASCII()
{
    curScan = FetchAndAnalyzeScancode(); // Grab scancode, and get the position of the shift key

    // filter Shift Key and Key Release
    if (curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT || KeyPressed == false)
    {
        return 0;
    }

    uint8_t retchar = 0; // The character that returns the scan code to ASCII code

    if (AltGrKeyDown)
    {
        if (VKPressed[VK_SHIFT])
        {
            retchar = asciiShiftAltGr[curScan];
        }
        if (!VKPressed[VK_SHIFT] || retchar == 0) // if just shift is pressed or if there is no key specified for ShiftAltGr (so retchar is still 0)
        {
            retchar = asciiAltGr[curScan];
        }
    }
    if (!AltGrKeyDown || retchar == 0)
    {
        if (VKPressed[VK_SHIFT])
        {
            retchar = asciiShift[curScan];
        }
        if (!VKPressed[VK_SHIFT] || retchar == 0)
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

    if(VKPressed[VK_CONTROL]) // TODO: Probably everything accessed by Ctrl+Key can be moved into the shell when the needed syscalls and/or PrettyIPC are implemented
    {
        if(retchar == 's') // Taking a screenshot (FLOPPY); Should be changed to the Print-Screen-Key (not available because of bugs in keyboard-headers)
        {
            ScreenDest = &FLOPPYDISK; // HACK
            printf("Screenshot to Floppy (Thread)\n");
            create_thread(&screenshot);
            return 0;
        }
        if(retchar == 'u') // Taking a screenshot (USB); Should be changed to the Print-Screen-Key (not available because of bugs in keyboard-headers)
        {
            ScreenDest = &USB_MSD; // HACK
            printf("Screenshot to USB (Thread)\n");
            create_thread(&screenshot);
            return 0;
        }
        if(retchar == 'd') // Prints the Port- and Disklist
        {
            showPortList();
            showDiskList();
            return 0;
        }
        if(retchar == 'a') // Display ARP tables of all network adapters
        {
            network_displayArpTables();
            return 0;
        }
        if(retchar == 't') // If you want to test something
        {
            scheduler_log();
            return 0;
        }
        if(retchar == 'n') // If you want to test something in networking
        {
            uint8_t sourceIP_address[4] ={0,0,0,0};
            uint8_t   destIP_address[4] ={255,255,255,255};

            network_adapter_t* adapter = network_getAdapter(sourceIP_address);
            printf("network adapter: %X\n", adapter); // check

            uint16_t srcPort  = 40; // unassigend
            uint16_t destPort = 40;

            if (adapter)
            {
                UDPSend(adapter, "PrettyOS says hello", strlen("PrettyOS says hello"), srcPort, adapter->IP_address, destPort, destIP_address);
                DHCP_Discover(adapter);
            }
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
        mutex_lock(reachableConsoles[displayedConsole]->KQ.mutex);
        *reachableConsoles[displayedConsole]->KQ.pTail = KEY;
        ++reachableConsoles[displayedConsole]->KQ.count;

        if (reachableConsoles[displayedConsole]->KQ.pTail > reachableConsoles[displayedConsole]->KQ.buffer)
        {
            --reachableConsoles[displayedConsole]->KQ.pTail;
        }
        if (reachableConsoles[displayedConsole]->KQ.pTail == reachableConsoles[displayedConsole]->KQ.buffer)
        {
            reachableConsoles[displayedConsole]->KQ.pTail = reachableConsoles[displayedConsole]->KQ.buffer + KQSIZE - 1;
        }
        mutex_unlock(reachableConsoles[displayedConsole]->KQ.mutex);
    }
}

uint8_t keyboard_getChar()
{
    if (currentConsole->KQ.count > 0)
    {
        mutex_lock(currentConsole->KQ.mutex);
        uint8_t KEY = *currentConsole->KQ.pHead;
        --currentConsole->KQ.count;

        if (currentConsole->KQ.pHead > currentConsole->KQ.buffer)
        {
            --currentConsole->KQ.pHead;
        }
        if (currentConsole->KQ.pHead == currentConsole->KQ.buffer)
        {
            currentConsole->KQ.pHead = (void*)currentConsole->KQ.buffer + KQSIZE - 1;
        }
        mutex_unlock(currentConsole->KQ.mutex);
        return KEY;
    }
    return 0;
}

char getch()
{
    char retVal = keyboard_getChar();
    while(retVal == 0)
    {
        waitForIRQ(IRQ_KEYBOARD, 0);
        irq_resetCounter(IRQ_KEYBOARD);
        retVal = keyboard_getChar();
    }
    return(retVal);
}

bool keyPressed(VK Key)
{
    return(VKPressed[Key]);
}

/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
