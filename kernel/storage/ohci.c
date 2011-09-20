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
#include "usb2.h"
#include "usb2_msd.h"

#define OHCI_USB_TRANSFER

static uint8_t index   = 0;
static ohci_t* curOHCI = 0;
static ohci_t* ohci[OHCIMAX];
static bool    OHCI_USBtransferFlag = false;
static uint8_t indexTD    = 0; // td and td-buffer
static uint8_t indexED    = 0; // ed

static void ohci_handler(registers_t* r, pciDev_t* device);
static void ohci_start();
static void showPortstatus(ohci_t* o);


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

    ohci[index]->bar += offset;
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
    pci_config_write_dword(bus, dev, func, PCI_COMMAND, pciCommandRegister | PCI_CMD_MMIO | PCI_CMD_BUSMASTER); // resets status register, sets command register
    //uint8_t pciCapabilitiesList = pci_config_read(bus, dev, func, PCI_CAPLIST, 1);

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, PCI_COMMAND, 2));
    //printf("\nPCI Capabilities List: first Pointer: %yh", pciCapabilitiesList);
 #endif
    irq_installPCIHandler(o->PCIdevice->irq, ohci_handler, o->PCIdevice);

    OHCI_USBtransferFlag = true;
    o->enabledPortFlag   = false;

    ohci_resetHC(o);
}

