/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "event_list.h"
#include "video.h"
#include "irq.h"
#include "sys_speaker.h"

#include "usb2.h"
#include "usb2_msd.h"
#include "ehciQHqTD.h"
#include "ehci.h"
#include "devicemanager.h"

struct ehci_CapRegs* pCapRegs; // = &CapRegs;
struct ehci_OpRegs*  pOpRegs;  // = &OpRegs;

bool EHCIflag = false; // signals that one EHCI device was found /// TODO: manage more than one EHCI
bool USBINTflag;       // signals STS_USBINT reset by EHCI handler

uint8_t numPorts; // maximum
port_t  port[17]; // device manager

uintptr_t eecp;

bool USBtransferFlag; // switch on/off tests for USB-Transfer
bool enabledPortFlag; // port enabled

pciDev_t* PCIdevice = 0; // pci device

// usb devices list
extern usb2_Device_t usbDevices[17]; // ports 1-16 // 0 not used

// Device Manager
disk_t      usbDev[17];
partition_t usbDevVolume[17];


void ehci_install(pciDev_t* PCIdev, uint32_t i)
{
    uintptr_t bar_phys = PCIdev->bar[i].baseAddress & 0xFFFFFFF0;
    uintptr_t bar      = (uintptr_t) paging_acquire_pcimem(bar_phys);
    uintptr_t offset   = bar_phys%PAGESIZE;

  #ifdef _USB_DIAGNOSIS_
    printf("\nEHCI_MMIO %X mapped to virt addr %X, offset: %x\n", bar_phys, bar, offset);
  #endif

    if (!EHCIflag) // only the first EHCI is used
    {
        PCIdevice = PCIdev; /// TODO: implement for more than one EHCI
        EHCIflag = true; // only the first EHCI is used

        addEvent(&EHCI_INIT);

        analyzeEHCI(bar,offset); // get data (capregs, opregs)
    }
}

void analyzeEHCI(uintptr_t bar, uintptr_t offset)
{
    bar += offset;
    pCapRegs = (struct ehci_CapRegs*) bar;
    pOpRegs  = (struct ehci_OpRegs*) (bar + pCapRegs->CAPLENGTH);
    numPorts = (pCapRegs->HCSPARAMS & 0x000F);

  #ifdef _USB_DIAGNOSIS_
    uintptr_t bar_phys  = (uintptr_t)paging_get_phys_addr(kernel_pd, (void*)bar);
    printf("EHCI bar get_phys_Addr: %X\n", bar_phys);
    printf("HCIVERSION: %x ",  pCapRegs->HCIVERSION);               // Interface Version Number
    printf("HCSPARAMS: %X ",   pCapRegs->HCSPARAMS);                // Structural Parameters
    printf("Ports: %u ",       numPorts);                           // Number of Ports
    printf("\nHCCPARAMS: %X ", pCapRegs->HCCPARAMS);                // Capability Parameters
    if (BYTE2(pCapRegs->HCCPARAMS)==0) printf("No ext. capabil. "); // Extended Capabilities Pointer
    printf("\nOpRegs Address: %X ", pOpRegs);                       // Host Controller Operational Registers
  #endif
}

// start thread at kernel idle loop (ckernel.c)
void ehci_init()
{
    create_cthread(&startEHCI, "EHCI");
}

void startEHCI()
{
    initEHCIHostController();
    textColor(0x0D);
    printf("\n>>> Press key to close this console. <<<");
    textColor(0x0F);
    while(!keyboard_getChar());
}

