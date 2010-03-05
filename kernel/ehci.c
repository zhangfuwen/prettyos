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

//uint32_t opregs;
//struct ehci_CapRegs CapRegs;
//struct ehci_OpRegs  OpRegs;
struct ehci_CapRegs* pCapRegs; // = &CapRegs;
struct ehci_OpRegs*  pOpRegs;  // = &OpRegs;

uint32_t ubar;

/*
<Tobiking> Mit "pCapRegs = (struct ehci_OpRegs*)bar;" kann man das Struct direkt auf die Register setzen
*/

void ehci_handler(struct regs* r)
{
	printformat("EHCI-Interrupt: %X\n", pOpRegs->USBSTS);
	settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )
    {
        printformat("\nUSB Interrupt");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_USBINT /*)*/;
    }
    if( pOpRegs->USBSTS & STS_USBERRINT )
    {
        printformat("\nUSB Error Interrupt");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_USBERRINT /*)*/;
    }
    if( pOpRegs->USBSTS & STS_PORT_CHANGE )
    {
        printformat("\nPort Change Detect");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_PORT_CHANGE /*)*/;
    }
    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER )
    {
        printformat("\nFrame List Rollover");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_FRAMELIST_ROLLOVER /*)*/;
    }
    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )
    {
        printformat("\nHost System Error");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_HOST_SYSTEM_ERROR /*)*/;
    }
    if( pOpRegs->USBSTS & STS_ASYNC_INT )
    {
        printformat("\nInterrupt on Async Advance");
        pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_ASYNC_INT/*)*/;
    }
	settextcolor(15,0);
}

void analyzeEHCI(uint32_t bar)
{
    ubar = bar;
    EHCIflag = true;
    pCapRegs = (struct ehci_CapRegs*) ubar;
	pOpRegs  = (struct ehci_OpRegs*) (ubar + pCapRegs->CAPLENGTH);

    // pCapRegs->CAPLENGTH = *((volatile uint8_t* )(bar + 0x00));
    // opregs = pOpRegs->USBCMD; // = bar + pCapRegs->CAPLENGTH;

    // pCapRegs->HCIVERSION = *((volatile uint16_t*)(bar + 0x02));
    printformat("HCIVERSION: %x ", pCapRegs->HCIVERSION); // Interface Version Number

    // pCapRegs->HCSPARAMS = *((volatile uint32_t*)(bar + 0x04));
    numPorts = (pCapRegs->HCSPARAMS & 0x000F);
    printformat("HCSPARAMS: %X ", pCapRegs->HCSPARAMS);   // Structural Parameters
    printformat("Ports: %d ", numPorts);

    // pCapRegs->HCCPARAMS = *((volatile uint32_t*)(bar + 0x08));
    printformat("\nHCCPARAMS: %X ", pCapRegs->HCCPARAMS);   // Capability Parameters

    // if(BYTE2(*((volatile uint32_t*) (bar + 0x08)))==0) printformat("No ext. capabil. "); // Extended Capabilities Pointer
    if(BYTE2(pCapRegs->HCCPARAMS)==0) printformat("No ext. capabil. "); // Extended Capabilities Pointer
    printformat("\nOpRegs Address: %X ", pOpRegs); // Host Controller Operational Registers
}

