/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "irq.h"
#include "audio/sys_speaker.h"
#include "keyboard.h"
#include "usb2.h"
#include "usb2_msd.h"


static struct ehci_CapRegs* pCapRegs; // = &CapRegs;
struct ehci_OpRegs*  OpRegs;  // = &OpRegs;

bool EHCIflag = false; // signals that one EHCI device was found /// TODO: manage more than one EHCI
bool USBINTflag;       // signals STS_USBINT reset by EHCI handler

static uint8_t numPorts; // maximum

// Device Manager
static disk_t usbDev[16];
static port_t port[16];

static bool USBtransferFlag; // switch on/off tests for USB-Transfer
static bool enabledPortFlag; // port enabled

static pciDev_t* PCIdevice = 0; // pci device

// usb devices list
extern usb2_Device_t usbDevices[16]; // ports 1-16
extern uintptr_t    SetupQTDpage0;


static void ehci_handler(registers_t* r, pciDev_t* device);
static void ehci_deactivateLegacySupport(pciDev_t* PCIdev);
static void ehci_analyze(uintptr_t bar, uintptr_t offset);
static void ehci_checkPortLineStatus(uint8_t j);


void ehci_install(pciDev_t* PCIdev, uintptr_t bar_phys)
{
  #ifdef _EHCI_DIAGNOSIS_
    printf("\n>>>ehci_install<<<\n");
  #endif

    uintptr_t bar    = (uintptr_t)paging_acquirePciMemory(bar_phys,1);
    uintptr_t offset = bar_phys % PAGESIZE;

  #ifdef _EHCI_DIAGNOSIS_
    printf("\nEHCI_MMIO %Xh mapped to virt addr %Xh, offset: %xh", bar_phys, bar, offset);
  #endif

    if (!EHCIflag) // only the first EHCI is used
    {
        PCIdevice = PCIdev; /// TODO: implement for more than one EHCI
        EHCIflag = true; // only the first EHCI is used

        scheduler_insertTask(create_cthread(&ehci_start, "EHCI"));

        ehci_analyze(bar,offset); // get data (capregs, opregs)
    }
}

static void ehci_analyze(uintptr_t bar, uintptr_t offset)
{
    bar += offset;
    pCapRegs = (struct ehci_CapRegs*)bar;
    OpRegs  = (struct ehci_OpRegs*)(bar + pCapRegs->CAPLENGTH);
    numPorts = (pCapRegs->HCSPARAMS & 0x000F);

  #ifdef _EHCI_DIAGNOSIS_
    uintptr_t bar_phys  = (uintptr_t)paging_getPhysAddr((void*)bar);
    printf("\nEHCI bar get_physAddress: %Xh\n", bar_phys);
    printf("HCIVERSION: %xh ",  pCapRegs->HCIVERSION);             // Interface Version Number
    printf("HCSPARAMS: %Xh ",   pCapRegs->HCSPARAMS);              // Structural Parameters
    printf("Ports: %u",         numPorts);                         // Number of Ports
    printf("\nHCCPARAMS: %Xh ", pCapRegs->HCCPARAMS);              // Capability Parameters
    if (BYTE2(pCapRegs->HCCPARAMS)==0) printf("No ext. capabil."); // Extended Capabilities Pointer
    printf("\nOpRegs Address: %Xh", OpRegs);                      // Host Controller Operational Registers
  #endif
}

