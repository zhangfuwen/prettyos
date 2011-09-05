/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "uhci.h"
#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "irq.h"
#include "keyboard.h"

//#define UHCI_SCENARIO // qh/td experiments


static uint8_t index   = 0;
static uhci_t* curUHCI = 0;
static uhci_t* uhci[UHCIMAX];
static bool    UHCI_USBtransferFlag = false;


static void uhci_start();
static void uhci_showPortState(uhci_t* u, uint8_t port);
static void uhci_handler(registers_t* r, pciDev_t* device);


void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>uhci_install<<<");
  #endif

    curUHCI = uhci[index]   = malloc(sizeof(uhci_t), 0, "uhci");
    uhci[index]->PCIdevice  = PCIdev;
    uhci[index]->PCIdevice->data = uhci[index];
    uhci[index]->bar        = bar_phys;
    uhci[index]->memSize    = memorySize;
    uhci[index]->num        = index;

    char str[10];
    snprintf(str, 10, "UHCI %u", index+1);

    scheduler_insertTask(create_cthread(&uhci_start, str));

    index++;
    sleepMilliSeconds(20); // HACK: Avoid race condition between uhci_install and the thread just created. Problem related to curUHCI global variable
}

static void uhci_start()
{
    uhci_t* u = curUHCI;

  #ifdef _UHCI_DIAGNOSIS_
    printf("\n>>>startUHCI<<<\n");
  #endif

    uhci_initHC(u);
    textColor(TEXT);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

void uhci_initHC(uhci_t* u)
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

    // prepare PCI command register
    // bit 9: Fast Back-to-Back Enable // not necessary
    // bit 2: Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, PCI_COMMAND, 2);
    pci_config_write_dword(bus, dev, func, PCI_COMMAND, pciCommandRegister | PCI_CMD_IO | PCI_CMD_BUSMASTER); // resets status register, sets command register
    //uint8_t pciCapabilitiesList = pci_config_read(bus, dev, func, PCI_CAPLIST, 1);

  #ifdef _UHCI_DIAGNOSIS_
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, PCI_COMMAND, 2));
    //printf("\nPCI Capabilities List: first Pointer: %yh", pciCapabilitiesList);
 #endif
    irq_installPCIHandler(u->PCIdevice->irq, uhci_handler, u->PCIdevice);

    UHCI_USBtransferFlag = true;
    u->enabledPorts      = false;

    uhci_resetHC(u);
}

