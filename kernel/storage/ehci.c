/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "ehci.h"
#include "ehciQHqTD.h"
#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "irq.h"
#include "audio/sys_speaker.h"
#include "keyboard.h"
#include "usb2_msd.h"

#define NUMBER_OF_EHCI_ASYNCLIST_RETRIES 3

ehci_t* curEHCI = 0;

static uint8_t numPorts = 0;
static ehci_t* ehci[EHCIMAX];


static void ehci_handler(registers_t* r, pciDev_t* device);
static void ehci_deactivateLegacySupport(ehci_t* e);
static void ehci_analyze(ehci_t* e);
static void ehci_checkPortLineStatus(ehci_t* e, uint8_t j);
static void ehci_detectDevice(ehci_t* e, uint8_t j);


void ehci_install(pciDev_t* PCIdev, uintptr_t bar_phys)
{
    ehci_t* e = curEHCI = ehci[numPorts] = malloc(sizeof(ehci_t), 0, "ehci");
    e->num              = numPorts;
    e->PCIdevice        = PCIdev;
    e->PCIdevice->data  = e;
    e->bar              = (uintptr_t)paging_acquirePciMemory(bar_phys,1) + (bar_phys % PAGESIZE);
    e->enabledPortFlag  = false;

  #ifdef _EHCI_DIAGNOSIS_
    printf("\nEHCI_MMIO %Xh mapped to virt addr %Xh", bar_phys, e->bar);
  #endif

    e->CapRegs   = (ehci_CapRegs_t*) e->bar;
    e->OpRegs    = (ehci_OpRegs_t*)(e->bar + e->CapRegs->CAPLENGTH);
    e->numPorts  = (e->CapRegs->HCSPARAMS & 0x000F);

    char str[10];
    snprintf(str, 10, "EHCI %u", numPorts+1);

    ehci_analyze(e); // get data (capregs, opregs)

    scheduler_insertTask(create_cthread(&ehci_start, str));

    numPorts++;
    sleepMilliSeconds(20); // HACK: Avoid race condition between ehci_install and the thread just created. Problem related to curEHCI global variable
}

static void ehci_analyze(ehci_t* e)
{
  #ifdef _EHCI_DIAGNOSIS_
    printf("\nEHCI bar get_physAddress: %Xh\n", (uintptr_t)paging_getPhysAddr((void*)e->bar));
    printf("HCIVERSION: %xh ",  e->CapRegs->HCIVERSION);             // Interface Version Number
    printf("HCSPARAMS: %Xh ",   e->CapRegs->HCSPARAMS);              // Structural Parameters
    printf("Ports: %u",         e->numPorts);                        // Number of Ports
    printf("\nHCCPARAMS: %Xh ", e->CapRegs->HCCPARAMS);              // Capability Parameters
    if (BYTE2(e->CapRegs->HCCPARAMS)==0) printf("No ext. capabil."); // Extended Capabilities Pointer
    printf("\nOpRegs Address: %Xh", e->OpRegs);                      // Host Controller Operational Registers
  #endif
}

void ehci_start()
{
    ehci_t* e = curEHCI;

    textColor(HEADLINE);
    printf("Start EHCI Host Controller:");
    textColor(TEXT);

    ehci_initHC(e);
    ehci_resetHC(e);
    ehci_startHC(e);

    if (!(e->OpRegs->USBSTS & STS_HCHALTED))
    {
        ehci_enablePorts(e);
    }
    else
    {
        textColor(ERROR);
        printf("\nFatal Error: HCHalted set. Ports cannot be enabled.");
        ehci_showUSBSTS(e);
    }

    textColor(LIGHT_MAGENTA);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

void ehci_initHC(ehci_t* e)
{
    printf(" Initialize");

    // pci bus data
    uint8_t bus  = e->PCIdevice->bus;
    uint8_t dev  = e->PCIdevice->device;
    uint8_t func = e->PCIdevice->func;

    // prepare PCI command register
    // bit 9: Fast Back-to-Back Enable // not necessary
    // bit 2: Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, PCI_COMMAND, 2);
    pci_config_write_dword(bus, dev, func, PCI_COMMAND, pciCommandRegister | PCI_CMD_MMIO | PCI_CMD_BUSMASTER); // resets status register, sets command register

  #ifdef _EHCI_DIAGNOSIS_
    uint8_t pciCapabilitiesList = pci_config_read(bus, dev, func, PCI_CAPLIST, 1);
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, PCI_COMMAND, 2));
    printf("\nPCI Capabilities List: first Pointer: %yh", pciCapabilitiesList);

    if (pciCapabilitiesList) // pointer != 0
    {
        uint16_t nextCapability = 0;
        do
        {
            nextCapability = pci_config_read(bus, dev, func, BYTE2(nextCapability), 2);
            printf("\nPCI Capabilities List: ID: %yh, next Pointer: %yh", BYTE1(nextCapability), BYTE2(nextCapability));
        } while (BYTE2(nextCapability)); // pointer to next capability != 0
    }
  #endif

    irq_installPCIHandler(e->PCIdevice->irq, ehci_handler, e->PCIdevice);
}

