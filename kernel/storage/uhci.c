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
#include "usb2.h"
#include "usb2_msd.h"

#define UHCI_USB_TRANSFER
#define NUMBER_OF_RETRIES 3

static uint8_t index   = 0;
static uhci_t* curUHCI = 0;
static uhci_t* uhci[UHCIMAX];


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
    uhci[index]->enabledPortFlag = false;

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
    u->framelistAddrVirt = (frPtr_t*) malloc(PAGESIZE, PAGESIZE, "uhci-framelist");
    // TODO: mutex for frame list

    uhciQH_t* qh     = malloc(sizeof(uhciQH_t), 16, "uhci-QH");
    qh->next         = BIT_T;
    qh->transfer     = BIT_T;
    qh->q_first      = 0;
    qh->q_last       = 0;
    u->qhPointerVirt = qh;

    for (uint16_t i=0; i<1024; i++)
    {
       u->framelistAddrVirt->frPtr[i] = paging_getPhysAddr(qh) | BIT_QH;
    }

    // define each millisecond one frame, provide physical address of frame list, and start at frame 0
    outportb(u->bar + UHCI_SOFMOD, 0x40); // SOF cycle time: 12000. For a 12 MHz SOF counter clock input, this produces a 1 ms Frame period.

    outportl(u->bar + UHCI_FRBASEADD, paging_getPhysAddr((void*)u->framelistAddrVirt->frPtr));
    outportw(u->bar + UHCI_FRNUM, 0);

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

    for (uint8_t j=0; j < min(u->rootPorts,UHCIPORTMAX); j++)
    {
        u->connected[j] = 0;
        uhci_resetPort(u, j);
        u->ports[j] = malloc(sizeof(uhci_port_t), 0, "uhci_port_t");
        u->ports[j]->num = j+1;
        u->ports[j]->uhci = u;
        u->ports[j]->port.type = &USB_UHCI; // device manager
        u->ports[j]->port.data = u->ports[j];
        u->ports[j]->port.insertedDisk = 0;
        snprintf(u->ports[j]->port.name, 14, "UHCI-Port %u", j+1);
        attachPort(&u->ports[j]->port);
        u->enabledPortFlag = true;
        uhci_showPortState(u, j);
    }
}

