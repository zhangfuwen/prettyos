/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb2.h"
#include "kheap.h"
#include "paging.h"
#include "video/console.h"
#include "timer.h"
#include "util.h"
#include "ehciQHqTD.h"


extern ehci_t* curEHCI;

const uint8_t ALIGNVALUE = 32;

usb2_Device_t usbDevices[16]; // ports 1-16

extern void* globalqTD[3];
extern void* globalqTDbuffer[3];


uint8_t usbTransferEnumerate(ehci_t* e, uint8_t j)
{
  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: SET_ADDRESS");
    textColor(TEXT);
  #endif

    uint8_t new_address = j; // indicated port number

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-Enum");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 5, 0, new_address+1, 0, 0); // SETUP DATA0, 8 byte, ..., SET_ADDRESS, hi, 0...127 (new address), index=0, length=0

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, 0, 0,64);

    performAsyncScheduler(e, true, false, 0);

    free(e->virtualAsyncList);

    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }

    return new_address;
}

void usbTransferDevice(uint32_t device)
{
  ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: GET_DESCRIPTOR device, dev: %u endpoint: 0", device+1);
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-Device");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1, OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 18);  // IN DATA1, 18 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 1, 0, 0, 18); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, hi, lo, index, length

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false,0);

    // showPacket(DataQTDpage0,18);
    addDevice ((struct usb2_deviceDescriptor*)DataQTDpage0, &usbDevices[device]);
    showDevice(&usbDevices[device]);

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void usbTransferConfig(uint32_t device)
{
    ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: GET_DESCRIPTOR config, dev: %u endpoint: 0", device+1);
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-Config");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, ALIGNVALUE);  // IN DATA1, 4096 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 2, 0, 0, ALIGNVALUE); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, configuration, lo, index, length

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false, 0);

  #ifdef _USB2_DIAGNOSIS_
    textColor(LIGHT_GRAY);
    printf("\n---------------------------------------------------------------------\n");
    textColor(GREEN);
  #endif

    // parsen auf config (len=9,type=2), interface (len=9,type=4), endpoint (len=7,type=5)
    uintptr_t addrPointer = (uintptr_t)DataQTDpage0;
    uintptr_t lastByte    = addrPointer + (*(uint16_t*)(addrPointer+2)); // totalLength (WORD)

  #ifdef _USB2_DIAGNOSIS_
    showPacket(DataQTDpage0,(*(uint16_t*)(addrPointer+2)));
  #endif

    while (addrPointer<lastByte)
    {
        bool found = false;

        if (*(uint8_t*)addrPointer == 9 && *(uint8_t*)(addrPointer+1) == 2) // length, type
        {
            showConfigurationDescriptor((struct usb2_configurationDescriptor*)addrPointer);
            addrPointer += 9;
            found = true;
        }

        if (*(uint8_t*)addrPointer == 9 && *(uint8_t*)(addrPointer+1) == 4) // length, type
        {
            showInterfaceDescriptor((struct usb2_interfaceDescriptor*)addrPointer);

            if (((struct usb2_interfaceDescriptor*)addrPointer)->interfaceClass == 8)
            {
                // store interface number for mass storage transfers
                usbDevices[device].numInterfaceMSD    = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceNumber;
                usbDevices[device].InterfaceClass     = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceClass;
                usbDevices[device].InterfaceSubclass  = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceSubclass;
            }
            addrPointer += 9;
            found = true;
        }

        if (*(uint8_t*)addrPointer == 7 && *(uint8_t*)(addrPointer+1) == 5) // length, type
        {
            showEndpointDescriptor((struct usb2_endpointDescriptor*)addrPointer);

            // store endpoint numbers for IN/OUT mass storage transfers, attributes must be 0x2, because there are also endpoints with attributes 0x3(interrupt)
            if (((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0x80 && ((struct usb2_endpointDescriptor*)addrPointer)->attributes == 0x2)
            {
                usbDevices[device].numEndpointInMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
            }

            if (!(((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0x80) && ((struct usb2_endpointDescriptor*)addrPointer)->attributes == 0x2)
            {
                usbDevices[device].numEndpointOutMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
            }

            addrPointer += 7;
            found = true;
        }

        if (*(uint8_t*)(addrPointer+1) != 2 && *(uint8_t*)(addrPointer+1) != 4 && *(uint8_t*)(addrPointer+1) != 5) // length, type
        {
          #ifdef _USB2_DIAGNOSIS_
            if ((*(uint8_t*)addrPointer) > 0)
            {
                textColor(HEADLINE);
                printf("\nlength: %u type: %u unknown\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
                textColor(TEXT);
            }
          #endif
            addrPointer += *(uint8_t*)addrPointer;
            found = true;
        }

        if (found == false)
        {
          #ifdef _USB2_DIAGNOSIS_
            printf("\nlength: %u type: %u not found\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
          #endif
            break;
        }
    }

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void usbTransferString(uint32_t device)
{
    ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: GET_DESCRIPTOR string, dev: %u endpoint: 0 languageIDs", device+1);
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-String");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 12);  // IN DATA1, 12 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, 0, 0, 12); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, lo, index, length

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false, 0);

  #ifdef _USB2_DIAGNOSIS_
    showPacket(DataQTDpage0,12);
  #endif
    showStringDescriptor((struct usb2_stringDescriptor*)DataQTDpage0);

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void usbTransferStringUnicode(uint32_t device, uint32_t stringIndex)
{
    ehci_t* e = curEHCI;  

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: GET_DESCRIPTOR string, dev: %u endpoint: 0 stringIndex: %u", device+1, stringIndex);
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-wideStr");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 64);  // IN DATA1, 64 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, stringIndex, 0x0409, 64); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, stringIndex, languageID, length

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false, 0);

  #ifdef _USB2_DIAGNOSIS_
    showPacket(DataQTDpage0,64);
  #endif

    showStringDescriptorUnicode((struct usb2_stringDescriptorUnicode*)DataQTDpage0, device, stringIndex);

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

// http://www.lowlevel.eu/wiki/USB#SET_CONFIGURATION
void usbTransferSetConfiguration(uint32_t device, uint32_t configuration)
{
    ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nUSB2: SET_CONFIGURATION %u",configuration);
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-SetConf");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 9, 0, configuration, 0, 0); // SETUP DATA0, 8 byte, request type, SET_CONFIGURATION(9), hi(reserved), configuration, index=0, length=0

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false, 0);

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

uint8_t usbTransferGetConfiguration(uint32_t device)
{
    ehci_t* e = curEHCI;
  
  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: GET_CONFIGURATION");
    textColor(TEXT);
  #endif

    e->virtualAsyncList = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-GetConf");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(e->virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(OUT);
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 1);  // IN DATA1, 1 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 8, 0, 0, 0, 1); // SETUP DATA0, 8 byte, request type, GET_CONFIGURATION(9), hi, lo, index=0, length=1

    // Create QH
    createQH(e->virtualAsyncList, paging_getPhysAddr(e->virtualAsyncList), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(e, true, false, 0);

    uint8_t configuration = *((uint8_t*)DataQTDpage0);

    free(e->virtualAsyncList);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }

    return configuration;
}

// new control transfer as TEST /////////////////////////////////////////////////
// seems not to work correct, does not set HALT ???
void usbSetFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
  ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: usbSetFeatureHALT, endpoint: %u", endpoint);
    textColor(TEXT);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-SetHalt");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x02, 3, 0, 0, endpoint, 0);
    // bmRequestType bRequest  wValue   wIndex  wLength   Data
    //     00000010b        3   0000h endpoint    0000h   none

    // Create QH
    createQH(QH, paging_getPhysAddr(QH), SetupQTD, 1, device+1, endpoint, packetSize); // endpoint

    performAsyncScheduler(e, true, false, 3);

  #ifdef _USB2_DIAGNOSIS_
    printf("\nset HALT at dev: %u endpoint: %u", device+1, endpoint);
  #endif

    free(QH);
    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void usbClearFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
  ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(HEADLINE);
    printf("\nUSB2: usbClearFeatureHALT, endpoint: %u", endpoint);
    textColor(TEXT);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-aL-QH-ClrHalt");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x02, 1, 0, 0, endpoint, 0);
    // bmRequestType bRequest  wValue   wIndex  wLength   Data
    //     00000010b        1   0000h endpoint    0000h   none

    // Create QH
    createQH(QH, paging_getPhysAddr(QH), SetupQTD, 1, device+1, endpoint, packetSize); // endpoint

    performAsyncScheduler(e, true, false, 3);

  #ifdef _USB2_DIAGNOSIS_
    printf("\nclear HALT at dev: %u endpoint: %u", device+1, endpoint);
  #endif

    free(QH);
    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

uint16_t usbGetStatus(uint32_t device, uint32_t endpoint, uint32_t packetSize)
{
  ehci_t* e = curEHCI;

  #ifdef _USB2_DIAGNOSIS_
    textColor(YELLOW);
    printf("\nusbGetStatus at device: %u endpoint: %u", device+1, endpoint);
    textColor(TEXT);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "usb2-QH-getStatus");
    e->OpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    e->OpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(OUT);
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 2);  // IN DATA1, 2 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x02, 0, 0, 0, endpoint, 2);
    // bmRequestType bRequest  wValue   wIndex  wLength   Data
    //     00000010b       0b   0000h endpoint    0002h   2 byte

    // Create QH
    createQH(QH, paging_getPhysAddr(QH), SetupQTD, 1, device+1, endpoint, packetSize); // endpoint

    performAsyncScheduler(e, true, false, 0);

    uint16_t status = *((uint16_t*)DataQTDpage0);

    free(QH);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }

    return status;
}

void addDevice(struct usb2_deviceDescriptor* d, usb2_Device_t* usbDev)
{
    usbDev->usbSpec               = d->bcdUSB;
    usbDev->usbClass              = d->deviceClass;
    usbDev->usbSubclass           = d->deviceSubclass;
    usbDev->usbProtocol           = d->deviceProtocol;
    usbDev->maxPacketSize         = d->maxPacketSize;
    usbDev->vendor                = d->idVendor;
    usbDev->product               = d->idProduct;
    usbDev->releaseNumber         = d->bcdDevice;
    usbDev->manufacturerStringID  = d->manufacturer;
    usbDev->productStringID       = d->product;
    usbDev->serNumberStringID     = d->serialNumber;
    usbDev->numConfigurations     = d->numConfigurations;
}

void showDevice(usb2_Device_t* usbDev)
{
    textColor(IMPORTANT);
    printf("\nUSB %u.%u\t",    BYTE2(usbDev->usbSpec), BYTE1(usbDev->usbSpec)); // e.g. 0x0210 means 2.10

    if (usbDev->usbClass == 0x09)
    {
        switch (usbDev->usbProtocol)
        {
            case 0:
                printf("Full speed USB hub");
                break;
            case 1:
                printf("Hi-speed USB hub with single TT");
                break;
            case 2:
                printf("Hi-speed USB hub with multiple TTs");
                break;
        }
    }

  #ifdef _USB2_DIAGNOSIS_
    printf("\nendpoint 0 mps: %u byte.", usbDev->maxPacketSize); // MPS0, must be 8,16,32,64
    printf("vendor:            %xh\n",   usbDev->vendor);
    printf("product:           %xh\t",   usbDev->product);
    printf("release number:    %u.%u\n", BYTE2(usbDev->releaseNumber), BYTE1(usbDev->releaseNumber));
    printf("manufacturer:      %xh\t",   usbDev->manufacturerStringID);
    printf("product:           %xh\n",   usbDev->productStringID);
    printf("serial number:     %xh\t",   usbDev->serNumberStringID);
    printf("number of config.: %u\n",    usbDev->numConfigurations); // number of possible configurations
    printf("numInterfaceMSD:   %u\n",    usbDev->numInterfaceMSD);
  #endif

    textColor(TEXT);
}

void showConfigurationDescriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
      #ifdef _USB2_DIAGNOSIS_
        textColor(IMPORTANT);
        printf("length:               %u\t\t",  d->length);
        printf("descriptor type:      %u\n",  d->descriptorType);
        textColor(LIGHT_GRAY);
        printf("total length:         %u\t",  d->totalLength);
      #endif
        textColor(IMPORTANT);
        printf("\nNumber of interfaces: %u",  d->numInterfaces);
      #ifdef _USB2_DIAGNOSIS_
        printf("ID of config:         %xh\t",  d->configurationValue);
        printf("ID of config name     %xh\n",  d->configuration);
        printf("remote wakeup:        %s\t",  d->attributes & BIT(5) ? "yes" : "no");
        printf("self-powered:         %s\n",  d->attributes & BIT(6) ? "yes" : "no");
        printf("max power (mA):       %u\n",  d->maxPower*2); // 2 mA steps used
      #endif
        textColor(TEXT);
    }
}

