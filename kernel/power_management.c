#include "power_management.h"
#include "util.h"
#include "video/video.h"
#include "video/console.h"
#include "task.h"

static PM_SYSTEM_t pm_systems[_PM_SYSTEMS_END];


// No power management

static bool nopm_action(PM_STATES state)
{
    switch(state)
    {
        case PM_SOFTOFF: // Implemented by "hack", just as a fallback.
            clear_screen();
            kprintf("                                                                                ", 24, 0x20);
            kprintf("                     Your computer can now be switched off.                     ", 25, 0x20);
            kprintf("                                                                                ", 26, 0x20);
            cli();
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

    // Check for APM
    waitForTask(create_vm86_task(APM_CHECK), 0);
    if(*((uint8_t*)0x1300) != 0) // Error
    {
        printf("\nAPM: Not available");
        return(false);
    }
    printf("\nAPM: Version: %u.%u, Control string: %c%c, Flags: %u", *((uint8_t*)0x1302), *((uint8_t*)0x1301), *((uint8_t*)0x1304), *((uint8_t*)0x1303), *((uint16_t*)0x1305));

    // Activate APM
    waitForTask(create_vm86_task(APM_INSTALL), 0);
    switch(*((uint8_t*)0x1300)) {
        case 0:
            printf("\nAPM: Successfully activated");
            return(true);
            break;
        case 1:
            printf("\nAPM: Error while disconnecting, %y", *((uint8_t*)0x1301));
            return(false);
            break;
        case 2:
            printf("\nAPM: Error while connecting, %y", *((uint8_t*)0x1301));
            return(false);
            break;
        case 3:
            printf("\nAPM: Error while handling out APM version, %y", *((uint8_t*)0x1301));
            return(false);
            break;
        case 4:
            printf("\nAPM: Error while activating, %y", *((uint8_t*)0x1301));
            return(false);
            break;
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
            return(false);
        default: // Every other state is unreachable with APM
            return(false);
    }
}


// Interface

void pm_install()
{
    pm_systems[PM_NO].supported = true; // Always available
    pm_systems[PM_NO].action = &nopm_action;
    pm_systems[PM_APM].supported = false;//apm_install();
    pm_systems[PM_APM].action = &apm_action;
    pm_systems[PM_ACPI].supported = false; // Unsupported by PrettyOS
    pm_systems[PM_ACPI].action = 0;
}

void pm_log()
{
    printf("\nAPM: %s \tACPI: %s\n", pm_systems[PM_APM].supported?"available":"not supported", "Not supported by PrettyOS");
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