void uhci_resetPort(uhci_t* u, uint8_t port)
{
    outportw(u->bar + UHCI_PORTSC1 + 2 * port, UHCI_PORT_RESET);
    sleepMilliSeconds(50); // do not delete this wait
    outportw(u->bar + UHCI_PORTSC1 + 2 * port, inportw(u->bar + UHCI_PORTSC1+2*port) & ~UHCI_PORT_RESET);     // clear reset bit

    // wait and check, whether reset bit is really zero
    uint32_t timeout=20;
    while ((inportw(u->bar + UHCI_PORTSC1 + 2 * port) & UHCI_PORT_RESET) != 0)
    {
        sleepMilliSeconds(20);
        timeout--;
        if (timeout == 0)
        {
            textColor(ERROR);
            printf("\nTimeour Error: Port %u did not reset! ", port + 1);
            textColor(TEXT);
          #ifdef _UHCI_DIAGNOSIS_
            printf("Port Status: %Xh", inportw(u->bar + UHCI_PORTSC1 + 2 * port));
            waitForKeyStroke();
          #endif
            break;
        }
    }

    sleepMilliSeconds(10);

    // Enable
    outportw(u->bar + UHCI_PORTSC1 + 2 * port, UHCI_PORT_CS_CHANGE | UHCI_PORT_ENABLE_CHANGE // clear Change-Bits Connected and Enabled
                                                                   | UHCI_PORT_ENABLE);      // set Enable-Bit
    sleepMilliSeconds(10);

  #ifdef _UHCI_DIAGNOSIS_
    printf("Port Status: %Xh", inportw(u->bar + UHCI_PORTSC1 + 2 * port));
    waitForKeyStroke();
  #endif
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
    for (uint8_t i=0; i<UHCIMAX; i++)
    {
        if (u == uhci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    if(!found || u == 0) // Interrupt did not came from UHCI device
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("Interrupt did not came from UHCI device!\n");
      #endif
        return;
    }

    uint16_t reg = u->bar + UHCI_USBSTS;
    uint16_t val = inportw(reg);

    if(val==0) // Interrupt came from another UHCI device
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("Interrupt came from another UHCI device!\n");
      #endif
        return;
    }

    if (!(val & UHCI_STS_USBINT))
    {
        printf("\nUSB UHCI %u: ", u->num);
    }

    textColor(IMPORTANT);

    if (val & UHCI_STS_USBINT)
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("Frame: %u - USB transaction completed", inportw(u->bar + UHCI_FRNUM));
      #endif
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

void uhci_pollDisk(void* dev)
{
    uhci_t* u = ((uhci_port_t*)dev)->uhci;

    for(uint8_t port = 0; port < u->rootPorts; port++)
    {
        uint16_t val = inportw(u->bar + UHCI_PORTSC1 + 2*port);

        if(val & UHCI_PORT_CS_CHANGE)
        {
            printf("\nUHCI %u: Port %u changed: ", u->num, port);
            outportw(u->bar + UHCI_PORTSC1 + 2*port, UHCI_PORT_CS_CHANGE);

            if (val & UHCI_PORT_LOWSPEED_DEVICE)
            {
                printf("Lowspeed device");
            }
            else
            {
                printf("Fullspeed device");
            }

            if (val & UHCI_PORT_CS)
            {
                printf(" attached.");

                if (u->connected[port] == 0)       // ovverrun of 32-bit-counter
                {
                    u->connected[port] = 1;
                }

                uhci_resetPort(u, port);           ///// <--- reset on attached /////
                uhci_showPortState(u, port);
                waitForKeyStroke();

              #ifdef UHCI_USB_TRANSFER
                if (u->connected[port] == 1)
                {
                    uhci_setupUSBDevice(u, port); // TEST
                    u->connected[port] = 2;
                }
              #endif
            }
            else
            {
                printf(" removed.");
                u->connected[port] = 0; // reset connect (counter)
            }
        }
    }
}


/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

void uhci_setupUSBDevice(uhci_t* u, uint8_t portNumber)
{
    u->ports[portNumber]->num = 0; // device number has to be set to 0
    u->ports[portNumber]->num = 1 + usbTransferEnumerate(&u->ports[portNumber]->port, portNumber);
    waitForKeyStroke();

    disk_t* disk = malloc(sizeof(disk_t), 0, "disk_t"); // TODO: Handle non-MSDs
    disk->port = &u->ports[portNumber]->port;

    usb2_Device_t* device = usb2_createDevice(disk);
    usbTransferDevice(device); waitForKeyStroke();
    usbTransferConfig(device); waitForKeyStroke();
    usbTransferString(device); waitForKeyStroke();

    for (uint8_t i=1; i<4; i++) // fetch 3 strings
    {
        usbTransferStringUnicode(device, i); waitForKeyStroke();
    }

    usbTransferSetConfiguration(device, 1); // set first configuration
    waitForKeyStroke();

  #ifdef _UHCI_DIAGNOSIS_
    uint8_t config = usbTransferGetConfiguration(device);
    printf("\nconfiguration: %u", config); // check configuration
    waitForKeyStroke();
  #endif

    if (device->InterfaceClass != 0x08)
    {
        textColor(ERROR);
        printf("\nThis is no Mass Storage Device!\nMSD test and addition to device manager will not be carried out.");
        textColor(TEXT);
        waitForKeyStroke();
    }
    else
    {
        // Disk
        disk->type       = &USB_MSD;
        disk->sectorSize = 512;
        disk->port       = &u->ports[portNumber]->port;
        strcpy(disk->name, device->productName);
        attachDisk(disk);

        // Port
        u->ports[portNumber]->port.insertedDisk = disk;

      #ifdef _UHCI_DIAGNOSIS_
        showPortList(); // TEST
        showDiskList(); // TEST
      #endif
        waitForKeyStroke();

        // device, interface, endpoints
      #ifdef _UHCI_DIAGNOSIS_
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
    uhciTD_t*   TD;
    void*       TDBuffer;
    void*       inBuffer;
    size_t      inLength;
} uhci_transaction_t;

static bool isTransactionSuccessful(uhci_transaction_t* uT);
static void uhci_showStatusbyteTD(uhciTD_t* TD);

void uhci_setupTransfer(usb_transfer_t* transfer)
{
    uhci_t* u = ((uhci_port_t*)transfer->HC->data)->uhci; // HC
    transfer->data = u->qhPointerVirt; // QH

    transfer->device = ((uhci_port_t*)transfer->HC->data)->num;
}

void uhci_setupTransaction(usb_transfer_t* transfer, usb_transaction_t* usbTransaction, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t i, uint32_t length)
{
    uhci_transaction_t* uT = usbTransaction->data = malloc(sizeof(uhci_transaction_t), 0, "uhci_transaction_t");
    uT->inBuffer = 0;
    uT->inLength = 0;

    uhci_t* u = ((uhci_port_t*)transfer->HC->data)->uhci;

    uT->TD = uhci_createTD_SETUP(u, transfer->data, 1, toggle, tokenBytes, type, req, hiVal, loVal, i, length, &uT->TDBuffer,
                                              transfer->device, transfer->endpoint, transfer->packetSize);

  #ifdef _UHCI_DIAGNOSIS_
    usb_request_t* request = (usb_request_t*)uT->TDBuffer;
    printf("\ntype: %u req: %u valHi: %u valLo: %u i: %u len: %u", request->type, request->request, request->valueHi, request->valueLo, request->index, request->length);
  #endif

    if (transfer->transactions->tail)
    {
        uhci_transaction_t* uhciLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        uhciLastTransaction->TD->next = (paging_getPhysAddr(uT->TD) & 0xFFFFFFF0) | BIT_Vf; // build TD queue
        uhciLastTransaction->TD->q_next = uT->TD;
    }
}

void uhci_inTransaction(usb_transfer_t* transfer, usb_transaction_t* usbTransaction, bool toggle, void* buffer, size_t length)
{
    uhci_t* u = ((uhci_port_t*)transfer->HC->data)->uhci;
    uhci_transaction_t* uT = usbTransaction->data = malloc(sizeof(uhci_transaction_t), 0, "uhci_transaction_t");
    uT->inBuffer = buffer;
    uT->inLength = length;

    uT->TD = uhci_createTD_IO(u, transfer->data, 1, UHCI_TD_IN, toggle, length, transfer->device, transfer->endpoint, transfer->packetSize);
    uT->TDBuffer = uT->TD->virtBuffer;

    if (transfer->transactions->tail)
    {
        uhci_transaction_t* uhciLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        uhciLastTransaction->TD->next = (paging_getPhysAddr(uT->TD) & 0xFFFFFFF0) | BIT_Vf; // build TD queue
        uhciLastTransaction->TD->q_next = uT->TD;
    }
}

void uhci_outTransaction(usb_transfer_t* transfer, usb_transaction_t* usbTransaction, bool toggle, void* buffer, size_t length)
{
    uhci_t* u = ((uhci_port_t*)transfer->HC->data)->uhci;
    uhci_transaction_t* uT = usbTransaction->data = malloc(sizeof(uhci_transaction_t), 0, "uhci_transaction_t");
    uT->inBuffer = 0;
    uT->inLength = 0;

    uT->TD = uhci_createTD_IO(u, transfer->data, 1, UHCI_TD_OUT, toggle, length, transfer->device, transfer->endpoint, transfer->packetSize);
    uT->TDBuffer = uT->TD->virtBuffer;

    if (buffer != 0 && length != 0)
    {
        memcpy(uT->TDBuffer, buffer, length);
    }

    if (transfer->transactions->tail)
    {
        uhci_transaction_t* uhciLastTransaction = ((usb_transaction_t*)transfer->transactions->tail->data)->data;
        uhciLastTransaction->TD->next = (paging_getPhysAddr(uT->TD) & 0xFFFFFFF0) | BIT_Vf; // build TD queue
        uhciLastTransaction->TD->q_next = uT->TD;
    }
}

void uhci_issueTransfer(usb_transfer_t* transfer)
{
    uhci_t* u = ((uhci_port_t*)transfer->HC->data)->uhci; // HC
    uhci_transaction_t* firstTransaction = ((usb_transaction_t*)transfer->transactions->head->data)->data;
    uhci_createQH(u, transfer->data, (uintptr_t)transfer->data, firstTransaction->TD);

    for (uint8_t i = 0; i < NUMBER_OF_RETRIES && !transfer->success; i++)
    {
      #ifdef _UHCI_DIAGNOSIS_
        printf("\ntransfer try = %u\n", i);
      #endif

        transfer->success = true;

        // start scheduler
        outportl(u->bar + UHCI_FRBASEADD, paging_getPhysAddr((void*)u->framelistAddrVirt->frPtr));
        outportw(u->bar + UHCI_FRNUM, 0);
        outportw(u->bar + UHCI_USBCMD, inportw(u->bar + UHCI_USBCMD) | UHCI_CMD_RS);

        // run transactions
        for(dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
        {
          #ifdef _UHCI_DIAGNOSIS_
            uint16_t num = inportw(u->bar + UHCI_FRNUM);
            printf("\nFRBADDR: %X  frame pointer: %X frame number: %u", inportl(u->bar + UHCI_FRBASEADD), u->framelistAddrVirt->frPtr[num], num);
          #endif

          delay(50000); // pause after transaction
        }
        delay(50000); // pause after transfer

        // stop scheduler
        outportw(u->bar + UHCI_USBCMD, inportw(u->bar + UHCI_USBCMD) & ~UHCI_CMD_RS);

        // check conditions and save data
        for (dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
        {
            uhci_transaction_t* uT = ((usb_transaction_t*)elem->data)->data;
            uhci_showStatusbyteTD(uT->TD);
            transfer->success = transfer->success && isTransactionSuccessful(uT); // executed w/o error

            if(uT->inBuffer != 0 && uT->inLength != 0)
            {
                memcpy(uT->inBuffer, uT->TDBuffer, uT->inLength);
            }
        }

      #ifdef _UHCI_DIAGNOSIS_
        printf("\nQH: %X  QH->transfer: %X", paging_getPhysAddr(transfer->data), ((uhciQH_t*)transfer->data)->transfer);

        for (dlelement_t* elem = transfer->transactions->head; elem != 0; elem = elem->next)
        {
            uhci_transaction_t* uT = ((usb_transaction_t*)elem->data)->data;
            printf("\nTD: %X next: %X", paging_getPhysAddr(uT->TD), uT->TD->next);
        }
      #endif

        if(transfer->success)
        {
          #ifdef _UHCI_DIAGNOSIS_
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
}


/*******************************************************************************************************
*                                                                                                      *
*                                            uhci QH TD functions                                      *
*                                                                                                      *
*******************************************************************************************************/

static uhciTD_t* uhci_allocTD(uintptr_t next)
{
    uhciTD_t* td = (uhciTD_t*)malloc(sizeof(uhciTD_t), 16, "uhciTD"); // 16 byte alignment
    memset(td, 0, sizeof(uhciTD_t));

    if (next != BIT(0))
    {
        td->next   = (paging_getPhysAddr((void*)next) & 0xFFFFFFF0) | BIT_Vf;
        td->q_next = (void*)next;
    }
    else
    {
        td->next = BIT_T;
    }

    td->active             = 1;  // to be executed
    td->intOnComplete      = 1;  // We want an interrupt after complete transfer
    td->PacketID           = UHCI_TD_SETUP;
    td->maxLength          = 0x3F; // 64 byte // uhci, rev. 1.1, page 24

    return td;
}

static void* uhci_allocTDbuffer(uhciTD_t* td)
{
    td->virtBuffer = malloc(1024, 0, "uhciTD-buffer");
    memset(td->virtBuffer, 0, 1024);
    td->buffer = paging_getPhysAddr(td->virtBuffer);

    return td->virtBuffer;
}


uhciTD_t* uhci_createTD_SETUP(uhci_t* u, uhciQH_t* uQH, uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t i, uint32_t length, void** buffer, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    uhciTD_t* td = uhci_allocTD(next);

    td->PacketID      = UHCI_TD_SETUP;

    td->dataToggle    = toggle; // Should be toggled every list entry

    td->deviceAddress = device;
    td->endpoint      = endpoint;
    td->maxLength     = tokenBytes-1;

    usb_request_t* request = *buffer = td->virtBuffer = uhci_allocTDbuffer(td);
    request->type    = type;
    request->request = req;
    request->valueHi = hiVal;
    request->valueLo = loVal;
    request->index   = i;
    request->length  = length;

    uQH->q_last = td;
    return (td);
}

uhciTD_t* uhci_createTD_IO(uhci_t* u, uhciQH_t* uQH, uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes, uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
    uhciTD_t* td = uhci_allocTD(next);

    td->PacketID      = direction;

    if (tokenBytes)
    {
        td->maxLength = (tokenBytes-1) & 0x7FF;
    }
    else
    {
        td->maxLength = 0x7FF;
    }

    td->dataToggle    = toggle; // Should be toggled every list entry

    td->deviceAddress = device;
    td->endpoint      = endpoint;

    td->virtBuffer    = uhci_allocTDbuffer(td);
    td->buffer        = paging_getPhysAddr(td->virtBuffer);

    uQH->q_last = td;
    return (td);
}

void uhci_createQH(uhci_t* u, uhciQH_t* head, uint32_t horizPtr, uhciTD_t* firstTD)
{
    head->next = BIT_T; // (paging_getPhysAddr((void*)horizPtr) & 0xFFFFFFF0) | BIT_QH;

    if (firstTD == 0)
    {
        head->transfer = BIT_T;
    }

    else
    {
        head->transfer = (paging_getPhysAddr(firstTD) & 0xFFFFFFF0);
        head->q_first  = firstTD;
    }
}


////////////////////
// analysis tools //
////////////////////

void uhci_showStatusbyteTD(uhciTD_t* TD)
{
    textColor(ERROR);
    if (TD->bitstuffError)     printf("\nBitstuff Error");          // receive data stream contained a sequence of more than 6 ones in a row
    if (TD->crc_timeoutError)  printf("\nNo Response from Device"); // no response from the device (CRC or timeout)
    if (TD->nakReceived)       printf("\nNAK received");            // NAK handshake
    if (TD->babbleDetected)    printf("\nBabble detected");         // Babble (fatal error), e.g. more data from the device than MAXP
    if (TD->dataBufferError)   printf("\nData Buffer Error");       // HC cannot keep up with the data  (overrun) or cannot supply data fast enough (underrun)
    if (TD->stall)             printf("\nStalled");                 // can be caused by babble, error counter (0) or STALL from device

    textColor(GRAY);
    if (TD->active)            printf("\nactive");                  // 1: HC will execute   0: set by HC after excution (HC will not excute next time)

  #ifdef _UHCI_DIAGNOSIS_
    textColor(IMPORTANT);
    if (TD->intOnComplete)     printf("\ninterrupt on complete");   // 1: HC issues interrupt on completion of the frame in which the TD is executed
    if (TD->isochrSelect)      printf("\nisochronous TD");          // 1: Isochronous TD
    if (TD->lowSpeedDevice)    printf("\nLowspeed Device");         // 1: LS   0: FS
    if (TD->shortPacketDetect) printf("\nShortPacketDetect");       // 1: enable   0: disable
  #endif

    textColor(TEXT);
}

bool isTransactionSuccessful(uhci_transaction_t* uT)
{
	if
	(
	    // no error
		(uT->TD->bitstuffError  == 0) && (uT->TD->crc_timeoutError == 0) && (uT->TD->nakReceived == 0) &&
		(uT->TD->babbleDetected == 0) && (uT->TD->dataBufferError  == 0) && (uT->TD->stall       == 0) &&
	    // executed
	    (uT->TD->active == 0)
	)
	{
		return (true);
	}
	else
	{
		return (false);
	}
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