void showInterfaceDescriptor(struct usb2_interfaceDescriptor* d)
{
    if (d->length)
    {
        putch('\n');
      #ifdef _USB2_DIAGNOSIS_
        textColor(LIGHT_GRAY);
        printf("---------------------------------------------------------------------\n");
      #endif
        textColor(IMPORTANT);
      #ifdef _USB2_DIAGNOSIS_
        printf("length:               %u\t\t", d->length);          // 9
        printf("descriptor type:      %u\n",   d->descriptorType);  // 4
      #endif
        switch (d->numEndpoints)
        {
            case 0:
                printf("Interface %u has no endpoint and belongs to class:\n", d->interfaceNumber);
                break;
            case 1:
                printf("Interface %u has only one endpoint and belongs to class:\n", d->interfaceNumber);
                break;
            default:
                printf("Interface %u has %u endpoints and belongs to class:\n", d->interfaceNumber, d->numEndpoints);
                break;
        }

        switch (d->interfaceClass)
        {
            case 0x01:
                printf("Audio");
                break;
            case 0x02:
                printf("Communications and CDC Control");
                break;
            case 0x03:
                printf("HID (Human Interface Device)");
                break;
            case 0x05:
                printf("Physical");
                break;
            case 0x06:
                printf("Image");
                break;
            case 0x07:
                printf("Printer");
                break;
            case 0x08:
                printf("Mass Storage, ");
                switch (d->interfaceSubclass)
                {
                    case 0x01:
                        printf("Reduced Block Commands, ");
                        break;
                    case 0x02:
                        printf("SFF-8020i or MMC-2(ATAPI), ");
                        break;
                    case 0x03:
                        printf("QIC-157 (tape device), ");
                        break;
                    case 0x04:
                        printf("UFI (e.g. Floppy Disk), ");
                        break;
                    case 0x05:
                        printf("SFF-8070i (e.g. Floppy Disk), ");
                        break;
                    case 0x06:
                        printf("SCSI transparent command set, ");
                        break;
                }
                switch (d->interfaceProtocol)
                {
                    case 0x00:
                        printf("CBI protocol with command completion interrupt.");
                        break;
                    case 0x01:
                        printf("CBI protocol without command completion interrupt.");
                        break;
                    case 0x50:
                        printf("Bulk-Only Transport protocol.");
                        break;
                }
                break;
            case 0x0A:
                printf("CDC-Data");
                break;
            case 0x0B:
                printf("Smart Card");
                break;
            case 0x0D:
                printf("Content Security");
                break;
            case 0x0E:
                printf("Video");
                break;
            case 0x0F:
                printf("Personal Healthcare");
                break;
            case 0xDC:
                printf("Diagnostic Device");
                break;
            case 0xE0:
                printf("Wireless Controller, subclass: %yh protocol: %yh.",d->interfaceSubclass,d->interfaceProtocol);
                break;
            case 0xEF:
                printf("Miscellaneous");
                break;
            case 0xFE:
                printf("Application Specific");
                break;
            case 0xFF:
                printf("Vendor Specific");
                break;
        }

      #ifdef _USB2_DIAGNOSIS_
        printf("\nalternate Setting:  %u\n",   d->alternateSetting);
        printf("interface class:      %u\n",   d->interfaceClass);
        printf("interface subclass:   %u\n",   d->interfaceSubclass);
        printf("interface protocol:   %u\n",   d->interfaceProtocol);
        printf("interface:            %xh\n",  d->interface);
      #endif

        textColor(TEXT);
        sleepSeconds(1); // wait to show data
    }
}