void ehci_startHC(ehci_t* e)
{
    printf(" Start");

    /*
    Intel Intel® 82801EB (ICH5), 82801ER (ICH5R), and 82801DB (ICH4)
    Enhanced Host Controller Interface (EHCI) Programmer’s Reference Manual (PRM) April 2003:
    After the reset has completed, the system software must reinitialize the host controller so as to
    return the host controller to an operational state (See Section 4.3.3.3, Post-Operating System Initialization)

    ... software must complete the controller initialization by performing the following steps:
    */

    // 1. Claim/request ownership of the EHCI. This process is described in detail in Section 5 - EHCI Ownership.
    ehci_deactivateLegacySupport(e);

    // 2. Program the CTRLDSSEGMENT register. This value must be programmed since the ICH4/5 only uses 64bit addressing
    //    (See Section 4.3.3.1.2-HCCPARAMS – Host Controller Capability Parameters).
    //    This register must be programmed before the periodic and asynchronous schedules are enabled.
    e->OpRegs->CTRLDSSEGMENT = 0; // Program the CTRLDSSEGMENT register with 4-GiB-segment where all of the interface data structures are allocated.

    // 3. Determine which events should cause an interrupt. System software programs the USB2INTR register
    //    with the appropriate value. See Section 9 - Hardware Interrupt Routing - for additional details.
    e->OpRegs->USBINTR = STS_ASYNC_INT|STS_HOST_SYSTEM_ERROR|STS_PORT_CHANGE|STS_USBERRINT|STS_USBINT/*|STS_FRAMELIST_ROLLOVER*/;

    // 4. Program the USB2CMD.InterruptThresholdControl bits to set the desired interrupt threshold
    e->OpRegs->USBCMD |= CMD_8_MICROFRAME;

    //    and turn the host controller ON via setting the USB2CMD.Run/Stop bit. Setting the Run/Stop
    //    bit with both the periodic and asynchronous schedules disabled will still allow interrupts and
    //    enabled port events to be visible to software
    if (e->OpRegs->USBSTS & STS_HCHALTED)
    {
        e->OpRegs->USBCMD |= CMD_RUN_STOP; // set Run-Stop-Bit
    }

    // 5. Write a 1 to CONFIGFLAG register to default-route all ports to the EHCI. The EHCI can temporarily release control
    //    of the port to a cHC by setting the PortOwner bit in the PORTSC register to a one
    e->OpRegs->CONFIGFLAG = CF; // if zero, EHCI is not enabled and all usb devices go to the cHC

  #ifdef _EHCI_DIAGNOSIS_
    // 60 bits = 15 nibble  for the 15 possible ports of the EHCI show number of cHC
    printf("\nHCSPPORTROUTE_Hi: %X  HCSPPORTROUTE_Lo: %X", e->CapRegs->HCSPPORTROUTE_Hi, e->CapRegs->HCSPPORTROUTE_Lo);
    // There VMWare has a bug! You can write to this Read-Only register, and then it does not reset.
  #endif

    sleepMilliSeconds(100); // do not delete
}

