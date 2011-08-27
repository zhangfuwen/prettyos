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


bool UHCIflag = false;          // signals that one UHCI device was found /// TODO: manage more than one UHCI
static pciDev_t* PCIdevice = 0; // pci device
uintptr_t bar;
size_t memSize;

void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_install<<<\n");
  #endif

    if (!UHCIflag) // only the first EHCI is used
    {  
        memSize = memorySize;
        bar = bar_phys;
        PCIdevice = PCIdev; /// TODO: implement for more than one EHCI
        UHCIflag = true; // only the first EHCI is used
        todoList_add(kernel_idleTasks, &uhci_init, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for UHCI ??        
    }
}

// start thread at kernel idle loop (ckernel.c)
void uhci_init(void* data, size_t size)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_init<<<\n");
  #endif
    scheduler_insertTask(create_cthread(&startUHCI, "UHCI"));
}

void startUHCI()
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>startUHCI<<<\n");
  #endif
    initUHCIHostController();
    textColor(LIGHT_MAGENTA);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

int32_t initUHCIHostController()
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>initUHCIHostController<<<\n");
  #endif
    
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
    pci_config_write_dword(bus, dev, func, 0x04, pciCommandRegister /*already set*/ | BIT(0) /*IO-space*/ | BIT(2) /* bus master */); // resets status register, sets command register
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
    if (!(puhci_OpRegs->UHCI_USBSTS & STS_HCHALTED))
    {
         enablePorts();
    }
    else
    {
         textColor(ERROR);
         printf("\nFatal Error: Ports cannot be enabled. HCHalted set.");
         uhci_showUSBSTS();
         textColor(TEXT);
         return -1;
    }
    */
    return 0;
}

void uhci_startHostController(pciDev_t* PCIdev)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_startHostController<<<\n");
  #endif
    
    textColor(TEXT);
    printf("\nStart Host Controller (reset HC).");
    uhci_resetHostController();
}

void uhci_resetHostController()
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_resetHostController<<<\n");
  #endif    
    
    // http://www.lowlevel.eu/wiki/Universal_Host_Controller_Interface#Informationen_vom_PCI-Treiber_holen

    // Zuerst sollte ein globaler USB-Reset auf allen aktivierten Ports getrieben werden. 
    // Dies geschieht, indem das GRESET-Bit im USBCMD-Register (USBCMD.GRESET) gesetzt wird.
    
    // Außerdem kann man hierbei auch gleich alle anderen Bits in diesem Register löschen, sodass der Controller anhält.
    outportw(bar + UHCI_USBCMD, 0x00);
    outportw(bar + UHCI_USBCMD, UHCI_CMD_GRESET); 

    // Nach 50 Millisekunden muss das Resetsignal wieder aufgehoben werden, indem das GRESET-Bit wieder gelöscht wird. 
    sleepMilliSeconds(50);
    outportw(bar + UHCI_USBCMD, 0x00); 
    
    // Jetzt muss die Anzahl der vorhanden Rootports in Erfahrung gebracht werden. 
    // Hierzu nimmt man zunächst die Größe des I/O-Raums minus 0x10 und geteilt durch 2 als Obergrenze und 2 als Untergrenze. 
    // Jetzt wird für jeden Rootport überprüft, ob sowohl Bit 7 gesetzt ist (0x0080) als auch der Gesamtwert nicht 0xFFFF ist. 
    // Ist das Bit nicht gesetzt oder beträgt der Wert 0xFFFF, so handelt es sich beim verwendeten Port um keinen I/O-Port für einen Rootport mehr.
    uint32_t root_ports = (memSize - UHCI_PORTSC1) / 2;
    for (uint32_t i=2; i<root_ports; i++)
    {
        if (!(inportw(bar + UHCI_PORTSC1 + i*2) & 0x0080) || (inportw(bar + UHCI_PORTSC1 + i*2) == 0xFFFF))
        {
            root_ports = i;
            break;
        }
    }
    textColor(IMPORTANT);
    printf("\nUHCI root ports: %u\n", root_ports);
    textColor(TEXT);

    // Deaktivieren des Legacy Supports 
    uhci_DeactivateLegacySupport(PCIdevice);

    // TODO
    /*
        http://www.lowlevel.eu/wiki/Universal_Host_Controller_Interface#Deaktivieren_des_Legacy_Supports
    */
}

