/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "power_management.h"
#include "util.h"
#include "video/video.h"
#include "video/console.h"
#include "task.h"


// No power management

static bool nopm_action(PM_STATES state)
{
    switch(state)
    {
        case PM_SOFTOFF: // Implemented by "hack", just as a fallback.
            cli();
            clear_screen();
            kprintf("                                                                                ", 24, 0x20);
            kprintf("                     Your computer can now be switched off.                     ", 25, 0x20);
            kprintf("                                                                                ", 26, 0x20);
            hlt();
            return(false); // Hopefully not reached
        case PM_REBOOT: // We do not use the powermanagement here because its not necessary
        {
            int32_t temp; // A temporary int for storing keyboard info. The keyboard is used to reboot
            do // Flush the keyboard controller
            {
               temp = inportb(0x64);
               if (temp & 1)
                 inportb(0x60);
            }
            while (temp & 2);

            // Reboot
            outportb(0x64, 0xFE);

            return(false); // Hopefully not reached
        }
        default: // Every other state is unreachable without PM
            return(false);
    }
}



// APM (http://www.lowlevel.eu/wiki/APM)

extern uintptr_t apm_com_start;
extern uintptr_t apm_com_end;
// This values are hardcoded adresses from documentation/apm.map
#define APM_CHECK    ((void*)0x200)
#define APM_INSTALL  ((void*)0x22A)
#define APM_SETSTATE ((void*)0x282)

bool apm_install()
{
    memcpy((void*)0x200, &apm_com_start, (uintptr_t)&apm_com_end - (uintptr_t)&apm_com_start);

    textColor(0x03);
    printf("\nAPM: ");
    textColor(WHITE);
    // Check for APM
    waitForTask(create_vm86_task(APM_CHECK), 0);
    if(*((uint8_t*)0x1300) != 0) // Error
    {
        printf("\nNot available.");
        return(false);
    }
    printf("\nVersion: %u.%u, Control string: %c%c, Flags: %u.", *((uint8_t*)0x1302), *((uint8_t*)0x1301), *((uint8_t*)0x1304), *((uint8_t*)0x1303), *((uint16_t*)0x1305));

    // Activate APM
    waitForTask(create_vm86_task(APM_INSTALL), 0);
    switch(*((uint8_t*)0x1300)) {
        case 0:
            printf("\nSuccessfully activated.");
            return(true);
        case 1:
            printf("\nError while disconnecting: %yh.", *((uint8_t*)0x1301));
            return(false);
        case 2:
            printf("\nError while connecting: %yh.", *((uint8_t*)0x1301));
            return(false);
        case 3:
            printf("\nError while handling out APM version: %yh.", *((uint8_t*)0x1301));
            return(false);
        case 4:
            printf("\nError while activating: %yh.", *((uint8_t*)0x1301));
            return(false);
    }
    return(false);
}

static bool apm_action(PM_STATES state)
{
    switch(state)
    {
        case PM_STANDBY:
            *((uint16_t*)0x1300) = 2; // Suspend-Mode (turns more hardware off than standby)
            waitForTask(create_vm86_task(APM_SETSTATE), 0);
            return(*((uint16_t*)0x1300) != 0);
        case PM_SOFTOFF:
            *((uint16_t*)0x1300) = 3;
            waitForTask(create_vm86_task(APM_SETSTATE), 0);
            return(*((uint16_t*)0x1300) != 0);
        default: // Every other state is unreachable with APM
            return(false);
    }
}


// Interface

static PM_SYSTEM_t pm_systems[_PM_SYSTEMS_END] = {
    {.action = &nopm_action}, // No PM
    {.action = &apm_action},  // APM
    {.action = 0}             // ACPI
};

void pm_install()
{
    pm_systems[PM_NO].supported = true; // Always available
    pm_systems[PM_APM].supported = false;//apm_install();
    pm_systems[PM_ACPI].supported = false; // Unsupported by PrettyOS
}

void pm_log()
{
    textColor(0x03);
    printf("\nAPM: ");
    textColor(WHITE);
    printf("%s.", pm_systems[PM_APM].supported?"Available":"Not supported");
    textColor(0x03);
    printf("\tACPI: ");
    textColor(WHITE);
    printf("%s.\n", "Not supported by PrettyOS");
}

bool pm_action(PM_STATES state)
{
    if(state < _PM_STATES_END)
    {
        bool success = false;
        for(int32_t i = _PM_SYSTEMS_END-1; i >= 0 && !success; i--) // Trying out all supported power management systems.
        {
            if(pm_systems[i].supported && pm_systems[i].action != 0)
                success = pm_systems[i].action(state);
        }
        return(success);
    }
    return(false); // Invalid state
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