void ehci_start()
{
    textColor(HEADLINE);
    printf("Start EHCI Host Controller:");
    textColor(TEXT);

    ehci_initHC();
    ehci_resetHC();
    ehci_startHC(PCIdevice);

    if (!(OpRegs->USBSTS & STS_HCHALTED))
    {
        ehci_enablePorts();
    }
    else
    {
        textColor(ERROR);
        printf("\nFatal Error: HCHalted set. Ports cannot be enabled.");
        showUSBSTS();
    }

    textColor(LIGHT_MAGENTA);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

void ehci_initHC()
{
    printf(" Initialize");

    // pci bus data
    uint8_t bus  = PCIdevice->bus;
    uint8_t dev  = PCIdevice->device;
    uint8_t func = PCIdevice->func;

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

    irq_installPCIHandler(PCIdevice->irq, ehci_handler, PCIdevice);

    USBtransferFlag = true;
    enabledPortFlag = false;
}

void ehci_startHC(pciDev_t* PCIdev)
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
    ehci_deactivateLegacySupport(PCIdev);

    // 2. Program the CTRLDSSEGMENT register. This value must be programmed since the ICH4/5 only uses 64bit addressing
    //    (See Section 4.3.3.1.2-HCCPARAMS – Host Controller Capability Parameters).
    //    This register must be programmed before the periodic and asynchronous schedules are enabled.
    OpRegs->CTRLDSSEGMENT = 0; // Program the CTRLDSSEGMENT register with 4-GiB-segment where all of the interface data structures are allocated.

    // 3. Determine which events should cause an interrupt. System software programs the USB2INTR register
    //    with the appropriate value. See Section 9 - Hardware Interrupt Routing - for additional details.
    // OpRegs->USBINTR = STS_INTMASK; // all interrupts allowed
    OpRegs->USBINTR = STS_ASYNC_INT|STS_HOST_SYSTEM_ERROR|STS_PORT_CHANGE|STS_USBERRINT|STS_USBINT/*|STS_FRAMELIST_ROLLOVER*/;

    // 4. Program the USB2CMD.InterruptThresholdControl bits to set the desired interrupt threshold
    OpRegs->USBCMD |= CMD_8_MICROFRAME;

    //    and turn the host controller ON via setting the USB2CMD.Run/Stop bit. Setting the Run/Stop
    //    bit with both the periodic and asynchronous schedules disabled will still allow interrupts and
    //    enabled port events to be visible to software
    if (OpRegs->USBSTS & STS_HCHALTED)
    {
        OpRegs->USBCMD |= CMD_RUN_STOP; // set Run-Stop-Bit
    }

    // 5. Program the Configure Flag to a 1 to route all ports to the EHCI controller. Because setting
    //    this flag causes all ports to be unconditionally routed to the EHCI, all USB1.1 devices will
    //    cease to function until the bus is properly enumerated (i.e., each port is properly routed to its
    //    associated controller type: UHCI or EHCI)
    OpRegs->CONFIGFLAG = CF; // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller

    sleepMilliSeconds(100); // do not delete
}

void ehci_resetHC()
{
    printf(" Reset");
    /*
    Intel Intel® 82801EB (ICH5), 82801ER (ICH5R), and 82801DB (ICH4)
    Enhanced Host Controller Interface (EHCI) Programmer’s Reference Manual (PRM) April 2003

    To initiate a host controller reset system software must:
    */

    // 1. Stop the host controller.
    //    System software must program the USB2CMD.Run/Stop bit to 0 to stop the host controller.
    OpRegs->USBCMD &= ~CMD_RUN_STOP;            // set Run-Stop-Bit to 0

    // 2. Wait for the host controller to halt.
    //    To determine when the host controller has halted, system software must read the USB2STS.HCHalted bit;
    //    the host controller will set this bit to 1 as soon as
    //    it has successfully transitioned from a running state to a stopped state (halted).
    //    Attempting to reset an actively running host controller will result in undefined behavior.
    while (!(OpRegs->USBSTS & STS_HCHALTED))
    {
        sleepMilliSeconds(10); // wait at least 16 microframes (= 16*125 micro-sec = 2 ms)
    }

    // 3. Program the USB2CMD.HostControllerReset bit to a 1.
    //    This will cause the host controller to begin the host controller reset.
    OpRegs->USBCMD |= CMD_HCRESET;              // set Reset-Bit to 1

    // 4. Wait until the host controller has completed its reset.
    // To determine when the reset is complete, system software must read the USB2CMD.HostControllerReset bit;
    // the host controller will set this bit to 0 upon completion of the reset.
    int32_t timeout=10;
    while ((OpRegs->USBCMD & CMD_HCRESET) != 0) // Reset-Bit still set to 1
    {
      #ifdef _EHCI_DIAGNOSIS_
        printf("waiting for HC reset\n");
      #endif
        sleepMilliSeconds(20);
        timeout--;
        if (timeout<=0)
        {
            textColor(ERROR);
            printf("Timeout Error: HC Reset-Bit still set to 1\n");
            textColor(TEXT);
            break;
        }
    }
}