void uhci_DeactivateLegacySupport(pciDev_t* PCIdev)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_DeactivateLegacySupport<<<\n");
  #endif   
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
    printf("\n>>>uhci_handler<<<\n");
  #endif   
    textColor(IMPORTANT);
    
    uint16_t reg = bar + UHCI_USBSTS; 
    uint16_t tmp = inportw(reg);
    
    if ( tmp & UHCI_STS_USBINT)
    {
        printf("USB UHCI - USB transaction completed\n");
        outportw(reg, UHCI_STS_USBINT); // reset interrupt      
    }

    if (tmp & UHCI_STS_RESUME_DETECT)
    {
        printf("USB UHCI Resume Detect\n");
        outportw(reg, UHCI_STS_RESUME_DETECT);
    }
    
    textColor(ERROR);

    if (tmp & UHCI_STS_HCHALTED)
    {
        printf("USB UHCI Host Controller Halted\n");
        // outportw(reg, UHCI_STS_HCHALTED); 
    }
    
    if (tmp & UHCI_STS_HC_PROCESS_ERROR)
    {
        printf("USB UHCI Host Controller Process Error\n");
        outportw(reg, UHCI_STS_HC_PROCESS_ERROR); // reset interrupt
    }

    if (tmp & UHCI_STS_USB_ERROR)
    {
        printf("USB UHCI - USB Error\n");
        outportw(reg, UHCI_STS_USB_ERROR); // reset interrupt
    }
    
    if (tmp & UHCI_STS_HOST_SYSTEM_ERROR)
    {
        textColor(ERROR);
        printf("Host System Error\n");
        textColor(TEXT);
        outportw(reg, UHCI_STS_HOST_SYSTEM_ERROR);
        pci_analyzeHostSystemError(PCIdevice);
        textColor(IMPORTANT);
        printf("\n>>> Init UHCI after fatal error:           <<<");
        printf("\n>>> Press key for UHCI (re)initialization. <<<");
        getch();
        textColor(TEXT);
        todoList_add(kernel_idleTasks, &uhci_init, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for UHCI
    }
    textColor(TEXT);
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
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_handler<<<\n");
  #endif   

  
  #ifdef _UHCI_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB status: ");
    textColor(IMPORTANT);
    printf("%xh",inportw(bar+UHCI_USBSTS));
  #endif
    /*
    textColor(ERROR);
    if (puhci_OpRegs->UHCI_USBSTS & STS_USBERRINT)          { printf("\nUSB Error Interrupt");           puhci_OpRegs->UHCI_USBSTS |= STS_USBERRINT;           }
    if (puhci_OpRegs->UHCI_USBSTS & STS_HOST_SYSTEM_ERROR)  { printf("\nHost System Error");             puhci_OpRegs->UHCI_USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if (puhci_OpRegs->UHCI_USBSTS & STS_HCHALTED)           { printf("\nHCHalted");                      puhci_OpRegs->UHCI_USBSTS |= STS_HCHALTED;            }
    textColor(IMPORTANT);
    if (puhci_OpRegs->UHCI_USBSTS & STS_PORT_CHANGE)        { printf("\nPort Change Detect");            puhci_OpRegs->UHCI_USBSTS |= STS_PORT_CHANGE;         }
    if (puhci_OpRegs->UHCI_USBSTS & STS_FRAMELIST_ROLLOVER) { printf("\nFrame List Rollover");           puhci_OpRegs->UHCI_USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if (puhci_OpRegs->UHCI_USBSTS & STS_USBINT)             { printf("\nUSB Interrupt");                 puhci_OpRegs->UHCI_USBSTS |= STS_USBINT;              }
    if (puhci_OpRegs->UHCI_USBSTS & STS_ASYNC_INT)          { printf("\nInterrupt on Async Advance");    puhci_OpRegs->UHCI_USBSTS |= STS_ASYNC_INT;           }
    if (puhci_OpRegs->UHCI_USBSTS & STS_RECLAMATION)        { printf("\nReclamation");                   puhci_OpRegs->UHCI_USBSTS |= STS_RECLAMATION;         }
    if (puhci_OpRegs->UHCI_USBSTS & STS_PERIODIC_ENABLED)   { printf("\nPeriodic Schedule Status");      puhci_OpRegs->UHCI_USBSTS |= STS_PERIODIC_ENABLED;    }
    if (puhci_OpRegs->UHCI_USBSTS & STS_ASYNC_ENABLED)      { printf("\nAsynchronous Schedule Status");  puhci_OpRegs->UHCI_USBSTS |= STS_ASYNC_ENABLED;       }
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