void ohci_resetHC(ohci_t* o)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n\n>>>ohci_resetHostController<<<\n");
  #endif

    // Revision and Number Downstream Ports (NDP)
    /*
    When checking the Revision, the HC Driver must mask the rest of the bits in the HcRevision register
    as they are used to specify which optional features that are supported by the HC.
    */
    textColor(IMPORTANT);
    printf("\nOHCI: Revision %u.%u, Number Downstream Ports: %u\n",
        BYTE1(o->OpRegs->HcRevision) >> 4,
        BYTE1(o->OpRegs->HcRevision) & 0xF,
        BYTE1(o->OpRegs->HcRhDescriptorA)); // bits 7:0 provide Number Downstream Ports (NDP)
    textColor(TEXT);

    if (!((BYTE1(o->OpRegs->HcRevision)) == 0x10 || BYTE1(o->OpRegs->HcRevision) == 0x11))
    {
        textColor(ERROR);
        printf("Revision not valid!");
        textColor(TEXT);
    }

    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;

    if (o->OpRegs->HcControl & OHCI_CTRL_IR) // SMM driver is active because the InterruptRouting bit is set
    {
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_OCR; // ownership change request

        // monitor the IR bit to determine when the ownership change has taken effect
        uint16_t i;
        for (i=0; (o->OpRegs->HcControl & OHCI_CTRL_IR) && (i < 1000); i++)
        {
            sleepMilliSeconds(1);
        }

        if (i < 1000)
        {
            // Once the IR bit is cleared, the HC driver may proceed to the setup of the HC.
            textColor(SUCCESS);
            printf("\nOHCI takes control from SMM after %u loops.", i);
        }
        else
        {
            textColor(ERROR);
            printf("\nOwnership change request did not work. SMM has still control.");

            o->OpRegs->HcControl &= ~OHCI_CTRL_IR; // we try to reset the IR bit
            sleepMilliSeconds(200);

            if (o->OpRegs->HcControl & OHCI_CTRL_IR) // SMM driver is still active
            {
                printf("\nOHCI taking control from SMM did not work."); // evil
            }
            else
            {
                textColor(SUCCESS);
                printf("\nSuccess in taking control from SMM.");
            }
        }
        textColor(TEXT);
    }
    else // InterruptRouting bit is not set
    {
        if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) != OHCI_USB_RESET)
        {
            // there is an active BIOS driver, if the InterruptRouting bit is not set
            // and the HostControllerFunctionalState (HCFS) is not USBRESET
            printf("\nThere is an active BIOS OHCI driver");

            if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) != OHCI_USB_OPERATIONAL)
            {
                // If the HostControllerFunctionalState is not USBOPERATIONAL, the OS driver should set the HCFS to USBRESUME
                printf("\nActivate RESUME");
                o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS; // clear HCFS bits
                o->OpRegs->HcControl |= OHCI_USB_RESUME; // set specific HCFS bit

                // and wait the minimum time specified in the USB Specification for assertion of resume on the USB
                sleepMilliSeconds(10);
            }
        }
        else // HCFS is USBRESET
        {
            // Neither SMM nor BIOS
            sleepMilliSeconds(10);
        }
    }

    // setup of the Host Controller
    printf("\n\nSetup of the HC\n");

    // The HC Driver should now save the contents of the HcFmInterval register ...
    uint32_t saveHcFmInterval = o->OpRegs->HcFmInterval;

    // ... and then issue a software reset
    o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
    sleepMilliSeconds(20);

    // After the software reset is complete (a maximum of 10 ms), the Host Controller Driver
    // should restore the value of the HcFmInterval register
    o->OpRegs->HcFmInterval = saveHcFmInterval;

    /*
    The HC is now in the USBSUSPEND state; it must not stay in this state more than 2 ms
    or the USBRESUME state will need to be entered for the minimum time specified
    in the USB Specification for the assertion of resume on the USB.
    */

    if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) == OHCI_USB_SUSPEND)
    {
        o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS; // clear HCFS bits
        o->OpRegs->HcControl |= OHCI_USB_RESUME; // set specific HCFS bit
        sleepMilliSeconds(10);
    }

    /////////////////////
    // initializations //
    /////////////////////

    // HCCA
    /*
    Initialize the device data HCCA block to match the current device data state;
    i.e., all virtual queues are run and constructed into physical queues on the HCCA block
    and other fields initialized accordingly.
    */
    void* hccaVirt = malloc(sizeof(ohci_HCCA_t), OHCI_HCCA_ALIGN, "ohci HCCA"); // HCCA must be minimum 256-byte aligned
    memset(hccaVirt, 0, sizeof(ohci_HCCA_t));
    o->hcca = (ohci_HCCA_t*)hccaVirt;
    // TODO: ...

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nHCCA (phys. address): %X", o->OpRegs->HcHCCA);
  #endif

    /*
    Initialize the Operational Registers to match the current device data state;
    i.e., all virtual queues are run and constructed into physical queues for HcControlHeadED and HcBulkHeadED
    */

    // Pointers to ED, TD and TD buffers are part of ohci_t

    // ED pool: 64 EDs
    for (uint8_t i=0; i<64; i++)
    {
        o->pED[i] = malloc(sizeof(ohciED_t), OHCI_DESCRIPTORS_ALIGN, "ohci_ED");
        if(i)
        {
            o->pED[i]->nextED = (uintptr_t)o->pED[i];
        }
    }
    o->OpRegs->HcControlHeadED = paging_getPhysAddr(o->pED[0]);

    // TD pool: 56 TDs and buffers
    for (uint8_t i=0; i<56; i++)
    {
        o->pTDbuff[i] = (uintptr_t) malloc(1024, OHCI_DESCRIPTORS_ALIGN, "ohci_TDbuffer");
        o->pTD[i] = malloc(sizeof(ohciTD_t), OHCI_DESCRIPTORS_ALIGN, "ohci_TD");
        o->pTD[i]->curBuffPtr = paging_getPhysAddr((void*)o->pTDbuff[i]);
    }

    

    // Set the HcHCCA to the physical address of the HCCA block
    o->OpRegs->HcHCCA = paging_getPhysAddr(hccaVirt);

    // Set HcInterruptEnable to have all interrupt enabled except Start-of-Frame detect
    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;
    o->OpRegs->HcInterruptStatus  = ~0;
    o->OpRegs->HcInterruptEnable  = OHCI_INT_SO   | // scheduling overrun
                                    OHCI_INT_WDH  | // write back done head
                                    OHCI_INT_RD   | // resume detected
                                    OHCI_INT_UE   | // unrecoverable error
                                    OHCI_INT_FNO  | // frame number overflow
                                    OHCI_INT_RHSC | // root hub status change
                                    OHCI_INT_OC   | // ownership change
                                    OHCI_INT_MIE;   // (de)activates interrupts

    // prepare transfers
    o->OpRegs->HcControl |=  (OHCI_CTRL_CLE | OHCI_CTRL_BLE); // activate control and bulk transfers
    o->OpRegs->HcControl &= ~(OHCI_CTRL_PLE | OHCI_CTRL_IE ); // de-activate periodical and isochronous transfers

    // Set HcPeriodicStart to a value that is 90% of the value in FrameInterval field of the HcFmInterval register
    // When HcFmRemaining reaches this value, periodic lists gets priority over control/bulk processing
    o->OpRegs->HcPeriodicStart = (o->OpRegs->HcFmInterval & 0x3FFF) * 90/100;

    /*
    The HCD then begins to send SOF tokens on the USB by writing to the HcControl register with
    the HostControllerFunctionalState set to USBOPERATIONAL and the appropriate enable bits set.
    The Host Controller begins sending SOF tokens within one ms
    (if the HCD needs to know when the SOFs it may unmask the StartOfFrame interrupt).
    */

    printf("\n\nHC will be activated.\n");

    o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS;      // clear HCFS bits
    o->OpRegs->HcControl |= OHCI_USB_OPERATIONAL; // set specific HCFS bit

    o->OpRegs->HcRhStatus |= OHCI_RHS_LPSC;           // SetGlobalPower: turn on power to all ports
    o->rootPorts = BYTE1(o->OpRegs->HcRhDescriptorA); // NumberDownstreamPorts

    // duration HCD has to wait before accessing a powered-on port of the Root Hub.
    // It is implementation-specific. Duration is calculated as POTPGT * 2 ms.
    sleepMilliSeconds(2 * BYTE4(o->OpRegs->HcRhDescriptorA));

    textColor(IMPORTANT);
    printf("\n\nFound %i Rootports.\n", o->rootPorts);
    textColor(TEXT);

    for (uint8_t j = 0; j < o->rootPorts; j++)
    {
        o->ports[j] = malloc(sizeof(ohci_port_t), 0, "ohci_port_t");
        o->ports[j]->num = j+1;
        o->ports[j]->ohci = o;
        o->ports[j]->port.type = &USB_OHCI; // device manager
        o->ports[j]->port.data = o->ports[j];
        o->ports[j]->port.insertedDisk = 0;
        snprintf(o->ports[j]->port.name, 14, "OHCI-Port %u", j+1);
        attachPort(&o->ports[j]->port);
        o->enabledPortFlag = true;
        o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PRS | OHCI_PORT_CCS | OHCI_PORT_PES;
    }
}