void ehci_resetHC(ehci_t* e)
{
    printf(" Reset");
    /*
    Intel Intel® 82801EB (ICH5), 82801ER (ICH5R), and 82801DB (ICH4)
    Enhanced Host Controller Interface (EHCI) Programmer’s Reference Manual (PRM) April 2003

    To initiate a host controller reset system software must:
    */

    // 1. Stop the host controller.
    //    System software must program the USB2CMD.Run/Stop bit to 0 to stop the host controller.
    e->OpRegs->USBCMD &= ~CMD_RUN_STOP;            // set Run-Stop-Bit to 0

    // 2. Wait for the host controller to halt.
    //    To determine when the host controller has halted, system software must read the USB2STS.HCHalted bit;
    //    the host controller will set this bit to 1 as soon as
    //    it has successfully transitioned from a running state to a stopped state (halted).
    //    Attempting to reset an actively running host controller will result in undefined behavior.
    while (!(e->OpRegs->USBSTS & STS_HCHALTED))
    {
        sleepMilliSeconds(10); // wait at least 16 microframes (= 16*125 micro-sec = 2 ms)
    }

    // 3. Program the USB2CMD.HostControllerReset bit to a 1.
    //    This will cause the host controller to begin the host controller reset.
    e->OpRegs->USBCMD |= CMD_HCRESET;              // set Reset-Bit to 1

    // 4. Wait until the host controller has completed its reset.
    // To determine when the reset is complete, system software must read the USB2CMD.HostControllerReset bit;
    // the host controller will set this bit to 0 upon completion of the reset.
    uint32_t timeout=20;
    while ((e->OpRegs->USBCMD & CMD_HCRESET) != 0) // Reset-Bit still set to 1
    {
      #ifdef _EHCI_DIAGNOSIS_
        printf("waiting for HC reset\n");
      #endif
        sleepMilliSeconds(10);
        timeout--;
        if (timeout==0)
        {
            textColor(ERROR);
            printf("Timeout Error: HC Reset-Bit still set to 1\n");
            textColor(TEXT);
            break;
        }
    }
}

static void ehci_deactivateLegacySupport(ehci_t* e)
{
    // pci bus data
    uint8_t bus  = e->PCIdevice->bus;
    uint8_t dev  = e->PCIdevice->device;
    uint8_t func = e->PCIdevice->func;

    uint8_t eecp = BYTE2(e->CapRegs->HCCPARAMS);

  #ifdef _EHCI_DIAGNOSIS_
    printf("\nDeactivateLegacySupport: eecp = %yh\n",eecp);
  #endif
    /*
    cf. EHCI 1.0 spec, 2.2.4 HCCPARAMS - Capability Parameters, Bit 15:8 (BYTE2)
    EHCI Extended Capabilities Pointer (EECP). Default = Implementation Dependent.
    This optional field indicates the existence of a capabilities list.
    A value of 00h indicates no extended capabilities are implemented.
    A non-zero value in this register indicates the offset in PCI configuration space
    of the first EHCI extended capability. The pointer value must be 40h or greater
    if implemented to maintain the consistency of the PCI header defined for this class of device.
    */
    // cf. http://wiki.osdev.org/PCI#PCI_Device_Structure

    //   eecp     // RO - This field identifies the extended capability.
                  //      01h identifies the capability as Legacy Support.
    if (eecp >= 0x40)
    {
        uint8_t eecp_id=0;

        while (eecp) // 00h indicates end of the ext. cap. list.
        {
          #ifdef _EHCI_DIAGNOSIS_
            printf("eecp = %yh, ",eecp);
          #endif
            eecp_id = pci_config_read(bus, dev, func, eecp, 1);
          #ifdef _EHCI_DIAGNOSIS_
            printf("eecp_id = %xh\n",eecp_id);
          #endif
            if (eecp_id == 1)
            {
                break;
            }
            eecp = pci_config_read(bus, dev, func, eecp + 1, 1);
        }
        uint8_t BIOSownedSemaphore = eecp + 2; // R/W - only Bit 16 (Bit 23:17 Reserved, must be set to zero)
        uint8_t OSownedSemaphore   = eecp + 3; // R/W - only Bit 24 (Bit 31:25 Reserved, must be set to zero)
        uint8_t USBLEGCTLSTS       = eecp + 4; // USB Legacy Support Control/Status (DWORD, cf. EHCI 1.0 spec, 2.1.8)

        // Legacy-Support-EC found? BIOS-Semaphore set?
        if (eecp_id == 1 && (pci_config_read(bus, dev, func, BIOSownedSemaphore, 1) & 0x01))
        {
          #ifdef _EHCI_DIAGNOSIS_
            printf("set OS-Semaphore.\n");
          #endif
            pci_config_write_byte(bus, dev, func, OSownedSemaphore, 0x01);

            int32_t timeout=250;
            // Wait for BIOS-Semaphore being not set
            while ((pci_config_read(bus, dev, func, BIOSownedSemaphore, 1) & 0x01) && (timeout>0))
            {
              #ifdef _EHCI_DIAGNOSIS_
                putch('.');
              #endif
                timeout--;
                sleepMilliSeconds(10);
            }
            if (!(pci_config_read(bus, dev, func, BIOSownedSemaphore, 1) & 0x01)) // not set
            {
              #ifdef _EHCI_DIAGNOSIS_
                printf("BIOS-Semaphore being not set.\n");
              #endif
                timeout=250;
                while (!(pci_config_read(bus, dev, func, OSownedSemaphore, 1) & 0x01) && (timeout>0))
                {
                    putch('.');
                    timeout--;
                    sleepMilliSeconds(10);
                }
            }
          #ifdef _EHCI_DIAGNOSIS_
            if (pci_config_read(bus, dev, func, OSownedSemaphore, 1) & 0x01)
            {
                printf("OS-Semaphore being set.\n");
            }
            printf("Check: BIOSownedSemaphore: %u OSownedSemaphore: %u\n",
                pci_config_read(bus, dev, func, BIOSownedSemaphore, 1),
                pci_config_read(bus, dev, func, OSownedSemaphore, 1));
          #endif

            // USB SMI Enable R/W. 0=Default.
            // The OS tries to set SMI to disabled in case that BIOS bit stays at one.
            pci_config_write_dword(bus, dev, func, USBLEGCTLSTS, 0x0); // USB SMI disabled
        }
      #ifdef _EHCI_DIAGNOSIS_
        else
        {
            textColor(SUCCESS);
            printf("\nBIOS did not own the EHCI. No action needed.\n");
            textColor(TEXT);
        }
    }
    else
    {
        printf("No valid eecp found.\n");
  #endif
    }
}

