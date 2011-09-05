/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "ohci.h"
#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "irq.h"
#include "keyboard.h"

//#define OHCI_SCENARIO // ed/td experiments


static uint8_t index   = 0;
static ohci_t* curOHCI = 0;
static ohci_t* ohci[OHCIMAX];
static bool    OHCI_USBtransferFlag = false;


static void ohci_handler(registers_t* r, pciDev_t* device);
static void ohci_start();


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>ohci_install<<<\n");
  #endif

    curOHCI = ohci[index]   = malloc(sizeof(ohci_t), 0, "ohci");
    ohci[index]->PCIdevice  = PCIdev;
    ohci[index]->PCIdevice->data = ohci[index];
    ohci[index]->bar        = (uintptr_t)paging_acquirePciMemory(bar_phys,1);
    uint16_t offset         = bar_phys % PAGESIZE;
    ohci[index]->memSize    = memorySize;
    ohci[index]->num        = index;

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nOHCI_MMIO %Xh mapped to virt addr %Xh, offset: %xh", bar_phys, ohci[index]->bar, offset);
  #endif

    ohci[index]->bar+= offset;
    ohci[index]->OpRegs = (ohci_OpRegs_t*) (ohci[index]->bar);

    char str[10];
    snprintf(str, 10, "OHCI %u", index+1);

    scheduler_insertTask(create_cthread(&ohci_start, str));

    index++;
    sleepMilliSeconds(20); // HACK: Avoid race condition between ohci_install and the thread just created. Problem related to curOHCI global variable
}

static void ohci_start()
{
    ohci_t* o = curOHCI;

  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>startOHCI<<<\n");
  #endif

    ohci_initHC(o);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

void ohci_initHC(ohci_t* o)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>initOHCIHostController<<<\n");
  #endif

    textColor(HEADLINE);
    printf("Initialize OHCI Host Controller:");
    textColor(TEXT);

    // pci bus data
    uint8_t bus  = o->PCIdevice->bus;
    uint8_t dev  = o->PCIdevice->device;
    uint8_t func = o->PCIdevice->func;

    // prepare PCI command register
    // bit 9: Fast Back-to-Back Enable // not necessary
    // bit 2: Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, PCI_COMMAND, 2);
    pci_config_write_dword(bus, dev, func, PCI_COMMAND, pciCommandRegister | PCI_CMD_IO | PCI_CMD_MMIO | PCI_CMD_BUSMASTER); // resets status register, sets command register
    //uint8_t pciCapabilitiesList = pci_config_read(bus, dev, func, PCI_CAPLIST, 1);

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, PCI_COMMAND, 2));
    //printf("\nPCI Capabilities List: first Pointer: %yh", pciCapabilitiesList);
 #endif
    irq_installPCIHandler(o->PCIdevice->irq, ohci_handler, o->PCIdevice);

    OHCI_USBtransferFlag = true;
    o->enabledPorts      = false;

    ohci_resetHC(o);
}