void uhci_resetHC(uhci_t* u)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n\n>>>uhci_resetHostController<<<\n");
  #endif

    uint8_t bus  = u->PCIdevice->bus;
    uint8_t dev  = u->PCIdevice->device;
    uint8_t func = u->PCIdevice->func;

    // http://www.lowlevel.eu/wiki/Universal_Host_Controller_Interface#Informationen_vom_PCI-Treiber_holen


    uint16_t legacySupport = pci_config_read(bus, dev, func, UHCI_PCI_LEGACY_SUPPORT, 2);
    // printf("\nLegacy Support Register: %xh", legacySupport); // if value is not zero, Legacy Support (LEGSUP) is activated

    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_GRESET);
    sleepMilliSeconds(50); // at least 50 msec
    outportw(u->bar + UHCI_USBCMD, 0);

    // get number of valid root ports
    u->rootPorts = (u->memSize - UHCI_PORTSC1) / 2; // each port has a two byte PORTSC register
    for (uint32_t i=2; i<u->rootPorts; i++)
    {
        if (((inportw(u->bar + UHCI_PORTSC1 + i*2) & UHCI_PORT_VALID) == 0) || // reserved bit 7 is already read as 1
             (inportw(u->bar + UHCI_PORTSC1 + i*2) == 0xFFFF))
        {
            u->rootPorts = i;
            break;
        }
    }

    if (u->rootPorts > UHCIMAX)
    {
        u->rootPorts = UHCIMAX;
    }

  #ifdef _UHCI_DIAGNOSIS_
    textColor(IMPORTANT);
    printf("\nUHCI root ports: %u", u->rootPorts);
    textColor(TEXT);
  #endif

    uint16_t uhci_usbcmd = inportw(u->bar + UHCI_USBCMD);
    if ((legacySupport & ~(UHCI_PCI_LEGACY_SUPPORT_STATUS | UHCI_PCI_LEGACY_SUPPORT_NO_CHG | UHCI_PCI_LEGACY_SUPPORT_PIRQ)) ||
         (uhci_usbcmd & UHCI_CMD_RS)   ||
         (uhci_usbcmd & UHCI_CMD_CF)   ||
        !(uhci_usbcmd & UHCI_CMD_EGSM) ||
         (inportw(u->bar + UHCI_USBINTR) & UHCI_INT_MASK))
    {
        outportw(u->bar + UHCI_USBSTS, UHCI_STS_MASK);    // reset all status bits
        sleepMilliSeconds(1);                             // wait one frame
        pci_config_write_word(bus, dev, func, UHCI_PCI_LEGACY_SUPPORT, UHCI_PCI_LEGACY_SUPPORT_STATUS); // resets support status bits in Legacy support register
        outportw(u->bar + UHCI_USBCMD, UHCI_CMD_HCRESET); // reset hostcontroller

        uint8_t timeout = 50;
        while (inportw (u->bar + UHCI_USBCMD) & UHCI_CMD_HCRESET)
        {
            if (timeout==0)
            {
                textColor(ERROR);
                printf("USBCMD_HCRESET timed out!");
                break;
            }
            sleepMilliSeconds(10);
            timeout--;
        }

        outportw(u->bar + UHCI_USBINTR, 0); // switch off all interrupts
        outportw(u->bar + UHCI_USBCMD,  0); // switch off the host controller

        for (uint8_t i=0; i<u->rootPorts; i++) // switch off the valid root ports
        {
            outportw(u->bar + UHCI_PORTSC1 + i*2, 0);
        }
    }


    // frame list
    u->framelistAddrVirt = (frPtr_t*)malloc(PAGESIZE, PAGESIZE, "uhci-framelist");
    u->framelistAddrPhys = paging_getPhysAddr((void*)u->framelistAddrVirt);
    // TODO: mutex for frame list


#ifdef UHCI_SCENARIO
    uhci_QH_t* qhIn  = malloc(sizeof(uhci_QH_t),16,"uhci-QH");
    uhci_QH_t* qhOut = malloc(sizeof(uhci_QH_t),16,"uhci-QH");

    uhci_TD_t* tdIn  = malloc(sizeof(uhci_TD_t),16,"uhci-TD");
    uhci_TD_t* tdOut = malloc(sizeof(uhci_TD_t),16,"uhci-TD");

    qhIn->next       = paging_getPhysAddr((void*)qhOut)| BIT_QH;
    qhIn->transfer   = paging_getPhysAddr((void*)tdIn);
    qhIn->q_first    = 0;
    qhIn->q_last     = 0;

    tdIn->next       = BIT_T;
    tdIn->buffer     = paging_getPhysAddr(malloc(0x1000,0,"uhci-TDbuffer"));
    tdIn->active     = 1;
    tdIn->intOnComplete = 1;

    qhOut->next       = BIT_T;
    qhOut->transfer   = paging_getPhysAddr((void*)tdOut);
    qhOut->q_first    = 0;
    qhOut->q_last     = 0;

    tdOut->next      = BIT_T;
    tdOut->buffer    = paging_getPhysAddr(malloc(0x1000,0,"uhci-TDbuffer"));
    tdOut->active    = 1;
    tdOut->intOnComplete = 1;
#else
    uhci_QH_t* qhIn  = malloc(sizeof(uhci_QH_t),PAGESIZE,"uhci-QH");
    qhIn->next       = BIT_T;
    qhIn->transfer   = BIT_T;
    qhIn->q_first    = 0;
    qhIn->q_last     = 0;
