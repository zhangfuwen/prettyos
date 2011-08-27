/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "todo_list.h"
#include "irq.h"
#include "audio/sys_speaker.h"
#include "keyboard.h"
#include "uhci.h"


bool UHCIflag = false; // signals that one UHCI device was found /// TODO: manage more than one UHCI
static pciDev_t* PCIdevice = 0; // pci device
struct uhci_OpRegs*  puhci_OpRegs;  // = &OpRegs;


void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys)
{
    uintptr_t bar      = (uintptr_t)paging_acquirePciMemory(bar_phys,1);
    uintptr_t offset   = bar_phys % PAGESIZE;

  #ifdef _UHCI_DIAGNOSIS_
    printf("\nUHCI_MMIO %Xh mapped to virt addr %Xh, offset: %xh\n", bar_phys, bar, offset);
  #endif

    if (!UHCIflag) // only the first EHCI is used
    {
        PCIdevice = PCIdev; /// TODO: implement for more than one EHCI
        UHCIflag = true; // only the first EHCI is used

        todoList_add(kernel_idleTasks, &uhci_init, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for UHCI ??

        analyzeUHCI(bar,offset); // get data (opregs)
    }
}

void analyzeUHCI(uintptr_t bar, uintptr_t offset)
{
    bar += offset;
    // pOpRegs  = (struct uhci_OpRegs*) (bar + pCapRegs->CAPLENGTH);
    

  #ifdef _UHCI_DIAGNOSIS_
    /*
    uintptr_t bar_phys  = (uintptr_t)paging_getPhysAddr((void*)bar);
    printf("EHCI bar get_physAddress: %Xh\n", bar_phys);
    printf("HCIVERSION: %xh ",  pCapRegs->HCIVERSION);              // Interface Version Number
    printf("HCSPARAMS: %Xh ",   pCapRegs->HCSPARAMS);               // Structural Parameters
    printf("Ports: %u ",       numPorts);                           // Number of Ports
    printf("\nHCCPARAMS: %Xh ", pCapRegs->HCCPARAMS);               // Capability Parameters
    if (BYTE2(pCapRegs->HCCPARAMS)==0) printf("No ext. capabil. "); // Extended Capabilities Pointer
    printf("\nOpRegs Address: %Xh ", pOpRegs);                      // Host Controller Operational Registers
    */
  #endif
}

// start thread at kernel idle loop (ckernel.c)
void uhci_init(void* data, size_t size)
{
    scheduler_insertTask(create_cthread(&startUHCI, "UHCI"));
}

void startUHCI()
{
    initUHCIHostController();
    textColor(LIGHT_MAGENTA);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

int32_t initUHCIHostController()
{
    textColor(HEADLINE);
    printf("Initialize UHCI Host Controller:");
    textColor(TEXT);

    // pci bus data
    uint8_t bus  = PCIdevice->bus;
    uint8_t dev  = PCIdevice->device;
    uint8_t func = PCIdevice->func;

    

    // prepare PCI command register // offset 0x04
    // bit 9 (0x0200): Fast Back-to-Back Enable // not necessary
    // bit 2 (0x0004): Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, 0x0204);
    pci_config_write_dword(bus, dev, func, 0x04, pciCommandRegister /*already set*/ | BIT(2) /* bus master */); // resets status register, sets command register
    // uint16_t pciCapabilitiesList = pci_config_read(bus, dev, func, 0x0234);

  #ifdef _UHCI_DIAGNOSIS_
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, 0x0204));
    // printf("\nPCI Capabilities List: first Pointer: %xh", pciCapabilitiesList);
 #endif
    irq_installPCIHandler(PCIdevice->irq, uhci_handler, PCIdevice);

    //USBtransferFlag = true;
    //enabledPortFlag = false;

    uhci_startHostController(PCIdevice);

    /*
    if (!(pOpRegs->USBSTS & STS_HCHALTED))
    {
         enablePorts();
    }
    else
    {
         textColor(ERROR);
         printf("\nFatal Error: Ports cannot be enabled. HCHalted set.");
         showUSBSTS();
         textColor(TEXT);
         return -1;
    }
    */
    return 0;
}

void uhci_startHostController(pciDev_t* PCIdev)
{
    textColor(TEXT);
    printf("\nStart Host Controller (reset HC).");

    // resetHostController();

    
}

