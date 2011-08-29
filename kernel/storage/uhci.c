/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "uhci.h"
#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "todo_list.h"
#include "irq.h"
#include "audio/sys_speaker.h"
#include "keyboard.h"


static uint8_t index   = 0;
static uhci_t* curUHCI = 0;
static uhci_t* uhci[UHCIMAX];

void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_install<<<\n");
  #endif

    curUHCI = uhci[index]   = (uhci_t*)malloc(sizeof(uhci_t),0,"uhci");
    uhci[index]->PCIdevice  = PCIdev;
    uhci[index]->bar        = bar_phys;
    uhci[index]->memSize    = memorySize;

    todoList_add(kernel_idleTasks, &uhci_init, 0, 0, 0); // HACK: RTL8139 generates interrupts (endless) if its not used for UHCI ??

    index++;
}

// start thread at kernel idle loop (ckernel.c)
void uhci_init(void* data, size_t size)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_init<<<\n");
  #endif
    scheduler_insertTask(create_cthread(&startUHCI, "UHCI"));
    sleepMilliSeconds(10); // HACK: Avoid race condition between uhci_init and the thread just created. Problem related to curUHCI global variable
}

void startUHCI()
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>startUHCI<<<\n");
  #endif
    uhci_t* u = curUHCI;
    initUHCIHostController(u);
    textColor(IMPORTANT);
    printf("\n>>> Press key to close this console. <<<");
    textColor(TEXT);
    getch();
}

int32_t initUHCIHostController(uhci_t* u)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>initUHCIHostController<<<\n");
  #endif

    textColor(HEADLINE);
    printf("Initialize UHCI Host Controller:");
    textColor(TEXT);

    // pci bus data
    uint8_t bus  = u->PCIdevice->bus;
    uint8_t dev  = u->PCIdevice->device;
    uint8_t func = u->PCIdevice->func;

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
    irq_installPCIHandler(u->PCIdevice->irq, uhci_handler, u->PCIdevice);

    //USBtransferFlag = true;
    //enabledPortFlag = false;

    uhci_resetHostController(u);

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

void uhci_resetHostController(uhci_t* u)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_resetHostController<<<\n");
  #endif

    uint8_t bus  = u->PCIdevice->bus;
    uint8_t dev  = u->PCIdevice->device;
    uint8_t func = u->PCIdevice->func;

    // http://www.lowlevel.eu/wiki/Universal_Host_Controller_Interface#Informationen_vom_PCI-Treiber_holen

    uint16_t val = pci_config_read(bus, dev, func, 0x02C0);
    printf("\nLegacy Support Register: %xh",val); // if value is not zero, Legacy Support (LEGSUP) is activated

    outportw(u->bar + UHCI_USBCMD, 0x00); // perhaps not necessary
    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_GRESET);
    sleepMilliSeconds(100); // at least 50 msec
    outportw(u->bar + UHCI_USBCMD, 0x00);
    sleepMilliSeconds(20);  // at least 10 msec

    // get number of root ports
    u->rootPorts = (u->memSize - UHCI_PORTSC1) / 2;
    for (uint32_t i=2; i<u->rootPorts; i++)
    {
        if (!(inportw(u->bar + UHCI_PORTSC1 + i*2) & 0x0080) || (inportw(u->bar + UHCI_PORTSC1 + i*2) == 0xFFFF))
        {
            u->rootPorts = i;
            break;
        }
    }
    textColor(IMPORTANT);
    printf("\nUHCI root ports: %u\n", u->rootPorts);
    textColor(TEXT);

    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_HCRESET); // Reset

    uint8_t timeout = 10;
    while (inportw (u->bar + UHCI_USBCMD) & UHCI_CMD_HCRESET)
    {
        if (timeout==0)
        {
            textColor(ERROR);
            printf("USBCMD_HCRESET timed out!");
            break;
        }
        sleepMilliSeconds(1);
        timeout--;
    }

    // turn on all interrupts
    outportw(u->bar + UHCI_USBINTR, UHCI_INT_TIMEOUT_ENABLE | UHCI_INT_RESUME_ENABLE |
                                    UHCI_INT_IOC_ENABLE     | UHCI_INT_SHORT_PACKET_ENABLE );
    sleepMilliSeconds(1);

    // resets support status bits in Legacy support register
    pci_config_write_word(bus, dev, func, UHCI_PCI_LEGACY_SUPPORT, UHCI_PCI_LEGACY_SUPPORT_STATUS);

    // frame timespan
    outportb(u->bar + UHCI_SOFMOD, 0x40);

    // start at frame 0 and provide phys. addr. of frame list
    outportw(u->bar + UHCI_FRNUM, 0x00);

    u->framelistAddrVirt = (uintptr_t)malloc(PAGESIZE,PAGESIZE,"uhci-framelist");
    u->framelistAddrPhys = paging_getPhysAddr((void*)u->framelistAddrVirt);
    printf("\nFrame list physical address (must be page-aligned): %Xh", u->framelistAddrPhys);
    outportl(u->bar + UHCI_FRBASEADD, u->framelistAddrPhys);

    // switch off the ports
    for (uint8_t i=0; i<u->rootPorts; i++)
    {
        outportw(u->bar + UHCI_PORTSC1 + i*2, UHCI_PORT_CS_CHANGE);
    }

    // generate PCI IRQs
    pci_config_write_word(bus, dev, func, UHCI_PCI_LEGACY_SUPPORT, UHCI_PCI_LEGACY_SUPPORT_PIRQ);
	
    val = pci_config_read(bus, dev, func, 0x02C0);
    printf("\nLegacy Support Register: %xh",val); // if value is not zero, Legacy Support (LEGSUP) is activated

    // Run and mark it configured with a 64-byte max packet
    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP);

    outportw(u->bar + UHCI_USBCMD, inportw(u->bar + UHCI_USBCMD) | UHCI_CMD_FGR);
    sleepMilliSeconds(20);
    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP);
    sleepMilliSeconds(10);

    textColor(SUCCESS);
    printf("\nUHCI ready");
    textColor(TEXT);
}