#endif

    // ---------------------------

    for (uint16_t i=0; i<1024; i++)
    {
       u->framelistAddrVirt->frPtr[i] = paging_getPhysAddr(qhIn) | BIT_QH;
    }

    // define each millisecond one frame, provide physical address of frame list, and start at frame 0
    outportb(u->bar + UHCI_SOFMOD, 0x40); // SOF cycle time: 12000. For a 12 MHz SOF counter clock input, this produces a 1 ms Frame period.
    outportl(u->bar + UHCI_FRBASEADD, u->framelistAddrPhys);
    outportw(u->bar + UHCI_FRNUM, 0x0000);

    // set PIRQ
    pci_config_write_word(bus, dev, func, UHCI_PCI_LEGACY_SUPPORT, UHCI_PCI_LEGACY_SUPPORT_PIRQ);

    // start hostcontroller and mark it configured with a 64-byte max packet
    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP);
    outportw(u->bar + UHCI_USBINTR, UHCI_INT_MASK ); // switch on all interrupts

    for (uint8_t i=0; i<u->rootPorts; i++) // reset the CSC of the valid root ports
    {
        outportw(u->bar + UHCI_PORTSC1 + i*2, UHCI_PORT_CS_CHANGE);
    }

    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP | UHCI_CMD_FGR);
    sleepMilliSeconds(20);
    outportw(u->bar + UHCI_USBCMD, UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP);
    sleepMilliSeconds(100);

  #ifdef _UHCI_DIAGNOSIS_
    printf("\n\nRoot-Ports   port1: %xh  port2: %xh\n", inportw (u->bar + UHCI_PORTSC1), inportw (u->bar + UHCI_PORTSC2));
  #endif

    u->run = inportw(u->bar + UHCI_USBCMD) & UHCI_CMD_RS;
    if (!(inportw(u->bar + UHCI_USBSTS) & UHCI_STS_HCHALTED))
    {
        textColor(SUCCESS);
        printf("\n\nRunStop bit: %u\n", u->run);
        textColor(TEXT);
        uhci_enablePorts(u); // attaches the ports
    }
    else
    {
        textColor(ERROR);
        printf("\nFatal Error: UHCI - HCHalted. Ports will not be enabled.");
        textColor(TEXT);
        printf("\nRunStop Bit: %u  Frame Number: %u", u->run, inportw(u->bar + UHCI_FRNUM));
    }
}

// ports
void uhci_enablePorts(uhci_t* u)
{
  #ifdef _UHCI_DIAGNOSIS_
    printf("\n\n>>>uhci_enablePorts<<<\n");
  #endif

    for (uint8_t j=0; j<u->rootPorts; j++)
    {
        uhci_resetPort(u, j);
        u->enabledPorts = true;

        u->port[j].type = &USB_UHCI; // device manager
        u->port[j].data = u;
        u->port[j].insertedDisk = 0;
        snprintf(u->port[j].name, 14, "UHCI-Port %u", j+1);
        attachPort(&u->port[j]);

        uhci_showPortState(u, j);
    }
}


void uhci_resetPort(uhci_t* u, uint8_t port)
{
    outportw(u->bar + UHCI_PORTSC1+2*port,UHCI_PORT_RESET);
    sleepMilliSeconds(50); // do not delete this wait
    outportw(u->bar + UHCI_PORTSC1+2*port, inportw(u->bar + UHCI_PORTSC1+2*port) & ~UHCI_PORT_RESET); // clear reset bit
    outportw(u->bar + UHCI_PORTSC1+2*port, UHCI_PORT_ENABLE_CHANGE|UHCI_PORT_CS_CHANGE|UHCI_PORT_ENABLE); // Clear bit 1 & 3, and Set bit 2 [Enable]

    // wait and check, whether reset bit is really zero
    uint32_t timeout=20;
    while ((inportw(u->bar + UHCI_PORTSC1+2*port) & UHCI_PORT_RESET) != 0)
    {
        sleepMilliSeconds(20);
        timeout--;
        if (timeout == 0)
        {
            textColor(ERROR);
            printf("\nTimeour Error: Port %u did not reset! ", port+1);
            textColor(TEXT);
          #ifdef _UHCI_DIAGNOSIS_
            printf("Port Status: %Xh", inportw(u->bar + UHCI_PORTSC1+2*port));
          #endif
            break;
        }
    }
    sleepMilliSeconds(10);
}


