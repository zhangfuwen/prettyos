/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"
#include "ehci.h"
#include "kheap.h"
#include "paging.h"

extern page_directory_t* kernel_pd;

uint32_t opregs;
struct ehci_CapRegs CapRegs;
struct ehci_OpRegs  OpRegs;
struct ehci_CapRegs* pCapRegs = &CapRegs;
struct ehci_OpRegs*  pOpRegs  = &OpRegs;

/*
<Tobiking>Mit "pCapRegs = (struct ehci_OpRegs*)bar;"
<Tobiking>kann man das Struct direkt auf die Register setzen
*/

void analyzeEHCI(uint32_t bar)
{
    EHCIflag = true;

    pCapRegs->CAPLENGTH = *((volatile uint8_t* )(bar + 0x00));
    opregs = pOpRegs->USBCMD = bar + pCapRegs->CAPLENGTH;

    pCapRegs->HCIVERSION = *((volatile uint16_t*)(bar + 0x02));
    printformat(  "HCIVERSION: %x ", pCapRegs->HCIVERSION); // Interface Version Number

    pCapRegs->HCSPARAMS = *((volatile uint32_t*)(bar + 0x04));
    printformat(  "HCSPARAMS: %X ", pCapRegs->HCSPARAMS);   // Structural Parameters

    pCapRegs->HCCPARAMS = *((volatile uint32_t*)(bar + 0x08));
    printformat(  "HCCPARAMS: %X ", pCapRegs->HCCPARAMS);   // Capability Parameters

    if(BYTE2(*((volatile uint32_t*) (bar + 0x08)))==0) printformat("No ext. capabil. "); // Extended Capabilities Pointer

    printformat("\nOpRegs Address: %X ", opregs); // Host Controller Operational Registers
}

void initEHCIHostController()
{
    // Program the CTRLDSSEGMENT register with 4-Gigabyte segment where all of the interface data structures are allocated.
    pOpRegs->CTRLDSSEGMENT = *((volatile uint32_t*)(opregs + 0x10)) = 0x0;

    // Write the appropriate value to the USBINTR register to enable the appropriate interrupts.
    // pOpRegs->USBINTR       = *((volatile uint32_t*)(opregs + 0x08)) = 0x3F; // 63 = 00111111b
    pOpRegs->USBINTR       = *((volatile uint32_t*)(opregs + 0x08)) = 0x1 | 0x2 | 0x4 |/*0x8 |*/ 0x10 | 0x20;
    /// TEST: Bit 3 cannot be set to 1, Frame List Rollover Enable, otherwise keyboard does not come through

    // Write the base address of the Periodic Frame List to the PERIODICLIST BASE register.
    uint32_t virtualMemoryPERIODICLISTBASE = (uint32_t) malloc(0x1000,PAGESIZE);
    uint32_t physicalMemoryPERIODICLISTBASE = paging_get_phys_addr( kernel_pd, (void*)virtualMemoryPERIODICLISTBASE );
    pOpRegs->PERIODICLISTBASE = *((volatile uint32_t*)(opregs + 0x14)) = physicalMemoryPERIODICLISTBASE;

    // If there are no work items in the periodic schedule,
    // all elements of the Periodic Frame List should have their T-Bits set to 1.

    for(int numElement=0; numElement<1024; numElement++)
    {
         *((volatile uint32_t*)(virtualMemoryPERIODICLISTBASE + 4*numElement)) |= 0x1;
    }
    printformat("\nFirst Periodic List Element: %X\n",*(volatile uint32_t*)virtualMemoryPERIODICLISTBASE);

    // Write the USBCMD register to set the desired interrupt threshold
    // and turn the host controller ON via setting the Run/Stop bit.
    // Software must not write a one to this field unless the host controller is in the Halted state
    // (i.e. HCHalted in the USBSTS register is a one). Doing so will yield undefined results.
    pOpRegs->USBSTS = *((volatile uint32_t*)(opregs + 0x04));
    pOpRegs->USBCMD = (*((volatile uint32_t*)(opregs + 0x00)) |= (0x8<<16) ); // Bits 23-16: 08h, means 8 micro-frames
    if( pOpRegs->USBSTS & (1<<12) )
    {
        pOpRegs->USBCMD = (*((volatile uint32_t*)(opregs + 0x00)) |=  0x1      ); // set Run-Stop-Bit
    }

    // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller
    pOpRegs->CONFIGFLAG    = *((volatile uint32_t*)(opregs + 0x40)) = 1;

    printformat("\n\nAfter Init of EHCI:");
    printformat("\nCTRLDSSEGMENT:              %X", *((volatile uint32_t*)(opregs + 0x10)) );
    printformat("\nUSBINTR:                    %X", *((volatile uint32_t*)(opregs + 0x08)) );
    printformat("\nPERIODICLISTBASE phys addr: %X", *((volatile uint32_t*)(opregs + 0x14)) );
    printformat("  virt addr: %X", virtualMemoryPERIODICLISTBASE);
    printformat("\nUSBCMD:                     %X", *((volatile uint32_t*)(opregs + 0x00)) );
    printformat("\nCONFIGFLAG:                 %X", *((volatile uint32_t*)(opregs + 0x40)) );
    showUSBSTS();
}