void ohci_resetHC(ohci_t* o)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n\n>>>ohci_resetHostController<<<\n");
  #endif

    /*
    uint8_t bus  = o->PCIdevice->bus;
    uint8_t dev  = o->PCIdevice->device;
    uint8_t func = o->PCIdevice->func;
    */

    // uint16_t legacySupport = pci_config_read(bus, dev, func, OHCI_PCI_LEGACY_SUPPORT, 2);
    // printf("\nLegacy Support Register: %xh", legacySupport); // if value is not zero, Legacy Support (LEGSUP) is activated

    // Revision and Number Downstream Ports (NDP)
    textColor(IMPORTANT);
    printf("\nOHCI: Revision %u.%u, Number Downstream Ports: %u\n",
        BYTE1(o->OpRegs->HcRevision) >> 4,
        BYTE1(o->OpRegs->HcRevision) & 0xF,
        BYTE1(o->OpRegs->HcRhDescriptorA)); // bits 7:0 provide Number Downstream Ports (NDP)
    textColor(TEXT);

    // HCCA
    void* hccaVirt = malloc(sizeof(ohci_HCCA_t), OHCI_HCCA_ALIGN, "ohci HCCA"); // HCCA must be 256-byte aligned
    memset(hccaVirt, 0, sizeof(ohci_HCCA_t));

    // ED Pool: malloc 64 EDs (size: ED)
    // TD Pool: malloc 56 TDs (size: TD+1024)

    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;

    if (o->OpRegs->HcControl & OHCI_CTRL_IR)
    {
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_OCR;

        uint16_t i=0;
        for (i=0; (o->OpRegs->HcControl & OHCI_CTRL_IR) && (i < 1000); i++)
        {
             sleepMilliSeconds(1);
        }

        if (i < 500)
        {
            printf("\nOHCI takes control from SMM.");
        }
        else
        {
            printf("\nOHCI taking control from SMM did not work.");
            o->OpRegs->HcControl &= ~OHCI_CTRL_IR;
        }
    }
    else if ((o->OpRegs->HcControl & OHCI_CTRL_CBSR) != OHCI_USB_RESET)
    {
        printf("\nBIOS active");

        if ((o->OpRegs->HcControl & OHCI_CTRL_CBSR) != OHCI_USB_OPERATIONAL)
        {
            printf("\nActivate RESUME");
            o->OpRegs->HcControl = (o->OpRegs->HcControl & ~OHCI_CTRL_CBSR) | OHCI_USB_RESUME;
            sleepMilliSeconds(10);
         }
     }
    else
    {
        //Neither BIOS nor SMM
        sleepMilliSeconds(10);
    }

    printf("\n\nReset HC\n");

    o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
    sleepMilliSeconds(3); //10 µs reset, 2 ms resume

    if ((o->OpRegs->HcControl & OHCI_CTRL_CBSR) == OHCI_USB_SUSPEND)
    {
        textColor(ERROR);
        printf("\nTimeout!\n");
        textColor(TEXT);
        o->OpRegs->HcControl = (o->OpRegs->HcControl & ~OHCI_CTRL_CBSR) | OHCI_USB_RESUME;
        sleepMilliSeconds(10);
    }

    o->hcca                        = (ohci_HCCA_t*)paging_getPhysAddr(hccaVirt);
  #ifdef _OHCI_DIAGNOSIS_
    printf("\nHCCA (phys. address): %X", o->hcca);
  #endif
    o->OpRegs->HcHCCA = (uintptr_t)o->hcca;
    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;
    o->OpRegs->HcInterruptStatus  = 0xFFFFFFFF;
    o->OpRegs->HcInterruptEnable  = OHCI_INT_SO   | // scheduling overrun
                                    OHCI_INT_WDH  | // write back done head
                                  //OHCI_INT_SF   | // start of frame           // disabled due to qemu
                                    OHCI_INT_RD   | // resume detected
                                    OHCI_INT_UE   | // unrecoverable error
                                    OHCI_INT_FNO  | // frame number overflow
                                    OHCI_INT_RHSC | // root hub status change
                                    OHCI_INT_OC   | // ownership change
                                    OHCI_INT_MIE;   // (de)activates interrupts

    o->OpRegs->HcControl         &= ~(OHCI_CTRL_PLE | OHCI_CTRL_IE);
    o->OpRegs->HcControl         |=   OHCI_CTRL_CLE | OHCI_CTRL_BLE; //activate control and bulk

    o->OpRegs->HcPeriodicStart    = 0x3E67; // When HcFmRemaining reaches this value,
                                            // periodic lists gets priority over control/bulk processing
                                            // The value is calculated as 10% off from HcFmInterval

    printf("\n\nHC will be activated.\n");

    o->OpRegs->HcControl = (o->OpRegs->HcControl & ~OHCI_CTRL_CBSR) | OHCI_USB_OPERATIONAL;
    o->OpRegs->HcRhStatus |= OHCI_RHS_LPSC; // power on
    o->rootPorts = o->OpRegs->HcRhDescriptorA & OHCI_RHA_NDP;
    sleepMilliSeconds((o->OpRegs->HcRhDescriptorA & OHCI_RHA_POTPGT) >> 23);

    textColor(IMPORTANT);
    printf("\n\nFound %i Rootports.\n", o->rootPorts);
    textColor(TEXT);

    for (uint8_t i=0; i < o->rootPorts; i++)
    {
        o->OpRegs->HcRhPortStatus[i] |= OHCI_PORT_CCS; // deactivate
    }

    //
    //
}


/*******************************************************************************************************
*                                                                                                      *
*                                              ohci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

static void ohci_handler(registers_t* r, pciDev_t* device)
{
    // Check if an OHCI controller issued this interrupt
    ohci_t* o = device->data;
    bool found = false;
    uint8_t i;
    for (i=0; i<OHCIMAX; i++)
    {
        if (o == ohci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    volatile uint32_t val = o->OpRegs->HcInterruptStatus;
    uint32_t handled = 0;
    uintptr_t phys;

    if(!found || o==0 || val==0) // No interrupt from corresponding ohci device found
    {
      #ifdef _OHCI_DIAGNOSIS_
        textColor(ERROR);
        printf("interrupt did not come from ohci device!\n");
        textColor(TEXT);
      #endif
        return;
    }

    printf("\nUSB OHCI %u: ", i);

    if (val & OHCI_INT_SO) // scheduling overrun
    {
        printf("\nScheduling overrun.");
        handled |= OHCI_INT_SO;
    }

    if (val & OHCI_INT_WDH) // write back done head
    {
        printf("\nWrite back done head.");
        phys = o->hcca->doneHead;
        // TODO: handle ready transfer (ED, TD)
        handled |= OHCI_INT_WDH;
    }

    if (val & OHCI_INT_SF) // start of frame
    {
        printf("\nStart of frame.");
        handled |= OHCI_INT_SF;
    }

    if (val & OHCI_INT_RD) // resume detected
    {
        printf("\nResume detected.");
        handled |= OHCI_INT_RD;
    }

    if (val & OHCI_INT_UE) // unrecoverable error
    {
        printf("\nUnrecoverable HC error.");
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
        handled |= OHCI_INT_UE;
    }

    if (val & OHCI_INT_FNO) // frame number overflow
    {
        printf("\nFrame number overflow.");
        handled |= OHCI_INT_FNO;
    }

    if (val & OHCI_INT_RHSC) // root hub status change
    {
        printf("\nRoot hub status change.");
        handled |= OHCI_INT_RHSC;
    }

    if (val & OHCI_INT_OC) // ownership change
    {
        printf("\nOwnership change.");
        handled |= OHCI_INT_OC;
    }

    if (val & ~handled)
    {
        printf("\nUnhandled interrupt: %X", val & ~handled);
    }

    o->OpRegs->HcInterruptStatus = val; // reset interrupts
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

// TODO


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