/*******************************************************************************************************
*                                                                                                      *
*                                              PORTS                                                   *
*                                                                                                      *
*******************************************************************************************************/

void showPortstatus(ohci_t* o)
{
    for (uint8_t j=0; j < o->rootPorts; j++)
    {
        if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CSC)
        {
            textColor(IMPORTANT);
            printf("\nport[%u]:", j+1);
            textColor(TEXT);

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_LSDA)
                printf(" LowSpeed");
            else
                printf(" FullSpeed");

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CCS)
            {
                textColor(SUCCESS);
                printf(" dev. attached  -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PES;
            }
            else
                printf(" device removed -");

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PES)
            {
                textColor(SUCCESS);
                printf(" enabled  -");
                if (OHCI_USBtransferFlag)
                {
                  #ifdef OHCI_USB_TRANSFER
                    ohci_setupUSBDevice(o, j); // TEST
                  #endif
                }
            }
            else
            {
                textColor(IMPORTANT);
                printf(" disabled -");
            }
            textColor(TEXT);

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PSS)
                printf(" suspend   -");
            else
                printf(" not susp. -");

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_POCI)
            {
                textColor(ERROR);
                printf(" overcurrent -");
                textColor(TEXT);
            }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PRS)
                printf(" reset -");

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PPS)
                printf(" pow on  -");
            else
                printf(" pow off -");

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CSC)
            {
                printf(" CSC -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_CSC;
            }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PESC)
            {
                printf(" enable Change -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PESC;
            }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PSSC)
            {
                printf(" resume compl. -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PSSC;
            }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_OCIC)
            {
                printf(" overcurrent Change -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_OCIC;
            }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PRSC)
            {
                printf(" Reset Complete -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PRSC;
            }
        }
    }
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
    for (uint8_t i=0; i<OHCIMAX; i++)
    {
        if (o == ohci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    if(!found || o == 0)
    {
      #ifdef _OHCI_DIAGNOSIS_
        printf("Interrupt did not came from OHCI device!\n");
      #endif
        return;
    }

    volatile uint32_t val = o->OpRegs->HcInterruptStatus;

    if(val==0)
    {
      #ifdef _OHCI_DIAGNOSIS_
        printf("Interrupt came from another OHCI device!\n");
      #endif
        return;
    }

    printf("\nUSB OHCI %u: ", o->num);

    uint32_t handled = 0;

    if (val & OHCI_INT_SO) // scheduling overrun
    {
        printf("Scheduling overrun.");
        handled |= OHCI_INT_SO;
    }

    if (val & OHCI_INT_WDH) // write back done head
    {
        printf("Write back done head.");
        //phys = o->hcca->doneHead;
        // TODO: handle ready transfer (ED, TD)
        handled |= OHCI_INT_WDH;
    }

    if (val & OHCI_INT_SF) // start of frame
    {
        printf("Start of frame.");
        handled |= OHCI_INT_SF;
    }

    if (val & OHCI_INT_RD) // resume detected
    {
        printf("Resume detected.");
        handled |= OHCI_INT_RD;
    }

    if (val & OHCI_INT_UE) // unrecoverable error
    {
        printf("Unrecoverable HC error.");
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
        handled |= OHCI_INT_UE;
    }

    if (val & OHCI_INT_FNO) // frame number overflow
    {
        printf("Frame number overflow.");
        handled |= OHCI_INT_FNO;
    }

    if (val & OHCI_INT_RHSC) // root hub status change
    {
        printf("Root hub status change.");
        handled |= OHCI_INT_RHSC;
        showPortstatus(o);
    }

    if (val & OHCI_INT_OC) // ownership change
    {
        printf("Ownership change.");
        handled |= OHCI_INT_OC;
    }

    if (val & ~handled)
    {
        printf("Unhandled interrupt: %X", val & ~handled);
    }

    o->OpRegs->HcInterruptStatus = val; // reset interrupts
}


/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

void ohci_setupUSBDevice(ohci_t* o, uint8_t portNumber)
{
    o->ports[portNumber]->num = 0; // device number has to be set to 0
    o->ports[portNumber]->num = 1 + usbTransferEnumerate(&o->ports[portNumber]->port, portNumber);

    disk_t* disk = malloc(sizeof(disk_t), 0, "disk_t"); // TODO: Handle non-MSDs
    disk->port = &o->ports[portNumber]->port;

    usb2_Device_t* device = usb2_createDevice(disk); // TODO: usb2 --> usb1 or usb (unified)
    
    usbTransferDevice(device);
    usbTransferConfig(device);
    usbTransferString(device);

    for (uint8_t i=1; i<4; i++) // fetch 3 strings
    {
        usbTransferStringUnicode(device, i);
    }

    usbTransferSetConfiguration(device, 1); // set first configuration

  #ifdef _OHCI_DIAGNOSIS_
    uint8_t config = usbTransferGetConfiguration(device);
    printf("\nconfiguration: %u", config); // check configuration
    waitForKeyStroke();
  #endif

    if (device->InterfaceClass != 0x08)
    {
        textColor(ERROR);
        printf("\nThis is no Mass Storage Device! MSD test and addition to device manager will not be carried out.");
        textColor(TEXT);
        waitForKeyStroke();
    }
    else
    {
        // Disk
        disk->type       = &USB_MSD;
        disk->sectorSize = 512;
        disk->port       = &o->ports[portNumber]->port;
        strcpy(disk->name, device->productName);
        attachDisk(disk);

        // Port
        o->ports[portNumber]->port.insertedDisk = disk;

      #ifdef _OHCI_DIAGNOSIS_
        showPortList(); // TEST
        showDiskList(); // TEST
      #endif
        waitForKeyStroke();

        // device, interface, endpoints
      #ifdef _OHCI_DIAGNOSIS_
        textColor(HEADLINE);
        printf("\n\nMSD test now with device: %X  interface: %u  endpOUT: %u  endpIN: %u\n",
                                                device, device->numInterfaceMSD,
                                                device->numEndpointOutMSD,
                                                device->numEndpointInMSD);
        textColor(TEXT);
      #endif

        testMSD(device); // test with some SCSI commands        
    }   
}


/*******************************************************************************************************
*                                                                                                      *
*                                            Transactions                                              *
*                                                                                                      *
*******************************************************************************************************/

typedef struct
{
    ohciTD_t*   qTD;
    void*       qTDBuffer;
    void*       inBuffer;
    size_t      inLength;
} ohci_transaction_t;


void ohci_setupTransfer(usb_transfer_t* transfer)
{
    transfer->HC->data = curOHCI;
    transfer->data = curOHCI->pED[indexED];
    indexED++;
}

void ohci_setupTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, uint32_t tokenBytes,
                           uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t i, uint32_t length)
{
    ohci_transaction_t* oTransaction = uTransaction->data = malloc(sizeof(ohci_transaction_t), 0, "ohci_transaction_t");
    oTransaction->inBuffer = 0;
    oTransaction->inLength = 0;

    oTransaction->qTD = ohci_createQTD_SETUP(1, toggle, tokenBytes, type, req, hiVal, loVal, i, length, &oTransaction->qTDBuffer);

    if(transfer->transactions->tail)
    {
        ohci_transaction_t* oLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data; // unused variable
        oLastTransaction->qTD->nextTD = paging_getPhysAddr(oTransaction->qTD); // no member next
    }
}

