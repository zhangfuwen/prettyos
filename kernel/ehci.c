/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"
#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "sys_speaker.h"

extern page_directory_t* kernel_pd;
struct ehci_CapRegs* pCapRegs; // = &CapRegs;
struct ehci_OpRegs*  pOpRegs;  // = &OpRegs;
uint32_t ubar;
uint32_t eecp;

void ehci_handler(struct regs* r)
{
	settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )
    {
        printformat("\nUSB Interrupt");
        pOpRegs->USBSTS |= STS_USBINT;
    }
    if( pOpRegs->USBSTS & STS_USBERRINT )
    {
        printformat("\nUSB Error Interrupt");
        pOpRegs->USBSTS |= STS_USBERRINT;
    }
    if( pOpRegs->USBSTS & STS_PORT_CHANGE )
    {
        pOpRegs->USBSTS |= STS_PORT_CHANGE;
        showPORTSC();
    }
    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER )
    {
        /*printformat("\nFrame List Rollover Interrupt");*/
        pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;
    }
    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )
    {
        printformat("\nHost System Error Interrupt");
        pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;
    }
    if( pOpRegs->USBSTS & STS_ASYNC_INT )
    {
        printformat("\nInterrupt on Async Advance");
        pOpRegs->USBSTS |= STS_ASYNC_INT;
    }
	settextcolor(15,0);
}

void analyzeEHCI(uint32_t bar)
{
    ubar = bar;
    EHCIflag = true;
    pCapRegs = (struct ehci_CapRegs*) ubar;
	pOpRegs  = (struct ehci_OpRegs*) (ubar + pCapRegs->CAPLENGTH);
    numPorts = (pCapRegs->HCSPARAMS & 0x000F);

    printformat("HCIVERSION: %x ", pCapRegs->HCIVERSION);                // Interface Version Number
    printformat("HCSPARAMS: %X ", pCapRegs->HCSPARAMS);                  // Structural Parameters
    printformat("Ports: %d ", numPorts);
    printformat("\nHCCPARAMS: %X ", pCapRegs->HCCPARAMS);                // Capability Parameters
    if(BYTE2(pCapRegs->HCCPARAMS)==0) printformat("No ext. capabil. ");  // Extended Capabilities Pointer
    printformat("\nOpRegs Address: %X ", pOpRegs);                       // Host Controller Operational Registers
}

void initEHCIHostController(uint32_t number)
{
    irq_install_handler(32 + pciDev_Array[number].irq, ehci_handler);

    /// TEST
    DeactivateLegacySupport(number);
    /// TEST

    pOpRegs->USBCMD &= ~CMD_RUN_STOP; // set Run-Stop-Bit to 0

    sleepMilliSeconds(30); // wait at least 16 microframes ( = 16*125 µs = 2 ms )

    pOpRegs->USBCMD |= CMD_HCRESET;  // set Reset-Bit to 1
    int32_t timeout=0;
    while( (pOpRegs->USBCMD & 0x2) != 0 )
    {
        printformat("waiting for HC reset\n");
        sleepMilliSeconds(20);
        timeout++;
        if(timeout>10)
        {
            break;
        }
    }

    pOpRegs->USBINTR = 0; // EHCI interrupts disabled

    // Program the CTRLDSSEGMENT register with 4-Gigabyte segment where all of the interface data structures are allocated.
    pOpRegs->CTRLDSSEGMENT = 0;

    // Write the base address of the Periodic Frame List to the PERIODICLIST BASE register.
    void* virtualMemoryPERIODICLISTBASE   = malloc(0x1000,PAGESIZE);
    void* physicalMemoryPERIODICLISTBASE  = (void*) paging_get_phys_addr(kernel_pd, virtualMemoryPERIODICLISTBASE);
    pOpRegs->PERIODICLISTBASE = (uint32_t)physicalMemoryPERIODICLISTBASE ;

    // If there are no work items in the periodic schedule, all elements should have their T-Bits set to 1.
    memsetl(virtualMemoryPERIODICLISTBASE, 1, 1024);

    // Turn the host controller ON via setting the Run/Stop bit
    // Software must not write a one to this field unless the host controller is in the Halted state
    pOpRegs->USBCMD |= CMD_8_MICROFRAME;
    if( pOpRegs->USBSTS & STS_HCHALTED )
    {
        pOpRegs->USBCMD |= CMD_RUN_STOP; // set Run-Stop-Bit
    }

    // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller
    pOpRegs->CONFIGFLAG = CF;

    // Enable ports
    for(uint8_t j=0; j<numPorts; j++)
    {
         pOpRegs->PORTSC[j] |=  PSTS_POWERON;
         pOpRegs->PORTSC[j] &= ~PSTS_ENABLED;
         pOpRegs->PORTSC[j] |=  PSTS_PORT_RESET;
         sleepMilliSeconds(50);
         pOpRegs->PORTSC[j] &= ~PSTS_PORT_RESET;
    }

    // Write the appropriate value to the USBINTR register to enable the appropriate interrupts.
    pOpRegs->USBINTR = STS_INTMASK; // all interrupts allowed  // ---> VMWare works!

    printformat("\n\nAfter Init of EHCI:");
    printformat("\nCTRLDSSEGMENT:              %X", pOpRegs->CTRLDSSEGMENT);
    printformat("\nUSBINTR:                    %X", pOpRegs->USBINTR);
    printformat("\nPERIODICLISTBASE phys addr: %X", pOpRegs->PERIODICLISTBASE);
    printformat("  virt addr: %X", virtualMemoryPERIODICLISTBASE);
    printformat("\nUSBCMD:                     %X", pOpRegs->USBCMD);
    printformat("\nCONFIGFLAG:                 %X", pOpRegs->CONFIGFLAG);

    showUSBSTS();
}