void ehci_enablePorts(ehci_t* e)
{
    e->ports = malloc(sizeof(ehci_port_t)*e->numPorts, 0, "ehci_port_t");
    for (uint8_t j=0; j<e->numPorts; j++)
    {
        e->ports[j].ehci = e;
        e->ports[j].port.type = &USB_EHCI; // device manager
        e->ports[j].port.data = e->ports + j;
        e->ports[j].port.insertedDisk = 0;
        snprintf(e->ports[j].port.name, 14, "EHCI-Port %u", j+1);
        attachPort(&e->ports[j].port);
        ehci_initializeAsyncScheduler(e);
    }
    e->enabledPortFlag = true;
    for (uint8_t j=0; j<e->numPorts; j++)
    {
        ehci_checkPortLineStatus(e, j);
    }
}

void ehci_resetPort(ehci_t* e, uint8_t j)
{
  #ifdef _EHCI_DIAGNOSIS_
    printf("Reset port %u\n", j+1);
  #endif

    e->OpRegs->PORTSC[j] |= PSTS_POWERON;

    /*
     http://www.intel.com/technology/usb/download/ehci-r10.pdf
     When software writes a one to this bit (from a zero),
     the bus reset sequence as defined in the USB Specification Revision 2.0 is started.
     Software writes a zero to this bit to terminate the bus reset sequence.
     Software must keep this bit at a one long enough to ensure the reset sequence,
     as specified in the USB Specification Revision 2.0, completes.
     Note: when software writes this bit to a one,
     it must also write a zero to the Port Enable bit.
    */
    e->OpRegs->PORTSC[j] &= ~PSTS_ENABLED;

    /*
     The HCHalted bit in the USBSTS register should be a zero
     before software attempts to use this bit.
     The host controller may hold Port Reset asserted to a one
     when the HCHalted bit is a one.
    */
    if (e->OpRegs->USBSTS & STS_HCHALTED) // TEST
    {
        textColor(ERROR);
        printf("\nHCHalted set to 1 (Not OK!)");
        ehci_showUSBSTS(e);
        textColor(TEXT);
    }

    e->OpRegs->USBINTR = 0;
    e->OpRegs->PORTSC[j] |=  PSTS_PORT_RESET; // start reset sequence
    sleepMilliSeconds(100);                   // do not delete this wait (freeBSD: 250 ms, spec: 50 ms)
    e->OpRegs->PORTSC[j] &= ~PSTS_PORT_RESET; // stop reset sequence

    // wait and check, whether really zero
    uint32_t timeout=20;
    while ((e->OpRegs->PORTSC[j] & PSTS_PORT_RESET) != 0)
    {
        sleepMilliSeconds(10);
        timeout--;
        if (timeout == 0)
        {
            textColor(ERROR);
            printf("\nTimeour Error: Port %u did not reset! ", j+1);
            textColor(TEXT);
            printf("Port Status: %Xh",e->OpRegs->PORTSC[j]);
            break;
        }
    }
    e->OpRegs->USBINTR = STS_ASYNC_INT|STS_HOST_SYSTEM_ERROR|STS_PORT_CHANGE|STS_USBERRINT|STS_USBINT;
}