void showEndpointDescriptor(struct usb2_endpointDescriptor* d)
{
  #ifdef _USB2_DIAGNOSIS_
    if (d->length)
    {
        textColor(LIGHT_GRAY);
        printf("\n---------------------------------------------------------------------\n");
        textColor(IMPORTANT);
        printf("length:      %u\t\t",   d->length);         // 7
        printf("descriptor type: %u\n", d->descriptorType); // 5
        printf("endpoint %u: %s, ",     d->endpointAddress & 0xF, d->endpointAddress & 0x80 ? "IN " : "OUT");
        printf("attributes: %yh\t\t",   d->attributes);
        // bit 1:0 00 control    01 isochronous    10 bulk                         11 interrupt
        // bit 3:2 00 no sync    01 async          10 adaptive                     11 sync (only if isochronous)
        // bit 5:4 00 data endp. 01 feedback endp. 10 explicit feedback data endp. 11 reserved (Iso Mode)

        if (d->attributes == 2)
        {
           printf(" bulk data,");
        }
        printf(" mps: %u byte",  d->maxPacketSize);
        printf("interval:          %u\n",  d->interval);
        textColor(TEXT);
    }
  #endif
}

void showStringDescriptor(struct usb2_stringDescriptor* d)
{
    if (d->length)
    {
        textColor(IMPORTANT);

      #ifdef _USB2_DIAGNOSIS_
        printf("\nlength:            %u\t\t",  d->length);     // 12
        printf("descriptor type:   %u\n",  d->descriptorType); //  3
      #endif

        printf("\n\nlanguages: ");
        for (uint8_t i=0; i<10;i++)
        {
            if (d->languageID[i] >= 0x0400 && d->languageID[i] <= 0x0465)
            {
                switch (d->languageID[i])
                {
                    case 0x401:
                        printf("Arabic");
                        break;
                    case 0x404:
                        printf("Chinese");
                        break;
                    case 0x407:
                        printf("German");
                        break;
                    case 0x409:
                        printf("English");
                        break;
                    case 0x40A:
                        printf("Spanish");
                        break;
                    case 0x40C:
                        printf("French");
                        break;
                    case 0x410:
                        printf("Italian");
                        break;
                    case 0x411:
                        printf("Japanese");
                        break;
                    case 0x416:
                        printf("Portuguese");
                        break;
                    case 0x419:
                        printf("Russian");
                        break;
                    default:
                        printf("language code: %xh", d->languageID[i]);
                        /*Language Codes
                        ; 0x400 Neutral
                        ; 0x401 Arabic
                        ; 0x402 Bulgarian
                        ; 0x403 Catalan
                        ; 0x404 Chinese
                        ; 0x405 Czech
                        ; 0x406 Danish
                        ; 0x407 German
                        ; 0x408 Greek
                        ; 0x409 English
                        ; 0x40a Spanish
                        ; 0x40b Finnish
                        ; 0x40c French
                        ; 0x40d Hebrew
                        ; 0x40e Hungarian
                        ; 0x40f Icelandic
                        ; 0x410 Italian
                        ; 0x411 Japanese
                        ; 0x412 Korean
                        ; 0x413 Dutch
                        ; 0x414 Norwegian
                        ; 0x415 Polish
                        ; 0x416 Portuguese
                        ; 0x418 Romanian
                        ; 0x419 Russian
                        ; 0x41a Croatian
                        ; 0x41a Serbian
                        ; 0x41b Slovak
                        ; 0x41c Albanian
                        ; 0x41d Swedish
                        ; 0x41e Thai
                        ; 0x41f Turkish
                        ; 0x420 Urdu
                        ; 0x421 Indonesian
                        ; 0x422 Ukrainian
                        ; 0x423 Belarusian
                        ; 0x424 Slovenian
                        ; 0x425 Estonian
                        ; 0x426 Latvian
                        ; 0x427 Lithuanian
                        ; 0x429 Farsi
                        ; 0x42a Vietnamese
                        ; 0x42b Armenian
                        ; 0x42c Azeri
                        ; 0x42d Basque
                        ; 0x42f Macedonian
                        ; 0x436 Afrikaans
                        ; 0x437 Georgian
                        ; 0x438 Faeroese
                        ; 0x439 Hindi
                        ; 0x43e Malay
                        ; 0x43f Kazak
                        ; 0x440 Kyrgyz
                        ; 0x441 Swahili
                        ; 0x443 Uzbek
                        ; 0x444 Tatar
                        ; 0x446 Punjabi
                        ; 0x447 Gujarati
                        ; 0x449 Tamil
                        ; 0x44a Telugu
                        ; 0x44b Kannada
                        ; 0x44e Marathi
                        ; 0x44f Sanskrit
                        ; 0x450 Mongolian
                        ; 0x456 Galician
                        ; 0x457 Konkani
                        ; 0x45a Syriac
                        ; 0x465 Divehi */
                        break;
                }
            }
        }
        printf("\n");
        textColor(TEXT);
    }
}