void showPORTSC()
{
    for(uint8_t j=0; j<numPorts; j++)
    {
        if( pOpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE )
        {
            char str[80], PortNumber[5], PortStatus[40];

            itoa(j+1,PortNumber);
            if( pOpRegs->PORTSC[j] & PSTS_CONNECTED )
            {
                strcpy(PortStatus,"Device attached");
            }
            else
            {
                strcpy(PortStatus,"Device not attached");
            }

            str[0]='\0';
            strcpy(str,"Port ");
            strcat(str,PortNumber);
            strcat(str," Status: ");
            strcat(str,PortStatus);
            uint8_t color = 14;
            printf("                                                                              ",46,color);
            printf(str, 46, color); // output to info screen area
            printf("                                                                              ",47,color);
            printf("                                                                              ",48,color);

            pOpRegs->PORTSC[j] |= PSTS_CONNECTED_CHANGE; // reset interrupt
            // settextcolor(15,0);
            beep(1000,500);
        }
    }
}

void showUSBSTS()
{
    settextcolor(15,0);
    printformat("\n\nUSB status: ");
    settextcolor(2,0);
    printformat("%X",pOpRegs->USBSTS);
    settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )             { printformat("\nUSB Interrupt");                 pOpRegs->USBSTS |= STS_USBINT;              }
    if( pOpRegs->USBSTS & STS_USBERRINT )          { printformat("\nUSB Error Interrupt");           pOpRegs->USBSTS |= STS_USBERRINT;           }
    if( pOpRegs->USBSTS & STS_PORT_CHANGE )        { printformat("\nPort Change Detect");            pOpRegs->USBSTS |= STS_PORT_CHANGE;         }
    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER ) { printformat("\nFrame List Rollover");           pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )  { printformat("\nHost System Error");             pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if( pOpRegs->USBSTS & STS_ASYNC_INT )          { printformat("\nInterrupt on Async Advance");    pOpRegs->USBSTS |= STS_ASYNC_INT;           }
    if( pOpRegs->USBSTS & STS_HCHALTED )           { printformat("\nHCHalted");                      pOpRegs->USBSTS |= STS_HCHALTED;            }
    if( pOpRegs->USBSTS & STS_RECLAMATION )        { printformat("\nReclamation");                   pOpRegs->USBSTS |= STS_RECLAMATION;         }
    if( pOpRegs->USBSTS & STS_PERIODIC_ENABLED )   { printformat("\nPeriodic Schedule Status");      pOpRegs->USBSTS |= STS_PERIODIC_ENABLED;    }
    if( pOpRegs->USBSTS & STS_ASYNC_ENABLED )      { printformat("\nAsynchronous Schedule Status");  pOpRegs->USBSTS |= STS_ASYNC_ENABLED;       }
    settextcolor(15,0);
}

void DeactivateLegacySupport(uint32_t number)
{
    // number = 2^3*2^5*bus + 2^3*dev + func
    uint8_t func = (number >> 0) & 0x07;
    uint8_t dev  = (number >> 3) & 0x1F;
    uint8_t bus  = (number >> 8);

    bool failed = false;
    eecp = BYTE2(pCapRegs->HCCPARAMS);
    printformat("DeactivateLegacySupport: eecp = %x\n",eecp);

    if (eecp >= 0x40) // behind standard pci registers, cf. http://wiki.osdev.org/PCI#PCI_Device_Structure
    {
        int32_t eecp_id=0;
        while(eecp)
        {
            eecp_id = pci_config_read( bus, dev, func, 0x0100/*length 1 byte*/ + eecp + 0 );
            if (eecp_id == 1)
            {
                break;
            }
            eecp = pci_config_read( bus, dev, func, 0x0100 + eecp + 1 );
        }

        // Check, whether a Legacy-Support-EC was found and the BIOS-Semaphore is set

        if((eecp_id == 1) && ( pci_config_read( bus, dev, func, 0x0100 + eecp + 2 ) & 0x01))
        {
            // set OS-Semaphore
            pci_config_write_byte( bus, dev, func, eecp + 3, 0x01 );
            failed = true;

            int32_t timeout=0;
            // Wait for BIOS-Semaphore being not set
            while( ( pci_config_read( bus, dev, func, 0x0100 + eecp + 2 ) & 0x01 ) && ( timeout<50 ) )
            {
                sleepMilliSeconds(20);
                timeout++;
            }

            if( !( pci_config_read( bus, dev, func, 0x0100 + eecp + 2 ) & 0x01) )
            {
                // Wait for the OS-Semaphore being set
                timeout=0;
                while( !( pci_config_read( bus, dev, func, 0x0100 + eecp + 3 ) & 0x01) && (timeout<50) )
                {
                    sleepMilliSeconds(20);
                    timeout++;
                }
            }
            if( pci_config_read( bus, dev, func, 0x0100 + eecp + 3 ) & 0x01 )
            {
                failed = false;
            }
            if(failed)
            {
                // Deactivate Legacy Support now
                pci_config_write_dword( bus, dev, func, eecp + 4, 0x0 );
            }
        }
    }
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