void initEHCIHostController(uint32_t number)
{
    irq_install_handler(32 + pciDev_Array[number].irq, ehci_handler);

    // Stop HC (bit0 = 0)
    pOpRegs->USBCMD /* = (*((volatile uint32_t*)(opregs + 0x00)) */ &= ~CMD_RUN_STOP /*)*/; // set Run-Stop-Bit to 0

    // wait
    sleepMilliSeconds(30); // wait at least 16 microframes ( = 16*125 µs = 2 ms )

    // resetHC (bit1 = 1)
    pOpRegs->USBCMD /* = (*((volatile uint32_t*)(opregs + 0x00)) */ |= CMD_HCRESET /*)*/;  // set Reset-Bit to 1
    int32_t timeout=0;
    while( /* (*((volatile uint32_t*)(opregs + 0x00)) */ (pOpRegs->USBCMD & 0x2) != 0 )
    {
        printformat("waiting for HC reset\n");
        sleepMilliSeconds(20);
        timeout++;
        if(timeout>10)
            break;
    }

    pOpRegs->USBINTR /* = *((volatile uint32_t*)(opregs + 0x08)) */ = 0; // EHCI interrupts disabled

    // Program the CTRLDSSEGMENT register with 4-Gigabyte segment where all of the interface data structures are allocated.
    pOpRegs->CTRLDSSEGMENT /* = *((volatile uint32_t*)(opregs + 0x10)) */ = 0x0;

    // Write the base address of the Periodic Frame List to the PERIODICLIST BASE register.
    void* virtualMemoryPERIODICLISTBASE  = malloc(0x1000,PAGESIZE);
    void* physicalMemoryPERIODICLISTBASE  = (void*) paging_get_phys_addr(kernel_pd, virtualMemoryPERIODICLISTBASE);
    pOpRegs->PERIODICLISTBASE /* = *((volatile uint32_t*)(opregs + 0x14)) */ = (uint32_t)physicalMemoryPERIODICLISTBASE ;

    // If there are no work items in the periodic schedule, all elements should have their T-Bits set to 1.
    memsetl(virtualMemoryPERIODICLISTBASE, 0x01, 0x400);

    // Write the USBCMD register to set the desired interrupt threshold
    // and turn the host controller ON via setting the Run/Stop bit.
    // Software must not write a one to this field unless the host controller is in the Halted state
    // (i.e. HCHalted in the USBSTS register is a one). Doing so will yield undefined results.
    // pOpRegs->USBSTS = *((volatile uint32_t*)(opregs + 0x04));
    pOpRegs->USBCMD /* = (*((volatile uint32_t*)(opregs + 0x00)) */ |= CMD_8_MICROFRAME /*)*/; // Bits 23-16: 08h
    if( pOpRegs->USBSTS & STS_HCHALTED )
    {
        pOpRegs->USBCMD /* = (*((volatile uint32_t*)(opregs + 0x00)) */ |= CMD_RUN_STOP /*)*/; // set Run-Stop-Bit
    }

    // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller
    pOpRegs->CONFIGFLAG /* = *((volatile uint32_t*)(opregs + 0x40)) */ = CF;

    // enable ports
    for(uint8_t j=0; j<numPorts; j++)
    {
         pOpRegs->PORTSC[j] /* = (*((volatile uint32_t*)(opregs + 0x44 + 4*j)) */ |=  PSTS_POWERON/*)*/;
         pOpRegs->PORTSC[j] /* = (*((volatile uint32_t*)(opregs + 0x44 + 4*j)) */ &= ~PSTS_ENABLED/*)*/;
         pOpRegs->PORTSC[j] /* = (*((volatile uint32_t*)(opregs + 0x44 + 4*j)) */ |=  PSTS_PORT_RESET/*)*/;
         sleepMilliSeconds(50);
         pOpRegs->PORTSC[j] /* = (*((volatile uint32_t*)(opregs + 0x44 + 4*j)) */ &= ~PSTS_PORT_RESET/*)*/;
    }

    // Write the appropriate value to the USBINTR register to enable the appropriate interrupts.
    /// pOpRegs->USBINTR /* = *((volatile uint32_t*)(opregs + 0x08)) */ = STS_INTMASK; // all interrupts allowed  // ---> breakdown!

    printformat("\n\nAfter Init of EHCI:");
    printformat("\nCTRLDSSEGMENT:              %X", /* *((volatile uint32_t*)(opregs + 0x10)) */ pOpRegs->CTRLDSSEGMENT);
    printformat("\nUSBINTR:                    %X", /* *((volatile uint32_t*)(opregs + 0x08)) */ pOpRegs->USBINTR);
    printformat("\nPERIODICLISTBASE phys addr: %X", /* *((volatile uint32_t*)(opregs + 0x14)) */ pOpRegs->PERIODICLISTBASE);
    printformat("  virt addr: %X", virtualMemoryPERIODICLISTBASE);
    printformat("\nUSBCMD:                     %X", /* *((volatile uint32_t*)(opregs + 0x00)) */ pOpRegs->USBCMD);
    printformat("\nCONFIGFLAG:                 %X", /* *((volatile uint32_t*)(opregs + 0x40)) */ pOpRegs->CONFIGFLAG);

    showUSBSTS();
}

void showUSBSTS()
{
    settextcolor(15,0);
    printformat("\n\nUSB status: ");
    // pOpRegs->USBSTS = *((volatile uint32_t*)(opregs + 0x04));
    settextcolor(2,0);
    printformat("%X",pOpRegs->USBSTS);
    settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )             { printformat("\nUSB Interrupt");                 pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_USBINT/*)*/; }
    if( pOpRegs->USBSTS & STS_USBERRINT )          { printformat("\nUSB Error Interrupt");           pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_USBERRINT/*)*/; }
    if( pOpRegs->USBSTS & STS_PORT_CHANGE )        { printformat("\nPort Change Detect");            pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_PORT_CHANGE/*)*/; }
    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER ) { printformat("\nFrame List Rollover");           pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_FRAMELIST_ROLLOVER/*)*/; }
    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )  { printformat("\nHost System Error");             pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_HOST_SYSTEM_ERROR/*)*/; }
    if( pOpRegs->USBSTS & STS_ASYNC_INT )          { printformat("\nInterrupt on Async Advance");    pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_ASYNC_INT/*)*/; }
    if( pOpRegs->USBSTS & STS_HCHALTED )           { printformat("\nHCHalted");                      pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_HCHALTED/*)*/;}
    if( pOpRegs->USBSTS & STS_RECLAMATION )        { printformat("\nReclamation");                   pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_RECLAMATION/*)*/;}
    if( pOpRegs->USBSTS & STS_PERIODIC_ENABLED )   { printformat("\nPeriodic Schedule Status");      pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_PERIODIC_ENABLED/*)*/;}
    if( pOpRegs->USBSTS & STS_ASYNC_ENABLED )      { printformat("\nAsynchronous Schedule Status");  pOpRegs->USBSTS /* = (*((volatile uint32_t*)(opregs + 0x04)) */ |= STS_ASYNC_ENABLED/*)*/;}
    settextcolor(15,0);
}

void showPORTSC()
{
    settextcolor(15,0);
    printformat("\n\nPort status: ");

    settextcolor(3,0);
    for(uint8_t j=0; j<numPorts; j++)
    {
         // pOpRegs->PORTSC[j] = *((volatile uint32_t*)(opregs + 0x44 + 4*j));
         printformat("%x ",pOpRegs->PORTSC[j]);
         if( pOpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE )
         {
             settextcolor(14,0);
             beep(1000,500);
             printformat(" Connect Status Change Port %d\n", j+1); // USB port# starts with 1
             sleepSeconds(2);
             settextcolor(3,0);
             pOpRegs->PORTSC[j] /* = (*((volatile uint32_t*)(opregs + 0x44 + 4*j)) */ |= PSTS_CONNECTED_CHANGE/*)*/; // ack
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