/*******************************************************************************************************
*                                                                                                      *
*                                              uhci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

static void uhci_handler(registers_t* r, pciDev_t* device)
{
    // Check if an UHCI controller issued this interrupt
    uhci_t* u = device->data;
    bool found = false;
    uint8_t i;
    for (i=0; i<UHCIMAX; i++)
    {
        if (u == uhci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    uint16_t reg = u->bar + UHCI_USBSTS;
    uint16_t val = inportw(reg);

    if(!found || u==0 || val==0) // No interrupt from corresponding uhci device found
    {
      #ifdef _UHCI_DIAGNOSIS_
        textColor(ERROR);
        printf("interrupt did not come from uhci device!\n");
        textColor(TEXT);
      #endif
        return;
    }

    printf("\nUSB UHCI %u: ", i);

    textColor(IMPORTANT);

    if (val & UHCI_STS_USBINT)
    {
        printf("Frame: %u - USB transaction completed", inportw(u->bar + UHCI_FRNUM));
        outportw(reg, UHCI_STS_USBINT); // reset interrupt
    }
    if (val & UHCI_STS_RESUME_DETECT)
    {
        printf("Resume Detect");
        outportw(reg, UHCI_STS_RESUME_DETECT); // reset interrupt
    }

    textColor(ERROR);

    if (val & UHCI_STS_HCHALTED)
    {
        printf("Host Controller Halted");
        outportw(reg, UHCI_STS_HCHALTED); // reset interrupt
    }
    if (val & UHCI_STS_HC_PROCESS_ERROR)
    {
        printf("Host Controller Process Error");
        outportw(reg, UHCI_STS_HC_PROCESS_ERROR); // reset interrupt
    }
    if (val & UHCI_STS_USB_ERROR)
    {
        printf("USB Error");
        outportw(reg, UHCI_STS_USB_ERROR); // reset interrupt
    }
    if (val & UHCI_STS_HOST_SYSTEM_ERROR)
    {
        printf("Host System Error");
        outportw(reg, UHCI_STS_HOST_SYSTEM_ERROR); // reset interrupt
        pci_analyzeHostSystemError(u->PCIdevice);
    }
    textColor(TEXT);
}


/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/

void uhci_pollDisk(void* dev)
{
    uhci_t* u = dev;
    for(uint8_t port = 0; port < u->rootPorts; port++)
    {
        uint16_t val = inportw(u->bar + UHCI_PORTSC1 + 2*port);

        if(val & UHCI_PORT_CS_CHANGE)
        {
            printf("\nUHCI %u: Port %u changed: ", u->num, port);
            outportw(u->bar + UHCI_PORTSC1 + 2*port, UHCI_PORT_CS_CHANGE);

            if (val & UHCI_PORT_LOWSPEED_DEVICE)
                printf("Lowspeed device");
            else
                printf("Fullspeed device");
            if (val & UHCI_PORT_CS)
                printf(" attached.");
            else
                printf(" removed.");
        }
    }
}

static void uhci_showPortState(uhci_t* u, uint8_t port)
{
    uint16_t val = inportw(u->bar + UHCI_PORTSC1 + 2*port);

    printf("\nport %u: %xh", port+1, val);

    if (val & UHCI_SUSPEND)              {printf(", SUSPEND"           );}
    if (val & UHCI_PORT_RESET)           {printf(", RESET"             );}
    if (val & UHCI_PORT_LOWSPEED_DEVICE) {printf(", LOWSPEED DEVICE"   );}
    else                                 {printf(", FULLSPEED DEVICE"  );}
    if (val & UHCI_PORT_RESUME_DETECT)   {printf(", RESUME DETECT"     );}

    if (val & BIT(5))                    {printf(", Line State: D-"    );}
    if (val & BIT(4))                    {printf(", Line State: D+"    );}

    if (val & UHCI_PORT_ENABLE_CHANGE)   {printf(", ENABLE CHANGE"     );}
    if (val & UHCI_PORT_ENABLE)          {printf(", ENABLED"           );}
    if (val & UHCI_PORT_CS_CHANGE)       {printf(", DEVICE CHANGE"     );}
    if (val & UHCI_PORT_CS)              {printf(", DEVICE ATTACHED"   );}
    else                                 {printf(", NO DEVICE ATTACHED");}
}

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