int32_t initEHCIHostController()
{
    textColor(0x09);
    printf("\ninitEHCIHostController");
    textColor(0x0F);

    // pci bus data
    uint8_t bus  = PCIdevice->bus;
    uint8_t dev  = PCIdevice->device;
    uint8_t func = PCIdevice->func;
    uint8_t irq  = PCIdevice->irq;
    // prepare PCI command register // offset 0x04
    // bit 9 (0x0200): Fast Back-to-Back Enable // not necessary
    // bit 2 (0x0004): Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, 0x0204);
    pci_config_write_dword(bus, dev, func, 0x04, pciCommandRegister /*already set*/ | 1<<2 /* bus master */); // resets status register, sets command register
    uint16_t pciCapabilitiesList = pci_config_read(bus, dev, func, 0x0234);

  #ifdef _USB_DIAGNOSIS_
    printf("\nPCI Command Register before:          %x", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %x", pci_config_read(bus, dev, func, 0x0204));
    printf("\nPCI Capabilities List: first Pointer: %x", pciCapabilitiesList);
  #endif

    if (pciCapabilitiesList) // pointer != NULL
    {
        uint16_t nextCapability = pci_config_read(bus, dev, func, 0x0200 | pciCapabilitiesList);
        printf("\nPCI Capabilities List: ID: %y, next Pointer: %y",BYTE1(nextCapability),BYTE2(nextCapability));

        while (BYTE2(nextCapability)) // pointer != NULL
        {
            nextCapability = pci_config_read(bus, dev, func, 0x0200 | BYTE2(nextCapability));
            printf("\nPCI Capabilities List: ID: %y, next Pointer: %y",BYTE1(nextCapability),BYTE2(nextCapability));
        }
    }

    irq_install_handler(32 + irq,   ehci_handler);
    irq_install_handler(32 + irq-1, ehci_handler); /// work-around for VirtualBox Bug!

    USBtransferFlag = true;
    enabledPortFlag = false;

    startHostController(PCIdevice);

    if (!(pOpRegs->USBSTS & STS_HCHALTED))
    {
         enablePorts();
    }
    else
    {
         textColor(0x0C);
         printf("\nFatal Error: Ports cannot be enabled. HCHalted set.");
         showUSBSTS();
         textColor(0x0F);
         return -1;
    }
    return 0;
}

void startHostController(pciDev_t* PCIdev)
{
    textColor(0x09);
    printf("\nstartHostController (reset HC)");
    textColor(0x0F);

    resetHostController();

    /*
    Intel Intel® 82801EB (ICH5), 82801ER (ICH5R), and 82801DB (ICH4)
    Enhanced Host Controller Interface (EHCI) Programmer’s Reference Manual (PRM) April 2003:
    After the reset has completed, the system software must reinitialize the host controller so as to
    return the host controller to an operational state (See Section 4.3.3.3, Post-Operating System Initialization)

    ... software must complete the controller initialization by performing the following steps:
    */

    // 1. Claim/request ownership of the EHCI. This process is described in detail in Section 5 - EHCI Ownership.
    DeactivateLegacySupport(PCIdev);

    // 2. Program the CTRLDSSEGMENT register. This value must be programmed since the ICH4/5 only uses 64bit addressing
    //    (See Section 4.3.3.1.2-HCCPARAMS – Host Controller Capability Parameters).
    //    This register must be programmed before the periodic and asynchronous schedules are enabled.
    pOpRegs->CTRLDSSEGMENT = 0; // Program the CTRLDSSEGMENT register with 4-GiB-segment where all of the interface data structures are allocated.

    // 3. Determine which events should cause an interrupt. System software programs the USB2INTR register
    //    with the appropriate value. See Section 9 - Hardware Interrupt Routing - for additional details.
    // pOpRegs->USBINTR = STS_INTMASK; // all interrupts allowed
    pOpRegs->USBINTR = STS_ASYNC_INT|STS_HOST_SYSTEM_ERROR|STS_PORT_CHANGE|STS_USBERRINT|STS_USBINT/*|STS_FRAMELIST_ROLLOVER*/;

    // 4. Program the USB2CMD.InterruptThresholdControl bits to set the desired interrupt threshold
    pOpRegs->USBCMD |= CMD_8_MICROFRAME;

    //    and turn the host controller ON via setting the USB2CMD.Run/Stop bit. Setting the Run/Stop
    //    bit with both the periodic and asynchronous schedules disabled will still allow interrupts and
    //    enabled port events to be visible to software
    if (pOpRegs->USBSTS & STS_HCHALTED)
    {
        pOpRegs->USBCMD |= CMD_RUN_STOP; // set Run-Stop-Bit
    }

    // 5. Program the Configure Flag to a 1 to route all ports to the EHCI controller. Because setting
    //    this flag causes all ports to be unconditionally routed to the EHCI, all USB1.1 devices will
    //    cease to function until the bus is properly enumerated (i.e., each port is properly routed to its
    //    associated controller type: UHCI or EHCI)
    pOpRegs->CONFIGFLAG = CF; // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller

    sleepMilliSeconds(100); // do not delete
}

void resetHostController()
{
    /*
    Intel Intel® 82801EB (ICH5), 82801ER (ICH5R), and 82801DB (ICH4)
    Enhanced Host Controller Interface (EHCI) Programmer’s Reference Manual (PRM) April 2003

    To initiate a host controller reset
    system software must:
    */

    // 1. Stop the host controller.
    //    System software must program the USB2CMD.Run/Stop bit to 0 to stop the host controller.
    pOpRegs->USBCMD &= ~CMD_RUN_STOP;            // set Run-Stop-Bit to 0

    /*
    2. Wait for the host controller to halt.
       To determine when the host controller has halted, system software must read the USB2STS.HCHalted bit;
       the host controller will set this bit to 1 as soon as
       it has successfully transitioned from a running state to a stopped state (halted).
       Attempting to reset an actively running host controller will result in undefined behavior.
    */
    while (!(pOpRegs->USBSTS & STS_HCHALTED))
    {
        sleepMilliSeconds(30); // wait at least 16 microframes (= 16*125 micro-sec = 2 ms)
    }

    // 3. Program the USB2CMD.HostControllerReset bit to a 1.
    //    This will cause the host controller to begin the host controller reset.
    pOpRegs->USBCMD |= CMD_HCRESET;              // set Reset-Bit to 1

    // 4. Wait until the host controller has completed its reset.
    // To determine when the reset is complete, system software must read the USB2CMD.HostControllerReset bit;
    // the host controller will set this bit to 0 upon completion of the reset.

    int32_t timeout=10;
    while ((pOpRegs->USBCMD & CMD_HCRESET) != 0) // Reset-Bit still set to 1
    {
        printf("waiting for HC reset\n");
        sleepMilliSeconds(20);
        timeout--;
        if (timeout<=0)
        {
            textColor(0x0C);
            printf("Error: HC Reset-Bit still set to 1\n");
            textColor(0x0F);
            break;
        }
    }
}

void DeactivateLegacySupport(pciDev_t* PCIdev)
{
    // pci bus data
    uint8_t bus  = PCIdev->bus;
    uint8_t dev  = PCIdev->device;
    uint8_t func = PCIdev->func;

    eecp = BYTE2(pCapRegs->HCCPARAMS);
    printf("\nDeactivateLegacySupport: eecp = %x\n",eecp);
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

    //               eecp       // RO - This field identifies the extended capability.
                                //      01h identifies the capability as Legacy Support.
    if (eecp >= 0x40)
    {
        int32_t eecp_id=0;

        while (eecp)
        {
            uint32_t NextEHCIExtCapPtr; // RO  - 00h indicates end of the ext. cap. list.

            printf("eecp = %x, ",eecp);
            eecp_id = pci_config_read(bus, dev, func, 0x0100/*length 1 byte*/ | (eecp + 0));
            printf("eecp_id = %x\n",eecp_id);
            if (eecp_id == 1)
                 break;
            NextEHCIExtCapPtr = eecp + 1;
            eecp = pci_config_read(bus, dev, func, 0x0100 | NextEHCIExtCapPtr);
        }
        uint32_t BIOSownedSemaphore = eecp + 2; // R/W - only Bit 16 (Bit 23:17 Reserved, must be set to zero)
        uint32_t OSownedSemaphore   = eecp + 3; // R/W - only Bit 24 (Bit 31:25 Reserved, must be set to zero)
        uint32_t USBLEGCTLSTS       = eecp + 4; // USB Legacy Support Control/Status (DWORD, cf. EHCI 1.0 spec, 2.1.8)

        // Legacy-Support-EC found? BIOS-Semaphore set?
        if (eecp_id == 1 && (pci_config_read(bus, dev, func, 0x0100 | BIOSownedSemaphore) & 0x01))
        {
            printf("set OS-Semaphore.\n");
            pci_config_write_byte(bus, dev, func, OSownedSemaphore, 0x01);

            int32_t timeout=200;
            // Wait for BIOS-Semaphore being not set
            while ((pci_config_read(bus, dev, func, 0x0100 | BIOSownedSemaphore) & 0x01) && (timeout>0))
            {
                printf(".");
                timeout--;
                sleepMilliSeconds(20);
            }
            if (!(pci_config_read(bus, dev, func, 0x0100 | BIOSownedSemaphore) & 0x01)) // not set
            {
                printf("BIOS-Semaphore being not set.\n");
                timeout=200;
                while (!(pci_config_read(bus, dev, func, 0x0100 | OSownedSemaphore) & 0x01) && (timeout>0))
                {
                    printf(".");
                    timeout--;
                    sleepMilliSeconds(20);
                }
            }
            if (pci_config_read(bus, dev, func, 0x0100 | OSownedSemaphore) & 0x01)
            {
                printf("OS-Semaphore being set.\n");
            }

            printf("Check: BIOSownedSemaphore: %u OSownedSemaphore: %u\n",
                pci_config_read(bus, dev, func, 0x0100 | BIOSownedSemaphore),
                pci_config_read(bus, dev, func, 0x0100 | OSownedSemaphore));

            // USB SMI Enable R/W. 0=Default.
            // The OS tries to set SMI to disabled in case that BIOS bit satys at one.
            pci_config_write_dword(bus, dev, func, USBLEGCTLSTS, 0x0); // USB SMI disabled
        }
        else
        {
                textColor(0x0A);
                printf("\nBIOS did not own the EHCI. No action needed.\n");
                textColor(0x0F);
        }
    }
    else
    {
        printf("No valid eecp found.\n");
    }
}

void enablePorts()
{
    textColor(0x09);
    printf("\nenablePorts");
    textColor(0x0F);

    for (uint8_t j=0; j<numPorts; j++)
    {
         resetPort(j);
         enabledPortFlag = true;

         port[j+1].type = &USB; // device manager
         port[j+1].data = (void*)(j+1);
         char name[14],portNum[3];
         strcpy(name,"EHCI-Port ");
         itoa(j+1,portNum);
         strcat(name,portNum);
         strncpy(port[j+1].name,name,14);
         attachPort(&port[j+1]);

         if ( USBtransferFlag && enabledPortFlag && pOpRegs->PORTSC[j] == (PSTS_POWERON | PSTS_ENABLED | PSTS_CONNECTED) ) // high speed, enabled, device attached
         {
             textColor(0x0E);
             printf("Port %u: high speed enabled, device attached\n",j+1);
             textColor(0x0F);

             setupUSBDevice(j); // TEST
         }
    }
}

void resetPort(uint8_t j)
{
    textColor(0x09);
    printf("\nresetPort %u  ",j+1);
    textColor(0x0F);

    pOpRegs->PORTSC[j] |=  PSTS_POWERON;

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
    pOpRegs->PORTSC[j] &= ~PSTS_ENABLED;

    /*
     The HCHalted bit in the USBSTS register should be a zero
     before software attempts to use this bit.
     The host controller may hold Port Reset asserted to a one
     when the HCHalted bit is a one.
    */
    if (pOpRegs->USBSTS & STS_HCHALTED) // TEST
    {
         textColor(0x0C);
         printf("\nHCHalted set to 1 (Not OK!)");
         showUSBSTS();
         textColor(0x0F);
    }

    pOpRegs->USBINTR = 0;
    pOpRegs->PORTSC[j] |=  PSTS_PORT_RESET; // start reset sequence
    sleepMilliSeconds(250);                 // do not delete this wait
    pOpRegs->PORTSC[j] &= ~PSTS_PORT_RESET; // stop reset sequence

    // wait and check, whether really zero
    uint32_t timeout=20;
    while ((pOpRegs->PORTSC[j] & PSTS_PORT_RESET) != 0)
    {
        sleepMilliSeconds(20);
        timeout--;
        if (timeout <= 0)
        {
            textColor(0x0C);
            printf("\nerror: port %u did not reset! ",j+1);
            textColor(0x0F);
            printf("PortStatus: %X",pOpRegs->PORTSC[j]);
            break;
        }
    }
    pOpRegs->USBINTR = STS_INTMASK;
}



/*******************************************************************************************************
*                                                                                                      *
*                                              ehci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

void ehci_handler(registers_t* r)
{
    if (!(pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) && !(pOpRegs->USBSTS & STS_USBINT))
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(0x09);
        printf("\nehci_handler: ");
        textColor(0x0F);
      #endif
    }

    textColor(0x0E);

    if (pOpRegs->USBSTS & STS_USBINT)
    {
        USBINTflag = true; // is asked by polling
        // printf("USB Interrupt");
        pOpRegs->USBSTS |= STS_USBINT; // reset interrupt
    }

    if (pOpRegs->USBSTS & STS_USBERRINT)
    {
        printf("USB Error Interrupt");
        pOpRegs->USBSTS |= STS_USBERRINT;
    }

    if (pOpRegs->USBSTS & STS_PORT_CHANGE)
    {
        textColor(0x09);
        printf("Port Change");
        textColor(0x0F);

        pOpRegs->USBSTS |= STS_PORT_CHANGE;

        if (enabledPortFlag && PCIdevice)
        {
            addEvent(&EHCI_PORTCHECK);
        }
    }

    if (pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER)
    {
        //printf("Frame List Rollover Interrupt");
        pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;
    }

    if (pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)
    {
        textColor(0x0C);
        printf("Host System Error");
        textColor(0x0F);
        pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;
        analyzeHostSystemError(PCIdevice);
        textColor(0x0E);
        printf("\n>>> Init EHCI after fatal error:           <<<");
        printf("\n>>> Press key for EHCI (re)initialization. <<<");
        while(!keyboard_getChar());
        textColor(0x0F);
        addEvent(&EHCI_INIT);
    }

    if (pOpRegs->USBSTS & STS_ASYNC_INT)
    {
      #ifdef _USB_DIAGNOSIS_
        printf("Interrupt on Async Advance");
      #endif
        pOpRegs->USBSTS |= STS_ASYNC_INT;
    }
}



/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/

// PORT_CHANGE via ehci_handler starts thread at kernel idle loop (ckernel.c)
void ehci_portcheck()
{
    create_cthread(&portCheck, "EHCI Ports");
}

void portCheck()
{
    showInfobar(true); // protect console against info area
    showPORTSC();      // with resetPort(j) and checkPortLineStatus(j)
    textColor(0x0D);
    printf("\n>>> Press key to close this console. <<<");
    textColor(0x0F);
    while(!keyboard_getChar());
}

void showPORTSC()
{
    for (uint8_t j=0; j<numPorts; j++)
    {
        if (pOpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE)
        {
            char PortStatus[20];

            if (pOpRegs->PORTSC[j] & PSTS_CONNECTED)
            {
                strcpy(PortStatus,"attached");
                writeInfo(0, "Port: %i, device %s", j+1, PortStatus);
                resetPort(j);
                checkPortLineStatus(j);
            }
            else
            {
                strcpy(PortStatus,"not attached");
                writeInfo(0, "Port: %i, device %s", j+1, PortStatus);

                // Device Manager
                removeDisk(&usbDev[j+1]);
                port[j+1].insertedDisk = NULL;

                showPortList();
                showDiskList();
                waitForKeyStroke();
            }
            pOpRegs->PORTSC[j] |= PSTS_CONNECTED_CHANGE; // reset interrupt
            beep(1000,100);
        }
    }
}

void checkPortLineStatus(uint8_t j)
{
    textColor(0x0E);
    if (j<numPorts)
    {
        //check line status
        textColor(0x0B);
        printf("\nport %u: %x, line: %y ",j+1,pOpRegs->PORTSC[j],(pOpRegs->PORTSC[j]>>10)&3);
        if (((pOpRegs->PORTSC[j]>>10)&3) == 0) // SE0
        {
            printf("SE0 ");

            if ((pOpRegs->PORTSC[j] & PSTS_POWERON) && (pOpRegs->PORTSC[j] & PSTS_ENABLED) && (pOpRegs->PORTSC[j] & ~PSTS_COMPANION_HC_OWNED))
            {
                 textColor(0x0E); printf(", power on, enabled, EHCI owned"); textColor(0x0F);
                 if (USBtransferFlag && enabledPortFlag && (pOpRegs->PORTSC[j] & (PSTS_POWERON | PSTS_ENABLED | PSTS_CONNECTED)))
                 {
                     setupUSBDevice(j);
                 }
            }
        }
    }
    textColor(0x0E);
    switch ((pOpRegs->PORTSC[j]>>10)&3)
    {
        case 1:
            printf("K-State");
            break;
        case 2:
            printf("J-state");
            break;
        default:
            textColor(0x0C);
            printf("undefined");
            break;
    }
    textColor(0x0F);
}



/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

void setupUSBDevice(uint8_t portNumber)
{
    uint8_t devAddr = usbTransferEnumerate(portNumber);

  #ifdef _USB_DIAGNOSIS_
    printf("\nSETUP: "); showStatusbyteQTD(SetupQTD); waitForKeyStroke();
  #endif

    usbTransferDevice(devAddr); // device address, endpoint=0

  #ifdef _USB_DIAGNOSIS_
    printf("\nsetup packet: "); showPacket(SetupQTDpage0,8); printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    printf("\nIO:    "); showStatusbyteQTD(DataQTD); waitForKeyStroke();
  #endif

    usbTransferConfig(devAddr); // device address, endpoint 0

  #ifdef _USB_DIAGNOSIS_
    printf("\nsetup packet: "); showPacket(SetupQTDpage0,8); printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    printf("\nIO   : "); showStatusbyteQTD(DataQTD); waitForKeyStroke();
  #endif

    usbTransferString(devAddr); // device address, endpoint 0

  #ifdef _USB_DIAGNOSIS_
    printf("\nsetup packet: "); showPacket(SetupQTDpage0,8); printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    printf("\nIO   : "); showStatusbyteQTD(DataQTD);
  #endif

    for(uint8_t i=1; i<4; i++) // fetch 3 strings
    {
      #ifdef _USB_DIAGNOSIS_
        waitForKeyStroke();
      #endif

        usbTransferStringUnicode(devAddr,i);

      #ifdef _USB_DIAGNOSIS_
        printf("\nsetup packet: "); showPacket(SetupQTDpage0,8);
        printf("\nSETUP: ");        showStatusbyteQTD(SetupQTD);
        printf("\nIO   : ");        showStatusbyteQTD(DataQTD);
      #endif
    }

    usbTransferSetConfiguration(devAddr, 1); // set first configuration
  #ifdef _USB_DIAGNOSIS_
    printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    uint8_t config = usbTransferGetConfiguration(devAddr);
    printf(" %u",config); // check configuration

    printf("\nsetup packet: "); showPacket(SetupQTDpage0,8); printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    printf("\ndata packet: ");  showPacket(DataQTDpage0, 1); printf("\nIO:    "); showStatusbyteQTD(DataQTD);
    waitForKeyStroke();
  #endif

    if (usbDevices[devAddr].InterfaceClass != 0x08)
    {
        textColor(0x0C);
        printf("\nThis is no Mass Storage Device! MSD test and addition to device manager will not be carried out.");
        textColor(0x0F);
        waitForKeyStroke();
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        // device manager //////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////

        // Partition
        usbDevVolume[portNumber+1].buffer = malloc(512,0);
        usbDevVolume[portNumber+1].disk = &usbDev[portNumber+1];

        //HACK
        usbDevVolume[portNumber+1].serial = malloc(13, 0);
        usbDevVolume[portNumber+1].serial[12] = 0;
        strncpy(usbDevVolume[portNumber+1].serial, usbDevices[devAddr].serialNumber, 12);

        // Disk
        usbDev[portNumber+1].type         = &USB_MSD;
        usbDev[portNumber+1].partition[0] = &usbDevVolume[portNumber+1];
        usbDev[portNumber+1].data         = (void*)&usbDevices[devAddr];
        strcpy(usbDev[portNumber+1].name, usbDevices[devAddr].productName);
        attachDisk(&usbDev[portNumber+1]);

        // Port
        port[portNumber+1].insertedDisk = &usbDev[portNumber+1];

        showPortList(); // TEST
        showDiskList(); // TEST
        waitForKeyStroke();

        ////////////////////////////////////////////////////////////////////////////////////////////

        // device, interface, endpoints
        textColor(0x07);
        printf("\n\nMSD test now with device: %u  interface: %u  endpOUT: %u  endpIN: %u\n",
                                                devAddr, usbDevices[devAddr].numInterfaceMSD,
                                                usbDevices[devAddr].numEndpointOutMSD,
                                                usbDevices[devAddr].numEndpointInMSD);
        textColor(0x0F);

        testMSD(devAddr, usbDev[devAddr].partition[0]); // test with some SCSI commands
    }
}



/*******************************************************************************************************
*                                                                                                      *
*                                          Status Analysis                                             *
*                                                                                                      *
*******************************************************************************************************/

void showUSBSTS()
{
  #ifdef _USB_DIAGNOSIS_
    printf("\nUSB status: ");
    textColor(0x02);
    printf("%X",pOpRegs->USBSTS);
  #endif
    textColor(0x0E);
    if (pOpRegs->USBSTS & STS_USBINT)             { printf("\nUSB Interrupt");                 pOpRegs->USBSTS |= STS_USBINT;              }
    if (pOpRegs->USBSTS & STS_USBERRINT)          { printf("\nUSB Error Interrupt");           pOpRegs->USBSTS |= STS_USBERRINT;           }
    if (pOpRegs->USBSTS & STS_PORT_CHANGE)        { printf("\nPort Change Detect");            pOpRegs->USBSTS |= STS_PORT_CHANGE;         }
    if (pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) { printf("\nFrame List Rollover");           pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if (pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)  { printf("\nHost System Error");             pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if (pOpRegs->USBSTS & STS_ASYNC_INT)          { printf("\nInterrupt on Async Advance");    pOpRegs->USBSTS |= STS_ASYNC_INT;           }
    if (pOpRegs->USBSTS & STS_HCHALTED)           { printf("\nHCHalted");                      pOpRegs->USBSTS |= STS_HCHALTED;            }
    if (pOpRegs->USBSTS & STS_RECLAMATION)        { printf("\nReclamation");                   pOpRegs->USBSTS |= STS_RECLAMATION;         }
    if (pOpRegs->USBSTS & STS_PERIODIC_ENABLED)   { printf("\nPeriodic Schedule Status");      pOpRegs->USBSTS |= STS_PERIODIC_ENABLED;    }
    if (pOpRegs->USBSTS & STS_ASYNC_ENABLED)      { printf("\nAsynchronous Schedule Status");  pOpRegs->USBSTS |= STS_ASYNC_ENABLED;       }
    textColor(0x0F);
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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