static void ehci_deactivateLegacySupport(pciDev_t* PCIdev)
{
    // pci bus data
    uint8_t bus  = PCIdev->bus;
    uint8_t dev  = PCIdev->device;
    uint8_t func = PCIdev->func;

    uint8_t eecp = BYTE2(pCapRegs->HCCPARAMS);

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

            int32_t timeout=200;
            // Wait for BIOS-Semaphore being not set
            while ((pci_config_read(bus, dev, func, BIOSownedSemaphore, 1) & 0x01) && (timeout>0))
            {
              #ifdef _EHCI_DIAGNOSIS_
                putch('.');
              #endif
                timeout--;
                sleepMilliSeconds(20);
            }
            if (!(pci_config_read(bus, dev, func, BIOSownedSemaphore, 1) & 0x01)) // not set
            {
              #ifdef _EHCI_DIAGNOSIS_
                printf("BIOS-Semaphore being not set.\n");
              #endif
                timeout=200;
                while (!(pci_config_read(bus, dev, func, OSownedSemaphore, 1) & 0x01) && (timeout>0))
                {
                    putch('.');
                    timeout--;
                    sleepMilliSeconds(20);
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
            // The OS tries to set SMI to disabled in case that BIOS bit satys at one.
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

void ehci_enablePorts()
{
    textColor(HEADLINE);
    printf("\nEnable ports:\n");
    textColor(TEXT);

    for (uint8_t j=0; j<numPorts; j++)
    {
        ehci_resetPort(j);
        enabledPortFlag = true;

        port[j].type = &USB_EHCI; // device manager
        port[j].data = (void*)(j+1);
        snprintf(port[j].name, 14, "EHCI-Port %u", j+1);
        attachPort(&port[j]);

        if (USBtransferFlag && enabledPortFlag && OpRegs->PORTSC[j] == (PSTS_POWERON | PSTS_ENABLED | PSTS_CONNECTED)) // high speed, enabled, device attached
        {
            textColor(YELLOW);
            printf("Port %u: high speed enabled, device attached\n",j+1);
            textColor(TEXT);

            setupUSBDevice(j); // TEST
        }
    }
}

void ehci_resetPort(uint8_t j)
{
  #ifdef _EHCI_DIAGNOSIS_
    printf("Reset port %u\n", j+1);
  #endif

    OpRegs->PORTSC[j] |= PSTS_POWERON;

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
    OpRegs->PORTSC[j] &= ~PSTS_ENABLED;

    /*
     The HCHalted bit in the USBSTS register should be a zero
     before software attempts to use this bit.
     The host controller may hold Port Reset asserted to a one
     when the HCHalted bit is a one.
    */
    if (OpRegs->USBSTS & STS_HCHALTED) // TEST
    {
        textColor(ERROR);
        printf("\nHCHalted set to 1 (Not OK!)");
        showUSBSTS();
        textColor(TEXT);
    }

    OpRegs->USBINTR = 0;
    OpRegs->PORTSC[j] |=  PSTS_PORT_RESET; // start reset sequence
    sleepMilliSeconds(250);                 // do not delete this wait
    OpRegs->PORTSC[j] &= ~PSTS_PORT_RESET; // stop reset sequence

    // wait and check, whether really zero
    uint32_t timeout=20;
    while ((OpRegs->PORTSC[j] & PSTS_PORT_RESET) != 0)
    {
        sleepMilliSeconds(20);
        timeout--;
        if (timeout == 0)
        {
            textColor(ERROR);
            printf("\nTimeour Error: Port %u did not reset! ", j+1);
            textColor(TEXT);
            printf("Port Status: %Xh",OpRegs->PORTSC[j]);
            break;
        }
    }
    OpRegs->USBINTR = STS_INTMASK;
}



/*******************************************************************************************************
*                                                                                                      *
*                                              ehci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

static void ehci_handler(registers_t* r, pciDev_t* device)
{
  #ifdef _EHCI_DIAGNOSIS_
    if (!(OpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) && !(OpRegs->USBSTS & STS_USBINT))
    {
        textColor(LIGHT_BLUE);
        printf("\nehci_handler: ");
    }
  #endif

    if (OpRegs->USBSTS & STS_USBINT)
    {
        USBINTflag = true; // is asked by polling
        // printf("USB Interrupt");
        OpRegs->USBSTS |= STS_USBINT; // reset interrupt
    }

    if (OpRegs->USBSTS & STS_USBERRINT)
    {
        textColor(ERROR);
        printf("USB Error Interrupt");
        textColor(TEXT);
        OpRegs->USBSTS |= STS_USBERRINT;
    }

    if (OpRegs->USBSTS & STS_PORT_CHANGE)
    {
        textColor(LIGHT_BLUE);
        printf("Port Change");
        textColor(TEXT);

        OpRegs->USBSTS |= STS_PORT_CHANGE;

        if (enabledPortFlag && PCIdevice)
        {
            scheduler_insertTask(create_cthread(&ehci_portCheck, "EHCI Ports"));
        }
    }

    if (OpRegs->USBSTS & STS_FRAMELIST_ROLLOVER)
    {
        //printf("Frame List Rollover Interrupt");
        OpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;
    }

    if (OpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)
    {
        textColor(ERROR);
        printf("Host System Error");
        OpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;
        pci_analyzeHostSystemError(PCIdevice);
        textColor(IMPORTANT);
        printf("\n>>> Init EHCI after fatal error:           <<<");
        printf("\n>>> Press key for EHCI (re)initialization. <<<");
        getch();
        scheduler_insertTask(create_cthread(&ehci_start, "EHCI"));
        textColor(TEXT);
    }

    if (OpRegs->USBSTS & STS_ASYNC_INT)
    {
      #ifdef _EHCI_DIAGNOSIS_
        textColor(YELLOW);
        printf("Interrupt on Async Advance");
        textColor(TEXT);
      #endif
        OpRegs->USBSTS |= STS_ASYNC_INT;
    }
}



/*******************************************************************************************************
*                                                                                                      *
*                                              PORT CHANGE                                             *
*                                                                                                      *
*******************************************************************************************************/

void ehci_portCheck()
{
    console_setProperties(CONSOLE_SHOWINFOBAR|CONSOLE_AUTOSCROLL|CONSOLE_AUTOREFRESH); // protect console against info area
    for (uint8_t j=0; j<numPorts; j++)
    {
        if (OpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE)
        {
            if (OpRegs->PORTSC[j] & PSTS_CONNECTED)
            {
                writeInfo(0, "Port: %u, device attached", j+1);
                ehci_resetPort(j);
                ehci_checkPortLineStatus(j);
            }
            else
            {
                writeInfo(0, "Port: %u, device not attached", j+1);

                // Device Manager
                removeDisk(&usbDev[j]);
                port[j].insertedDisk = 0;

                showPortList();
                showDiskList();
            }
            OpRegs->PORTSC[j] |= PSTS_CONNECTED_CHANGE; // reset interrupt
            beep(1000,100);
        }
    }
    textColor(IMPORTANT);
    printf("\n>>> Press key to close this console. <<<");
    textColor(TEXT);
    getch();
}

static void ehci_checkPortLineStatus(uint8_t j)
{
    // check line status

  #ifdef _EHCI_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nport %u: %xh, line: %yh ",j+1,OpRegs->PORTSC[j],(OpRegs->PORTSC[j]>>10)&3);
  #endif
    if (((OpRegs->PORTSC[j]>>10)&3) == 0) // SE0
    {
      #ifdef _EHCI_DIAGNOSIS_
        printf("SE0");
      #endif

        if ((OpRegs->PORTSC[j] & PSTS_POWERON) && (OpRegs->PORTSC[j] & PSTS_ENABLED) && (OpRegs->PORTSC[j] & ~PSTS_COMPANION_HC_OWNED))
        {
          #ifdef _EHCI_DIAGNOSIS_
            textColor(IMPORTANT); printf(", power on, enabled, EHCI owned"); textColor(TEXT);
          #endif
            if (USBtransferFlag && enabledPortFlag && (OpRegs->PORTSC[j] & (PSTS_POWERON | PSTS_ENABLED | PSTS_CONNECTED)))
            {
                setupUSBDevice(j);
            }
        }
    }

  #ifdef _EHCI_DIAGNOSIS_
    textColor(IMPORTANT);
    switch ((OpRegs->PORTSC[j]>>10)&3)
    {
        case 1:
            printf("K-State");
            break;
        case 2:
            printf("J-state");
            break;
        default:
            textColor(RED);
            printf("undefined");
            break;
    }
    textColor(TEXT);
  #endif
}



/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

#ifdef _EHCI_DIAGNOSIS_
static void analyzeQTD()
{
    printf("\nsetup packet: ");
    showPacket(SetupQTDpage0,8);
    printf("\nSETUP: ");
    showStatusbyteQTD(SetupQTD);
    printf("\nIO:    ");
    showStatusbyteQTD(DataQTD);
    waitForKeyStroke();
}
#endif


void setupUSBDevice(uint8_t portNumber)
{
    uint8_t devAddr = usbTransferEnumerate(portNumber);

  #ifdef _EHCI_DIAGNOSIS_
    printf("\nSETUP: "); showStatusbyteQTD(SetupQTD); waitForKeyStroke();
  #endif

    usbTransferDevice(devAddr); // device address, endpoint=0

  #ifdef _EHCI_DIAGNOSIS_
    analyzeQTD();
  #endif

    usbTransferConfig(devAddr); // device address, endpoint 0

  #ifdef _EHCI_DIAGNOSIS_
    analyzeQTD();
  #endif

    usbTransferString(devAddr); // device address, endpoint 0

  #ifdef _EHCI_DIAGNOSIS_
    analyzeQTD();
  #endif

    for (uint8_t i=1; i<4; i++) // fetch 3 strings
    {
      #ifdef _EHCI_DIAGNOSIS_
        waitForKeyStroke();
      #endif

        usbTransferStringUnicode(devAddr,i);

      #ifdef _EHCI_DIAGNOSIS_
        printf("\nsetup packet: "); showPacket(SetupQTDpage0,8);
        printf("\nSETUP: ");        showStatusbyteQTD(SetupQTD);
        printf("\nIO   : ");        showStatusbyteQTD(DataQTD);
      #endif
    }

    usbTransferSetConfiguration(devAddr, 1); // set first configuration
  #ifdef _EHCI_DIAGNOSIS_
    printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    uint8_t config = usbTransferGetConfiguration(devAddr);
    printf(" %u",config); // check configuration
    printf("\nsetup packet: "); showPacket(SetupQTDpage0,8); printf("\nSETUP: "); showStatusbyteQTD(SetupQTD);
    printf("\ndata packet: ");  showPacket(DataQTDpage0, 1); printf("\nIO:    "); showStatusbyteQTD(DataQTD);
    waitForKeyStroke();
  #endif

    if (usbDevices[devAddr].InterfaceClass != 0x08)
    {
        textColor(ERROR);
        printf("\nThis is no Mass Storage Device! MSD test and addition to device manager will not be carried out.");
        textColor(TEXT);
        waitForKeyStroke();
    }
    else
    {
        // Disk
        usbDev[portNumber].type         = &USB_MSD;
        usbDev[portNumber].data         = (void*)&usbDevices[devAddr];
        usbDev[portNumber].sectorSize   = 512;
        strcpy(usbDev[portNumber].name, usbDevices[devAddr].productName);
        attachDisk(&usbDev[portNumber]);

        // Port
        port[portNumber].insertedDisk = &usbDev[portNumber];

        showPortList(); // TEST
        showDiskList(); // TEST
        waitForKeyStroke();

        // device, interface, endpoints
      #ifdef _EHCI_DIAGNOSIS_
        textColor(HEADLINE);
        printf("\n\nMSD test now with device: %u  interface: %u  endpOUT: %u  endpIN: %u\n",
                                                devAddr+1, usbDevices[devAddr].numInterfaceMSD,
                                                usbDevices[devAddr].numEndpointOutMSD,
                                                usbDevices[devAddr].numEndpointInMSD);
        textColor(TEXT);
      #endif

        testMSD(devAddr, &usbDev[devAddr]); // test with some SCSI commands
    }
}

void showUSBSTS()
{
  #ifdef _EHCI_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB status: ");
    textColor(IMPORTANT);
    printf("%Xh",OpRegs->USBSTS);
  #endif
    textColor(ERROR);
    if (OpRegs->USBSTS & STS_USBERRINT)          { printf("\nUSB Error Interrupt");           OpRegs->USBSTS |= STS_USBERRINT;           }
    if (OpRegs->USBSTS & STS_HOST_SYSTEM_ERROR)  { printf("\nHost System Error");             OpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if (OpRegs->USBSTS & STS_HCHALTED)           { printf("\nHCHalted");                      OpRegs->USBSTS |= STS_HCHALTED;            }
    textColor(IMPORTANT);
    if (OpRegs->USBSTS & STS_PORT_CHANGE)        { printf("\nPort Change Detect");            OpRegs->USBSTS |= STS_PORT_CHANGE;         }
    if (OpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) { printf("\nFrame List Rollover");           OpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if (OpRegs->USBSTS & STS_USBINT)             { printf("\nUSB Interrupt");                 OpRegs->USBSTS |= STS_USBINT;              }
    if (OpRegs->USBSTS & STS_ASYNC_INT)          { printf("\nInterrupt on Async Advance");    OpRegs->USBSTS |= STS_ASYNC_INT;           }
    if (OpRegs->USBSTS & STS_RECLAMATION)        { printf("\nReclamation");                   OpRegs->USBSTS |= STS_RECLAMATION;         }
    if (OpRegs->USBSTS & STS_PERIODIC_ENABLED)   { printf("\nPeriodic Schedule Status");      OpRegs->USBSTS |= STS_PERIODIC_ENABLED;    }
    if (OpRegs->USBSTS & STS_ASYNC_ENABLED)      { printf("\nAsynchronous Schedule Status");  OpRegs->USBSTS |= STS_ASYNC_ENABLED;       }
    textColor(TEXT);
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