/*******************************************************************************************************
*                                                                                                      *
*                                              uhci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

void uhci_handler(registers_t* r, pciDev_t* device)
{
  #ifdef _UHCI_DIAGNOSIS_
    textColor(TEXT);
    printf("\n>>>uhci_handler<<<\n");
  #endif

    uhci_t* u = 0; // find UHCI that issued the interrupt
    
    for (uint8_t i=0; i<UHCIMAX; i++)
    {
        if (u->PCIdevice == device)
        {
            u = uhci[i];
            printf("USB UHCI %u: ", i);
            break;
        }
    }
    
    if(!u) // No interrupt from uhci device found
    {
      #ifdef _UHCI_DIAGNOSIS_
        textColor(ERROR);
        printf("interrupt did not come from uhci device!\n");
      #endif
        return; 
    }

    textColor(IMPORTANT);

    uint16_t reg = u->bar + UHCI_USBSTS;
    uint16_t tmp = inportw(reg);

    if (tmp & UHCI_STS_USBINT)
    {
        printf("USB transaction completed\n");
        outportw(reg, UHCI_STS_USBINT); // reset interrupt
    }
    if (tmp & UHCI_STS_RESUME_DETECT)
    {
        printf("Resume Detect\n");
        outportw(reg, UHCI_STS_RESUME_DETECT); // reset interrupt
    }
    if (tmp & UHCI_STS_HCHALTED)
    {
        textColor(ERROR);
        printf("Host Controller Halted\n");
        outportw(reg, UHCI_STS_HCHALTED); // reset interrupt
    }
    if (tmp & UHCI_STS_HC_PROCESS_ERROR)
    {
        textColor(ERROR);
        printf("Host Controller Process Error\n");
        outportw(reg, UHCI_STS_HC_PROCESS_ERROR); // reset interrupt
    }
    if (tmp & UHCI_STS_USB_ERROR)
    {
        textColor(ERROR);
        printf("USB Error\n");
        outportw(reg, UHCI_STS_USB_ERROR); // reset interrupt
    }
    if (tmp & UHCI_STS_HOST_SYSTEM_ERROR)
    {
        textColor(ERROR);
        printf("Host System Error\n");
        outportw(reg, UHCI_STS_HOST_SYSTEM_ERROR); // reset interrupt
        pci_analyzeHostSystemError(u->PCIdevice);
    }
    if (tmp == 0)
        printf("Invalid interrupt");
    else
    {
        textColor(IMPORTANT);
        printf("%x", tmp);
    }
    textColor(TEXT);
}




/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/

// TODO

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
