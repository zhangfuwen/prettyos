/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define _PCI_VEND_PROD_LIST_ // http://www.pcidatabase.com/pci_c_header.php - Increases the size of devmgr significantly
#ifdef _PCI_VEND_PROD_LIST_
#include "pciVendProdList.h"
#endif

void error(const char* string) {
    textColor(ERROR);
    puts(string);
    textColor(TEXT);
}

void showHelp(void) {
    printf("Command line arguments:\n"
           "    --help, -?       Prints available command line arguments\n"
           "    --showpci, -sp   Shows all PCI devices present\n"
           "\n");
}

void showPciDevices(void) {
    textColor(TABLE_HEADING);
    printf("\nB:D:F\tIRQ\tDescription");
    printf("\n--------------------------------------------------------------------------------");
    textColor(TEXT);

    int64_t count;
    ipc_getInt("PrettyOS/PCI/Number of Devices", &count);
    for(int i = 0; i < count; i++) {
        char path[100] = "PrettyOS/PCI/";
        itoa(i, path+13);
        int64_t bus, dev, func, irq, vendId, devId;
        char* end = path+strlen(path);
        strcpy(end, "/Bus");
        ipc_getInt(path, &bus);
        strcpy(end, "/Device");
        ipc_getInt(path, &dev);
        strcpy(end, "/Function");
        ipc_getInt(path, &func);
        strcpy(end, "/IRQ");
        ipc_getInt(path, &irq);
        strcpy(end, "/VendorID");
        ipc_getInt(path, &vendId);
        strcpy(end, "/DeviceID");
        ipc_getInt(path, &devId);

        // Screen output
        if (irq != 255)
        {
            printf("%d:%d.%d\t%d", (uint32_t)bus, (uint32_t)dev, (uint32_t)func, (uint32_t)irq);

            #ifdef _PCI_VEND_PROD_LIST_
            // Find Vendor
            bool found = false;
            for (uint32_t j = 0; j < PCI_VENTABLE_LEN; j++)
            {
                if (PciVenTable[j].VenId == vendId)
                {
                    printf("\t%s", PciVenTable[j].VenShort); // Found! Display name and break out
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                printf("\tvend: %xh", (uint32_t)vendId); // Vendor not found, display ID
            }
            else
            {
                // Find Device
                found = false;
                for (uint32_t j = 0; j < PCI_DEVTABLE_LEN; j++)
                {
                    if (PciDevTable[j].DevId == devId && PciDevTable[j].VenId == vendId) // VendorID and DeviceID have to fit
                    {
                        printf(", %s", PciDevTable[j].ChipDesc); // Found! Display name and break out
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                printf(", dev: %xh", (uint32_t)devId); // Device not found, display ID
            }
            #else
            printf("\tvend: %xh, dev: %xh", (uint32_t)vendId, (uint32_t)devId);
            #endif

            putchar('\n');
        }
    }
    textColor(TABLE_HEADING);
    printf("--------------------------------------------------------------------------------\n");
    textColor(TEXT);
}

void parseArg(const char* arg) {
    if(strcmp(arg, "--help") == 0 || strcmp(arg, "-?") == 0)
        showHelp();
    else if(strcmp(arg, "--showpci") == 0 || strcmp(arg, "-sp") == 0)
        showPciDevices();
    else {
        error("Unknown command.\n\n");
        showHelp();
    }
}

int main(int argc, char* argv[])
{
    printLine("================================================================================", 0, 0x0B);
    printLine("                                 Device Manager",                                  2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 6);

    if(argc <= 1) {
        showHelp();
        printf("\nCommand ('exit' to quit): ");
        char cmd[20];
        gets(cmd);
        if(strcmp(cmd, "exit") == 0)
            return 0;
        parseArg(cmd);
        getchar();
        return 0;
    }

    for(size_t i = 1; i < argc; i++) {
        parseArg(argv[i]);
    }

    getchar();
    return (0);
}

/*
* Copyright (c) 2012-2013 The PrettyOS Project. All rights reserved.
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