/*******************************************************************************************************
*                                                                                                      *
*                                              ehci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

static void ehci_handler(registers_t* r, pciDev_t* device)
{
    // Check if an EHCI controller issued this interrupt
    ehci_t* e = device->data;
    bool found = false;
    for (uint8_t i=0; i<EHCIMAX; i++)
    {
        if (e == ehci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    if(!found || e == 0) // Interrupt did not came from EHCI device
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("Interrupt did not came from EHCI device!\n");
      #endif
        return;
    }

    volatile uint32_t val = e->OpRegs->USBSTS;

    if(val==0) // Interrupt came from another EHCI device
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("Interrupt came from another EHCI device!\n");
      #endif
        return;
    }

    e->OpRegs->USBSTS = val; // reset interrupt


  #ifdef _EHCI_DIAGNOSIS_
    if (!(val & STS_FRAMELIST_ROLLOVER) && !(e->OpRegs->USBSTS & STS_USBINT))
    {
        textColor(LIGHT_BLUE);
        printf("\nehci_handler: ");
    }
  #endif

    if (val & STS_USBINT)
    {
        e->USBINTflag = true; // is asked by polling
    }

    if (val & STS_USBERRINT)
    {
        textColor(ERROR);
        printf("USB Error Interrupt");
        textColor(TEXT);
    }

    if (val & STS_PORT_CHANGE)
    {
        textColor(LIGHT_BLUE);
        printf("Port Change");
        textColor(TEXT);

        if (e->enabledPortFlag && e->PCIdevice)
        {
            scheduler_insertTask(create_cthread(&ehci_portCheck, "EHCI Ports"));
        }
    }

    if (val & STS_FRAMELIST_ROLLOVER)
    {
        //printf("Frame List Rollover Interrupt");
    }

    if (val & STS_HOST_SYSTEM_ERROR)
    {
        textColor(ERROR);
        printf("Host System Error");
        pci_analyzeHostSystemError(e->PCIdevice);
        textColor(IMPORTANT);
        printf("\n>>> Init EHCI after fatal error:           <<<");
        printf("\n>>> Press key for EHCI (re)initialization. <<<");
        getch();
        scheduler_insertTask(create_cthread(&ehci_start, "EHCI"));
        textColor(TEXT);
    }

    if (val & STS_ASYNC_INT)
    {
        e->USBasyncIntFlag = true;
        e->OpRegs->USBCMD |= CMD_ASYNCH_INT_DOORBELL; // Activate Doorbell: We would like to receive an asynchronous schedule interrupt
      #ifdef _EHCI_DIAGNOSIS_
        textColor(YELLOW);
        printf("Interrupt on Async Advance");
        textColor(TEXT);
      #endif
    }
}



/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/

void ehci_portCheck()
{
    ehci_t* e = curEHCI;

    console_setProperties(CONSOLE_SHOWINFOBAR|CONSOLE_AUTOSCROLL|CONSOLE_AUTOREFRESH); // protect console against info area
    for (uint8_t j=0; j<e->numPorts; j++)
    {
        if (e->OpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE)
        {
            if (e->OpRegs->PORTSC[j] & PSTS_CONNECTED)
            {
                ehci_checkPortLineStatus(e,j);
                beep(800, 100);
                beep(1000, 100);
            }
            else
            {
                writeInfo(0, "Port: %u, no device attached", j+1);
                e->OpRegs->PORTSC[j] &= ~PSTS_COMPANION_HC_OWNED; // port is given back to the EHCI

                if(e->ports[j].port.insertedDisk && e->ports[j].port.insertedDisk->type == &USB_MSD)
                {
                    usb2_destroyDevice(e->ports[j].port.insertedDisk->data);
                    removeDisk(e->ports[j].port.insertedDisk);
                    e->ports[j].port.insertedDisk = 0;

                    showPortList();
                    showDiskList();
                    beep(1000, 100);
                    beep(800, 100);
                }

            }
            e->OpRegs->PORTSC[j] |= PSTS_CONNECTED_CHANGE; // reset interrupt
        }
    }
    textColor(IMPORTANT);
    printf("\n>>> Press key to close this console. <<<");
    textColor(TEXT);
    getch();
}

static void ehci_checkPortLineStatus(ehci_t* e, uint8_t j)
{
  #ifdef _EHCI_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    static const char* const state[] = {"SE0", "K-state", "J-state", "undefined"};
    printf("\nport %u: %xh, line: %yh (%s) ",j+1,e->OpRegs->PORTSC[j],(e->OpRegs->PORTSC[j]>>10)&3, state[(e->OpRegs->PORTSC[j]>>10)&3]);
  #else
    static const char* const state[] = {"SE0", "K-state", "J-state", "undefined"};
    printf("\nline state: %s", state[(e->OpRegs->PORTSC[j]>>10)&3]);
  #endif

    switch ((e->OpRegs->PORTSC[j]>>10)&3) // bits 11:10
    {
        case 1: // K-state, release ownership of port, because a low speed device is attached
            e->OpRegs->PORTSC[j] |= PSTS_COMPANION_HC_OWNED; // release it to the cHC
            break;
        case 0: // SE0
        case 2: // J-state
        case 3: // undefined
            ehci_detectDevice(e, j);
            break;
    } // switch
}

static void ehci_detectDevice(ehci_t* e, uint8_t j)
{
    ehci_resetPort(e,j);
    if (e->enabledPortFlag && (e->OpRegs->PORTSC[j] & PSTS_POWERON) && (e->OpRegs->PORTSC[j] & PSTS_CONNECTED)) // enabled, device attached
    {
        if(e->OpRegs->PORTSC[j] & PSTS_ENABLED) // High speed
        {
            writeInfo(0, "Port: %u, hi-speed device attached", j+1);
            ehci_setupUSBDevice(e, j);
        }
        else // Full speed
        {
            writeInfo(0, "Port: %u, full-speed device attached", j+1);
            e->OpRegs->PORTSC[j] |= PSTS_COMPANION_HC_OWNED; // release it to the cHC
        }
    }
}


/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

void ehci_setupUSBDevice(ehci_t* e, uint8_t portNumber)
{
    disk_t* disk = malloc(sizeof(disk_t), 0, "disk_t"); // TODO: Handle non-MSDs
    disk->port = &e->ports[portNumber].port;
    disk->port->insertedDisk = disk;

    usb2_Device_t* device = usb2_createDevice(disk);
    usb_setupDevice(device, portNumber+1);
}

void ehci_showUSBSTS(ehci_t* e)
{
  #ifdef _EHCI_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB status: ");
    textColor(IMPORTANT);
    printf("%Xh", e->OpRegs->USBSTS);
  #endif
    textColor(ERROR);
    if (e->OpRegs->USBSTS & STS_HCHALTED)         { printf("\nHCHalted");                     e->OpRegs->USBSTS |= STS_HCHALTED;         }
    textColor(IMPORTANT);
    if (e->OpRegs->USBSTS & STS_RECLAMATION)      { printf("\nReclamation");                  e->OpRegs->USBSTS |= STS_RECLAMATION;      }
    if (e->OpRegs->USBSTS & STS_PERIODIC_ENABLED) { printf("\nPeriodic Schedule Status");     e->OpRegs->USBSTS |= STS_PERIODIC_ENABLED; }
    if (e->OpRegs->USBSTS & STS_ASYNC_ENABLED)    { printf("\nAsynchronous Schedule Status"); e->OpRegs->USBSTS |= STS_ASYNC_ENABLED;    }
    textColor(TEXT);
}



/*******************************************************************************************************
*                                                                                                      *
*                                            Transactions                                              *
*                                                                                                      *
*******************************************************************************************************/