void uhci_resetHostController()
{
    // TODO
}

void uhci_DeactivateLegacySupport(pciDev_t* PCIdev)
{
    // TODO
}


/*******************************************************************************************************
*                                                                                                      *
*                                              uhci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

void uhci_handler(registers_t* r, pciDev_t* device)
{
  #ifdef _UHCI_DIAGNOSIS_
    /*
    if (!(pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) && !(pOpRegs->USBSTS & STS_USBINT))
    {
        textColor(LIGHT_BLUE);
        printf("\nehci_handler: ");
    }
    */
  #endif

    textColor(YELLOW);

    /*
    if (pOpRegs->USBSTS & STS_USBINT)
    {
        USBINTflag = true; // is asked by polling
        // printf("USB Interrupt");
        pOpRegs->USBSTS |= STS_USBINT; // reset interrupt
    }

    if (pOpRegs->USBSTS & STS_USBERRINT)
    {
        textColor(ERROR);
        printf("USB Error Interrupt");
        textColor(TEXT);
        pOpRegs->USBSTS |= STS_USBERRINT;
    }

    if (pOpRegs->USBSTS & STS_PORT_CHANGE)
    {
        textColor(LIGHT_BLUE);
        printf("Port Change");
        textColor(TEXT);

        pOpRegs->USBSTS |= STS_PORT_CHANGE;

        if (enabledPortFlag && PCIdevice)
        {
            todoList_add(kernel_idleTasks, &ehci_portcheck, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for EHCI
        }
    }

    if (pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER)
    {
        //printf("Frame List Rollover Interrupt");
        pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;
    }

    if (pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)
    {
        textColor(ERROR);
        printf("Host System Error");
        textColor(TEXT);
        pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;
        pci_analyzeHostSystemError(PCIdevice);
        textColor(IMPORTANT);
        printf("\n>>> Init EHCI after fatal error:           <<<");
        printf("\n>>> Press key for EHCI (re)initialization. <<<");
        getch();
        textColor(TEXT);
        todoList_add(kernel_idleTasks, &ehci_init, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for EHCI
    }

    if (pOpRegs->USBSTS & STS_ASYNC_INT)
    {
      #ifdef _EHCI_DIAGNOSIS_
        printf("Interrupt on Async Advance");
      #endif
        pOpRegs->USBSTS |= STS_ASYNC_INT;
    }
    */
}



/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/



/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/


void uhci_showUSBSTS()
{
  /*
  #ifdef _EHCI_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB status: ");
    textColor(IMPORTANT);
    printf("%Xh",pOpRegs->USBSTS);
  #endif
    textColor(ERROR);
    if (pOpRegs->USBSTS & STS_USBERRINT)          { printf("\nUSB Error Interrupt");           pOpRegs->USBSTS |= STS_USBERRINT;           }
    if (pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)  { printf("\nHost System Error");             pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if (pOpRegs->USBSTS & STS_HCHALTED)           { printf("\nHCHalted");                      pOpRegs->USBSTS |= STS_HCHALTED;            }
    textColor(IMPORTANT);
    if (pOpRegs->USBSTS & STS_PORT_CHANGE)        { printf("\nPort Change Detect");            pOpRegs->USBSTS |= STS_PORT_CHANGE;         }
    if (pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) { printf("\nFrame List Rollover");           pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if (pOpRegs->USBSTS & STS_USBINT)             { printf("\nUSB Interrupt");                 pOpRegs->USBSTS |= STS_USBINT;              }
    if (pOpRegs->USBSTS & STS_ASYNC_INT)          { printf("\nInterrupt on Async Advance");    pOpRegs->USBSTS |= STS_ASYNC_INT;           }
    if (pOpRegs->USBSTS & STS_RECLAMATION)        { printf("\nReclamation");                   pOpRegs->USBSTS |= STS_RECLAMATION;         }
    if (pOpRegs->USBSTS & STS_PERIODIC_ENABLED)   { printf("\nPeriodic Schedule Status");      pOpRegs->USBSTS |= STS_PERIODIC_ENABLED;    }
    if (pOpRegs->USBSTS & STS_ASYNC_ENABLED)      { printf("\nAsynchronous Schedule Status");  pOpRegs->USBSTS |= STS_ASYNC_ENABLED;       }
    textColor(TEXT);
    */
}

/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
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