void showUSBSTS()
{
    settextcolor(13,0);
    printformat("\n\nFetching USB status register: ");
    pOpRegs->USBSTS = *((volatile uint32_t*)(opregs + 0x04));
    settextcolor(3,0);
    printformat("%X",pOpRegs->USBSTS);
    if( pOpRegs->USBSTS & (1<<0) )  { printformat("\nUSB Interrupt");                 pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<0)); }
    if( pOpRegs->USBSTS & (1<<1) )  { printformat("\nUSB Error Interrupt");           pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<1)); }
    if( pOpRegs->USBSTS & (1<<2) )  { printformat("\nPort Change Detect");            pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<2)); }
    if( pOpRegs->USBSTS & (1<<3) )  { printformat("\nFrame List Rollover");           pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<3)); }
    if( pOpRegs->USBSTS & (1<<4) )  { printformat("\nHost System Error");             pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<4)); }
    if( pOpRegs->USBSTS & (1<<5) )  { printformat("\nInterrupt on Async Advance");    pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<5)); }
    if( pOpRegs->USBSTS & (1<<12) ) { printformat("\nHCHalted");                      pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<12));}
    if( pOpRegs->USBSTS & (1<<13) ) { printformat("\nReclamation");                   pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<13));}
    if( pOpRegs->USBSTS & (1<<14) ) { printformat("\nPeriodic Schedule Status");      pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<14));}
    if( pOpRegs->USBSTS & (1<<15) ) { printformat("\nAsynchronous Schedule Status");  pOpRegs->USBSTS = (*((volatile uint32_t*)(opregs + 0x04)) |= (1<<15));}
    settextcolor(15,0);
}

void showPORTSC()
{
    settextcolor(14,0);
    printformat("\n\nFetching Port status register: ");

    uint8_t numPorts = (pCapRegs->HCSPARAMS & 0x000F);
    printformat("number of ports: %d\n", numPorts);

    settextcolor(3,0);
    for(uint8_t j=1; j<=numPorts; j++)
    {
         pOpRegs->PORTSC[j] = (*((volatile uint32_t*)(opregs + 0x44 + 4*(j-1))) |= (1<<12)); // power on
         printformat("%X\t",pOpRegs->PORTSC[j]);
         if( pOpRegs->PORTSC[j] & (1<<1) )
         {
             settextcolor(12,0);
             printformat(" Connect Status Change Port %d\n", j);
             sleepSeconds(5);
             settextcolor(3,0);
             pOpRegs->PORTSC[j] = (*((volatile uint32_t*)(opregs + 0x44 + 4*(j-1))) |= (1<<1));
         }
    }
    settextcolor(15,0);
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