void ehci_setupTransfer(usb_transfer_t* transfer)
{
    transfer->data = malloc(sizeof(ehci_qhd_t), 32, "EHCI-QH");
}

void ehci_setupTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length)
{
    ehci_transaction_t* eTransaction = uTransaction->data = malloc(sizeof(ehci_transaction_t), 0, "ehci_transaction_t");
    eTransaction->inBuffer = 0;
    eTransaction->inLength = 0;
    eTransaction->qTD = ehci_createQTD_SETUP(1, toggle, tokenBytes, type, req, hiVal, loVal, index, length, &eTransaction->qTDBuffer);
    if(transfer->transactions->tail)
    {
        ehci_transaction_t* eLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        eLastTransaction->qTD->next = paging_getPhysAddr(eTransaction->qTD);
    }
}

void ehci_inTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, void* buffer, size_t length)
{
    ehci_transaction_t* eTransaction = uTransaction->data = malloc(sizeof(ehci_transaction_t), 0, "ehci_transaction_t");
    eTransaction->inBuffer = buffer;
    eTransaction->inLength = length;
    eTransaction->qTD = ehci_createQTD_IO(1, 1, toggle, length, &eTransaction->qTDBuffer);
    if(transfer->transactions->tail)
    {
        ehci_transaction_t* eLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        eLastTransaction->qTD->next = paging_getPhysAddr(eTransaction->qTD);
    }
}