void showStringDescriptorUnicode(struct usb2_stringDescriptorUnicode* d, uint32_t device, uint32_t stringIndex)
{
    if (d->length)
    {
      #ifdef _USB2_DIAGNOSIS_
        textColor(IMPORTANT);
        printf("\nlength:            %u\t\t",  d->length);
        printf("descriptor type:   %u\n",  d->descriptorType); // 3
        printf("string: ");
      #endif

        textColor(YELLOW);
        for (uint8_t i=0; i<(d->length-2);i+=2) // show only low value of Unicode character
        {
            if (d->widechar[i])
            {
                putch(d->widechar[i]); // output
                d->asciichar[i/2] = d->widechar[i];
            }
        }
        printf("\t");
        textColor(TEXT);

        if (stringIndex == 2) // product name
        {
            strncpy(usbDevices[device].productName, (char*)d->asciichar, 15);

          #ifdef _USB2_DIAGNOSIS_
            printf(" product name: %s", usbDevices[device].productName);
          #endif
        }
        else if (stringIndex == 3) // serial number
        {
            // take the last 12 characters:

            // find the last character
            int16_t j=0; // start at the front
            while (d->asciichar[j]) // not '\0'
            {
                j++;     // go to the next character
            }
            int16_t last = j; // store last position
            j = max(j-12, 0); // step 12 characters backwards, but not below zero

            for (uint16_t index=0; index<13; index++)
            {
                if (j+index>last)
                {
                    usbDevices[device].serialNumber[index] = 0;
                    break;
                }
                else
                {
                    usbDevices[device].serialNumber[index] = d->asciichar[j+index];
                }
            }
          #ifdef _USB2_DIAGNOSIS_
            printf(" serial: %s", usbDevices[device].serialNumber);
          #endif
        }
    }
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