void ohci_inTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, void* buffer, size_t length)
{
    ohci_transaction_t* oTransaction = uTransaction->data = malloc(sizeof(ohci_transaction_t), 0, "ohci_transaction_t");
    oTransaction->inBuffer = buffer;
    oTransaction->inLength = length;

    oTransaction->qTD = ohci_createQTD_IO(1, 1, toggle, length);
    // oTransaction->qTDBuffer = ohci_allocQTDbuffer(oTransaction->qTD);

    if(transfer->transactions->tail)
    {
       ohci_transaction_t* oLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
       oLastTransaction->qTD->nextTD = paging_getPhysAddr(oTransaction->qTD);
    }
}

void ohci_outTransaction(usb_transfer_t* transfer, usb_transaction_t* uTransaction, bool toggle, void* buffer, size_t length)
{
    ohci_transaction_t* oTransaction = uTransaction->data = malloc(sizeof(ohci_transaction_t), 0, "ohci_transaction_t");
    oTransaction->inBuffer = 0;
    oTransaction->inLength = 0;

    oTransaction->qTD = ohci_createQTD_IO(1, 0, toggle, length);
    // oTransaction->qTDBuffer = ohci_allocQTDbuffer(oTransaction->qTD);

    if(buffer != 0 && length != 0)
    {
        memcpy(oTransaction->qTDBuffer, buffer, length);
    }

    if(transfer->transactions->tail)
    {
        ohci_transaction_t* oLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        oLastTransaction->qTD->nextTD = paging_getPhysAddr(oTransaction->qTD);
    }
}