void ehci_outTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, void* buffer, size_t length)
{
    ehci_transaction_t* eTransaction = uTransaction->data = malloc(sizeof(ehci_transaction_t), 0, "ehci_transaction_t");
    eTransaction->inBuffer = 0;
    eTransaction->inLength = 0;
    eTransaction->qTD = ehci_createQTD_IO(1, 0, toggle, length, &eTransaction->qTDBuffer);
    if(buffer != 0 && length != 0)
        memcpy(eTransaction->qTDBuffer, buffer, length);
    if(transfer->transactions->tail)
    {
        ehci_transaction_t* eLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        eLastTransaction->qTD->next = paging_getPhysAddr(eTransaction->qTD);
    }
}

void ehci_issueTransfer(usb_transfer_t* transfer)
{
    ehci_t* e = ((ehci_port_t*)transfer->HC->data)->ehci;

    ehci_transaction_t* firstTransaction = ((usb_transaction_t*)transfer->transactions->head->data)->data;
    ehci_createQH(transfer->data, paging_getPhysAddr(transfer->data), firstTransaction->qTD, 0, ((usb2_Device_t*)transfer->HC->insertedDisk->data)->num, transfer->endpoint, transfer->packetSize);

    for(uint8_t i = 0; i < NUMBER_OF_EHCI_ASYNCLIST_RETRIES && !transfer->success; i++)
    {
        if(transfer->type == USB_CONTROL)
        {
            ehci_addToAsyncScheduler(e, transfer, 0);
        }
        else
        {
            ehci_addToAsyncScheduler(e, transfer, 1 + transfer->packetSize/200);
        }

        transfer->success = true;
        for(dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
        {
            ehci_transaction_t* transaction = ((usb_transaction_t*)elem->data)->data;
            uint8_t status = ehci_showStatusbyteQTD(transaction->qTD);
            transfer->success = transfer->success && (status == 0 || status == BIT(0));
        }

      #ifdef _EHCI_DIAGNOSIS_
        if(!transfer->success)
        {
            printf("\nRetry transfer: %u", i+1);
        }
      #endif
    }

    free(transfer->data);
    for(dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
    {
        ehci_transaction_t* transaction = ((usb_transaction_t*)elem->data)->data;

        if(transaction->inBuffer != 0 && transaction->inLength != 0)
            memcpy(transaction->inBuffer, transaction->qTDBuffer, transaction->inLength);
        free(transaction->qTDBuffer);
        free(transaction->qTD);
        free(transaction);
    }
    if(transfer->success)
    {
      #ifdef _EHCI_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nTransfer successful.");
        textColor(TEXT);
      #endif
    }
    else
    {
        textColor(ERROR);
        printf("\nTransfer failed.");
        textColor(TEXT);
    }
}


/*
* Copyright (c) 2009-2011 The PrettyOS Project. All rights reserved.
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