void ohci_issueTransfer(usb_transfer_t* transfer)
{
    ohci_t* o = curOHCI; 
    
    // TEST
    textColor(TEXT);
    printf("\n\nED-Index: %u, Transfer->endpoint: %u, &o: %X", indexED-1, transfer->endpoint, o); // TEST for ununsed variable
    // TEST

    if(transfer->type == USB_CONTROL)
    {
       //// e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    }

    o->OpRegs->HcControlCurrentED = paging_getPhysAddr(transfer->data);

    ohci_transaction_t* firstTransaction = ((usb_transaction_t*)transfer->transactions->head->data)->data; // unused variable

    ohci_createQH(transfer->data, paging_getPhysAddr(transfer->data), firstTransaction->qTD,  1, ((ohci_port_t*)transfer->HC->data)->num, transfer->endpoint, transfer->packetSize);

    for(uint8_t i = 0; i < 5 && !transfer->success; i++)
    {
        if(transfer->type == USB_CONTROL)
        {
            // performAsyncScheduler(e, true, false, 0);
        }
        else
        {
            // performAsyncScheduler(e, true, true, 1 + transfer->packetSize/100);
        }

        transfer->success = true;
        for(dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
        {
            ohci_transaction_t* transaction = ((usb_transaction_t*)elem->data)->data; // unused variable
            ohci_showStatusbyteQTD(transaction->qTD);
            transfer->success = transfer->success && (transaction->qTD->cond == 0); // status (TD: condition)
        }
      #ifdef _OHCI_DIAGNOSIS_
        if(!transfer->success)
        {
            printf("\nRetry transfer: %u", i+1);
        }
      #endif
    }

    free(transfer->data);
    for(dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
    {
        ohci_transaction_t* transaction = ((usb_transaction_t*)elem->data)->data;

        if(transaction->inBuffer != 0 && transaction->inLength != 0)
        {
            memcpy(transaction->inBuffer, transaction->qTDBuffer, transaction->inLength);
        }
        // free(transaction->qTDBuffer);
        // free(transaction->qTD);
        free(transaction);
    }
    if(transfer->success)
    {
      //#ifdef _OHCI_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nTransfer successful.");
        textColor(TEXT);
      //#endif
    }
    else
    {
        textColor(ERROR);
        printf("\nTransfer failed.");
        textColor(TEXT);
    }
}


/*******************************************************************************************************
*                                                                                                      *
*                                            ohci ED TD functions                                      *
*                                                                                                      *
*******************************************************************************************************/

ohciTD_t* ohci_createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t i, uint32_t length, void** buffer)
{
    ohciTD_t* oTD = curOHCI->pTD[indexTD];
    
    oTD->nextTD      = next;
    oTD->direction   = 0; // SETUP
    oTD->toggle      = toggle;
    oTD->curBuffPtr  = (uintptr_t)curOHCI->pTD[indexTD];
    
    ohci_request_t* request = (ohci_request_t*)oTD->curBuffPtr;
    request->type    = type;
    request->request = req;
    request->valueHi = hiVal;
    request->valueLo = loVal;
    request->index   = i;
    request->length  = length;

    indexTD++;
    return (oTD);
}

ohciTD_t* ohci_createQTD_IO(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes)
{
    
    ohciTD_t* oTD = curOHCI->pTD[indexTD];
    
    oTD->nextTD     = next;
    oTD->direction  = direction;
    oTD->toggle     = toggle;
    oTD->curBuffPtr = curOHCI->pTDbuff[indexTD];
    
    // tokenBytes ??

    indexTD++;
    return (oTD);
}

void ohci_createQH(ohciED_t* head, uint32_t horizPtr, ohciTD_t* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    // head->nextED  = horizPtr;
    head->endpNum = endpoint;
    head->devAddr = device;
    head->mps     = packetSize;
    head->dir     = 0; // 00b Get direction From TD
    head->speed   = 0; // speed of the endpoint: full-speed (0), low-speed (1)
    head->sKip    = 0; // 1: HC continues on to next ED w/o attempting access to the TD queue or issuing any USB token for the endpoint
    head->format  = 0; // format of the TDs: Control, Bulk, or Interrupt Endpoint (0); Isochronous Endpoint (1)
    
    // H not needed? 

    if (firstQTD == 0)
    {
        head->tdQueueHead = 0x1; // no TD in queue
    }
    else
    {
        head->tdQueueHead = paging_getPhysAddr(firstQTD); // head TD in queue
    }
}


////////////////////
// analysis tools //
////////////////////

uint8_t ohci_showStatusbyteQTD(ohciTD_t* qTD)
{
    if (qTD->cond != 0x00)
    {
        printf("\n");
        textColor(ERROR);
        if (qTD->cond ==  1) { printf("\nLast data packet from endpoint contained a CRC error."); }
        if (qTD->cond ==  2) { printf("\nLast data packet from endpoint contained a bit stuffing violation."); }
        if (qTD->cond ==  3) { printf("\nLast packet from endpoint had data toggle PID that did not match the expected value."); }
        if (qTD->cond ==  4) { printf("\nTD was moved to the Done Queue because the endpoint returned a STALL PID."); }
        if (qTD->cond ==  5) { printf("\nDevice did not respond to token (IN) or did not provide a handshake (OUT)."); }
        if (qTD->cond ==  6) { printf("\nCheck bits on PID from endpoint failed on data PID (IN) or handshake (OUT)."); }
        if (qTD->cond ==  7) { printf("\nReceive PID was not valid when encountered or PID value is not defined."); }
        if (qTD->cond ==  8) { printf("\nDATAOVERRUN: Too many data returned by the endpoint."); }
        if (qTD->cond ==  9) { printf("\nDATAUNDERRUN: Endpoint returned less than MPS, not sufficient to fill the specified buffer"); }
        // 10 and 11 reserved 
        if (qTD->cond == 12) { printf("\nBUFFEROVERRUN"); }
        if (qTD->cond == 13) { printf("\nBUFFERUNDERRUN"); }
        // 14, 15 NOT ACCESSED: This code is set by software before the TD is placed on a list to be processed by the HC
        textColor(TEXT);
    }
    else
    {
        textColor(SUCCESS);
        printf("\nStatus: Successful Completion");
        textColor(TEXT);
    }
    return qTD->cond;
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
