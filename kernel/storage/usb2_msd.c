/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "paging.h"
#include "kheap.h"
#include "video/console.h"
#include "util.h"
#include "usb2.h"
#include "usb2_msd.h"

extern const uint8_t ALIGNVALUE;

extern void* globalqTD[3];
extern void* globalqTDbuffer[3];

extern const uint32_t CSWMagicNotOK;
static const uint32_t CSWMagicOK = 0x53425355; // USBS
static const uint32_t CBWMagic   = 0x43425355; // USBC

static uint8_t currentDevice;
static uint8_t currCSWtag;

static void* cmdQTD;
static void* StatusQTD;

static int32_t numberTries = 10; // repeats for IN-Transfer

uint32_t usbMSDVolumeMaxLBA;

extern usb2_Device_t usbDevices[16]; // ports 1-16


// Bulk-Only Mass Storage get maximum number of Logical Units
uint8_t usbTransferBulkOnlyGetMaxLUN(uint32_t device, uint8_t numInterface)
{
  #ifdef _USB_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nUSB2: usbTransferBulkOnlyGetMaxLUN, dev: %u interface: %u", device+1, numInterface);
    textColor(TEXT);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "QH-GetMaxLun");
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH);

    // Create QTDs (in reversed order)
    void* next      = createQTD_Handshake(OUT); // Handshake is the opposite direction of Data
    globalqTD[2] = globalqTD[0]; globalqTDbuffer[2] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = DataQTD  = createQTD_IO( (uintptr_t)next, IN, 1, 1);  // IN DATA1, 1 byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0xA1, 0xFE, 0, 0, numInterface, 1);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 10100001b     11111110b 0000h  Interface 0001h     1 byte

    // Create QH
    createQH(QH, paging_getPhysAddr(QH), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(true, false, 0);

    free(QH);
    for (uint8_t i=0; i<=2; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }

    return *((uint8_t*)DataQTDpage0);
}

// Bulk-Only Mass Storage Reset
void usbTransferBulkOnlyMassStorageReset(uint32_t device, uint8_t numInterface)
{
  #ifdef _USB_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nUSB2: usbTransferBulkOnlyMassStorageReset, dev: %u interface: %u", device+1, numInterface);
    textColor(TEXT);
  #endif

    void* QH = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "QH-MSD-Reset");
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x21, 0xFF, 0, 0, numInterface, 0);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 00100001b     11111111b 0000h  Interface 0000h     none

    // Create QH
    createQH(QH, paging_getPhysAddr(QH), SetupQTD, 1, device+1, 0, 64); // endpoint 0

    performAsyncScheduler(true, false, 0);

    free(QH);
    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void SCSIcmd(uint8_t SCSIcommand, struct usb2_CommandBlockWrapper* cbw, uint32_t LBA, uint16_t TransferLength)
{
    switch (SCSIcommand)
    {
        case 0x00: // test unit ready(6)
            cbw->CBWSignature          = CBWMagic;      // magic
            cbw->CBWTag                = 0x42424200;    // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWFlags              = 0x00;          // Out: 0x00  In: 0x80
            cbw->CBWCBLength           = 6;             // only bits 4:0
            break;
        case 0x03: // Request Sense(6)
            cbw->CBWSignature          = CBWMagic;      // magic
            cbw->CBWTag                = 0x42424203;    // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWDataTransferLength = 18;
            cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
            cbw->CBWCBLength           = 6;             // only bits 4:0
            cbw->commandByte[0]        = 0x03;          // Operation code
            cbw->commandByte[4]        = 18;            // Allocation length (max. bytes)
            break;
        case 0x12: // Inquiry(6)
            cbw->CBWSignature          = CBWMagic;      // magic
            cbw->CBWTag                = 0x42424212;    // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWDataTransferLength = 36;
            cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
            cbw->CBWCBLength           = 6;             // only bits 4:0
            cbw->commandByte[0]        = 0x12;          // Operation code
            cbw->commandByte[4]        = 36;            // Allocation length (max. bytes)
            break;
        case 0x25: // read capacity(10)
            cbw->CBWSignature          = CBWMagic;      // magic
            cbw->CBWTag                = 0x42424225;    // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWDataTransferLength = 8;
            cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
            cbw->CBWCBLength           = 10;            // only bits 4:0
            cbw->commandByte[0]        = 0x25;          // Operation code
            cbw->commandByte[2]        = BYTE4(LBA);    // LBA MSB
            cbw->commandByte[3]        = BYTE3(LBA);    // LBA
            cbw->commandByte[4]        = BYTE2(LBA);    // LBA
            cbw->commandByte[5]        = BYTE1(LBA);    // LBA LSB
            break;
        case 0x28: // read(10)
            cbw->CBWSignature           = CBWMagic;              // magic
            cbw->CBWTag                 = 0x42424228;            // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWDataTransferLength  = TransferLength*512;    // byte = 512 * block
            cbw->CBWFlags               = 0x80;                  // Out: 0x00  In: 0x80
            cbw->CBWCBLength            = 10;                    // only bits 4:0
            cbw->commandByte[0]         = 0x28;                  // Operation code
            cbw->commandByte[2]         = BYTE4(LBA);            // LBA MSB
            cbw->commandByte[3]         = BYTE3(LBA);            // LBA
            cbw->commandByte[4]         = BYTE2(LBA);            // LBA
            cbw->commandByte[5]         = BYTE1(LBA);            // LBA LSB
            cbw->commandByte[7]         = BYTE2(TransferLength); // MSB <--- blocks not byte!
            cbw->commandByte[8]         = BYTE1(TransferLength); // LSB
            break;
         case 0x2A: // write(10)
            cbw->CBWSignature           = CBWMagic;              // magic
            cbw->CBWTag                 = 0x4242422A;            // device echoes this field in the CSWTag field of the associated CSW
            cbw->CBWDataTransferLength  = TransferLength*512;    // byte = 512 * block
            cbw->CBWFlags               = 0x00;                  // Out: 0x00  In: 0x80
            cbw->CBWCBLength            = 10;                    // only bits 4:0
            cbw->commandByte[0]         = 0x2A;                  // Operation code
            cbw->commandByte[2]         = BYTE4(LBA);            // LBA MSB
            cbw->commandByte[3]         = BYTE3(LBA);            // LBA
            cbw->commandByte[4]         = BYTE2(LBA);            // LBA
            cbw->commandByte[5]         = BYTE1(LBA);            // LBA LSB
            cbw->commandByte[7]         = BYTE2(TransferLength); // MSB <--- blocks not byte!
            cbw->commandByte[8]         = BYTE1(TransferLength); // LSB
            break;
    }

    currCSWtag = SCSIcommand;
}

static int32_t checkSCSICommandUSBTransfer(uint32_t device, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer)
{
    // CSW Status
    printf("\n");
    showPacket(MSDStatusQTDpage0,13);

    // check signature 0x53425355 // DWORD 0 (byte 0:3)
    uint32_t CSWsignature = *(uint32_t*)MSDStatusQTDpage0; // DWORD 0
    if (CSWsignature == CSWMagicOK)
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nCSW signature OK    ");
      #endif
    }
    else if (CSWsignature == CSWMagicNotOK)
    {
        textColor(ERROR);
        printf("\nCSW signature wrong (not processed)");
        textColor(TEXT);
        return -1;
    }
    else
    {
        textColor(ERROR);
        printf("\nCSW signature wrong (processed, but wrong value)");
    }
    textColor(TEXT);

    // check matching tag
    uint32_t CSWtag = *(((uint32_t*)MSDStatusQTDpage0)+1); // DWORD 1 (byte 4:7)
    if ((BYTE1(CSWtag) == currCSWtag) && (BYTE2(CSWtag) == 0x42) && (BYTE3(CSWtag) == 0x42) && (BYTE4(CSWtag) == 0x42))
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(SUCCESS);
        printf("CSW tag %yh OK    ",BYTE1(CSWtag));
      #endif
    }
    else
    {
        textColor(ERROR);
        printf("\nError: CSW tag wrong");
    }
    textColor(TEXT);

    // check CSWDataResidue
    uint32_t CSWDataResidue = *(((uint32_t*)MSDStatusQTDpage0)+2); // DWORD 2 (byte 8:11)
    if (CSWDataResidue == 0)
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\tCSW data residue OK    ");
      #endif
    }
    else
    {
        textColor(0x06);
        printf("\nCSW data residue: %d",CSWDataResidue);
    }
    textColor(TEXT);

    // check status byte // DWORD 3 (byte 12)
    uint8_t CSWstatusByte = *(((uint8_t*)MSDStatusQTDpage0)+12); // byte 12 (last byte of 13 bytes)

    textColor(ERROR);
    switch (CSWstatusByte)
    {
        case 0x00:
          #ifdef _USB_DIAGNOSIS_
            textColor(SUCCESS);
            printf("\tCSW status OK");
          #endif
            break;
        case 0x01:
            printf("\nCommand failed");
            break;
        case 0x02:
            printf("\nPhase Error");
            textColor(IMPORTANT);
            printf("\nReset recovery is needed");
            usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
            break;
        default:
            printf("\nCSW status byte: undefined value (error)");
            break;
    }
    textColor(TEXT);

    // transfer diagnosis (qTD status)
    uint32_t statusCommand = showStatusbyteQTD(cmdQTD);
    if (statusCommand)
    {
        printf("<-- command"); // In/Out Data
        bulkTransfer->successfulCommand = (statusCommand == 0x01); // Do Ping
    }
    else // OK
    {
        bulkTransfer->successfulCommand = true;
    }

    uint32_t statusData = 0x00;
    if (TransferLength)
    {
        statusData = showStatusbyteQTD(DataQTD);
        if (statusData)
        {
            printf("<-- data");   // In/Out Data
        }
        else // OK
        {
            if (bulkTransfer->SCSIopcode == 0x2A) // write(10)
            {
                bulkTransfer->successfulDataOUT = true; // Out Data
            }
            else
            {
                bulkTransfer->successfulDataIN  = true; // In  Data
            }
        }
    }

    uint32_t statusStatus = showStatusbyteQTD(StatusQTD);
    if (statusStatus)
    {
        printf("<-- status");   // In CSW
    }
    else // OK
    {
        bulkTransfer->successfulCSW = true;
    }

    if ((statusCommand & 0x40) || (statusData & 0x40) || (statusStatus & 0x40))
    {
        textColor(ERROR);
        if (statusCommand & 0x40) { printf("\nCommand phase: HALT"); }
        if (statusData    & 0x40) { printf("\nData    phase: HALT"); }
        if (statusStatus  & 0x40) { printf("\nStatus  phase: HALT"); }
        textColor(IMPORTANT);
        printf("\nReset recovery is needed");
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    }
    return 0;
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(uint32_t device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer)
{
  #ifdef _USB_DIAGNOSIS_
    printf("\nOUT part");
  #endif

    // Two QHs: one for OUT and one for IN are established
    void* QH_Out = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "scsiIN-QH_Out");
    void* QH_In  = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "scsiIN-QH_In");

    // async list points to QH Out
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH_Out);

  #ifdef _USB_DIAGNOSIS_
    printf("\nasyncList: %Xh <-- QH_Out", pOpRegs->ASYNCLISTADDR);

    // OUT qTD
    // No handshake!

     textColor(0x03);
     printf("\ntoggle OUT %u", usbDevices[device].ToggleEndpointOutMSD);
     textColor(TEXT);
  #endif

    // The qTD for the SCSI command is built
    cmdQTD = createQTD_IO(0x01, OUT, usbDevices[device].ToggleEndpointOutMSD, 31);        // OUT DATA0 or DATA1, 31 byte
    usbDevices[device].ToggleEndpointOutMSD = !(usbDevices[device].ToggleEndpointOutMSD); // switch toggle

  #ifdef _USB_DIAGNOSIS_
    printf("\tCommandQTD: %Xh",paging_getPhysAddr((void*)cmdQTD));
  #endif

    // implement cbw at DataQTDpage0
    // http://en.wikipedia.org/wiki/SCSI_CDB
    struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
    memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw
    SCSIcmd(SCSIcommand, cbw, LBA, TransferLength);
    if (SCSIcommand == 0x28 || SCSIcommand == 0x2A)   // read(10) and write(10)
    {
        TransferLength *= 512; // byte = 512 * block
    }

    // QH Out with command qTD
    createQH(QH_Out, paging_getPhysAddr(QH_Out), cmdQTD,  1, device+1, endpointOut, 512); // endpoint OUT for MSD

    // Bulk Transfer to endpoint OUT
    performAsyncScheduler(true, true, 0);

    free(QH_Out);
    free(globalqTD[0]);
    free(globalqTDbuffer[0]);

  /**************************************************************************************************************************************/

  #ifdef _USB_DIAGNOSIS_
    printf("\nIN part");
  #endif

    // async list points to QH In
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH_In);

  #ifdef _USB_DIAGNOSIS_
    printf("\nasyncList: %Xh <-- QH_In", pOpRegs->ASYNCLISTADDR);
  #endif

    // IN qTDs
    // No handshake!
    void* QTD_In;
    void* next = 0;

    if (TransferLength > 0)
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(0x03);
        printf("\ntoggles IN: data: %u  status: %u", usbDevices[device].ToggleEndpointInMSD, !(usbDevices[device].ToggleEndpointInMSD));
        textColor(TEXT);
      #endif

        // Data in and Status qTD
        next = StatusQTD = createQTD_MSDStatus(0x1, !(usbDevices[device].ToggleEndpointInMSD));   // next, toggle, IN 13 byte
        globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
        QTD_In = DataQTD = createQTD_IO((uintptr_t)next, IN, usbDevices[device].ToggleEndpointInMSD, TransferLength); // IN/OUT DATA0, ... byte
        /*do not switch toggle*/

      #ifdef _USB_DIAGNOSIS_
        printf("\tStatusQTD: %Xh", paging_getPhysAddr((void*)StatusQTD));
        printf("\tDataQTD: %Xh",   paging_getPhysAddr((void*)DataQTD));
      #endif
    }
    else
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(0x03);
        printf("\ntoggle IN: status: %u", usbDevices[device].ToggleEndpointInMSD);
        textColor(TEXT);
      #endif

        QTD_In = StatusQTD = createQTD_MSDStatus(0x1, usbDevices[device].ToggleEndpointInMSD); // next, toggle, IN 13 byte
        usbDevices[device].ToggleEndpointInMSD = !(usbDevices[device].ToggleEndpointInMSD);    // switch toggle

      #ifdef _USB_DIAGNOSIS_
        printf("\tStatusQTD: %Xh", paging_getPhysAddr((void*)StatusQTD));
      #endif
    }


    // QH IN with data and status qTD
    createQH(QH_In, paging_getPhysAddr(QH_In), QTD_In, 1, device+1, endpointIn, 512); // endpoint IN for MSD

labelTransferIN: /// TEST

    performAsyncScheduler(true, true, TransferLength/200);

    if (TransferLength) // byte
    {
        printf("\n");
        showPacket(DataQTDpage0,TransferLength);
        if ((TransferLength==512) || (TransferLength==36)) // data block (512 byte), inquiry feedback (36 byte)
        {
            showPacketAlphaNumeric(DataQTDpage0,TransferLength);
        }
    }

    uint8_t index = TransferLength > 0 ? 1 : 0;

    if (checkSCSICommandUSBTransfer(device, TransferLength, bulkTransfer) == -1)
    {
        numberTries--;
        if (numberTries >= 0)
        {
            printf("\nIN-Transfer will be repeated: %u left",numberTries);
            goto labelTransferIN;
        }
    }

    free(QH_In);
    for (uint8_t i=0; i<=index; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

void usbSendSCSIcmdOUT(uint32_t device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer, uint8_t* buffer)
{
  #ifdef _USB_DIAGNOSIS_
    printf("\nOUT part");
  #endif

    // Two QHs: one for OUT and one for IN are established
    void* QH_Out = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "scsiOUT-QH_Out");
    void* QH_In  = malloc(sizeof(ehci_qhd_t), ALIGNVALUE, "scsiOUT-QH_In");

    // async list points to QH Out
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH_Out);

  #ifdef _USB_DIAGNOSIS_
    printf("\nasyncList: %Xh <-- QH_Out", pOpRegs->ASYNCLISTADDR);

    // OUT qTD
    // No handshake!

     textColor(0x03);
     printf("\ntoggle OUT %u", usbDevices[device].ToggleEndpointOutMSD);
     textColor(TEXT);
  #endif

    if (SCSIcommand == 0x2A)   // write(10)
    {
        TransferLength *= 512; // byte = 512 * block
    }

    // The qTD for the SCSI command and data out is built
    void* next = DataQTD = createQTD_IO_OUT(0x1, OUT, !(usbDevices[device].ToggleEndpointOutMSD), TransferLength, buffer); // IN/OUT DATA0, ... byte
    globalqTD[1] = globalqTD[0]; globalqTDbuffer[1] = globalqTDbuffer[0]; // save pointers for later free(pointer)
    cmdQTD               = createQTD_IO((uintptr_t)next, OUT, usbDevices[device].ToggleEndpointOutMSD, 31);                // OUT DATA0 or DATA1, 31 byte
    /*do not switch toggle*/

    // implement cbw at DataQTDpage0
    // http://en.wikipedia.org/wiki/SCSI_CDB
    struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
    memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw
    if (SCSIcommand == 0x2A)   // write(10)
    {
        TransferLength /= 512; // block = byte/512
    }
    SCSIcmd(SCSIcommand, cbw, LBA, TransferLength);    // block

  #ifdef _USB_DIAGNOSIS_
    printf("\tCommandQTD: %Xh",paging_getPhysAddr((void*)cmdQTD));
  #endif

    // QH Out with command qTD and data out qTD
    createQH(QH_Out, paging_getPhysAddr(QH_Out), cmdQTD,  1, device+1, endpointOut, 512); // endpoint OUT for MSD

    // Bulk Transfer to endpoint OUT
    performAsyncScheduler(true, true, TransferLength/200);

    free(QH_Out);
    for (uint8_t i=0; i<=1; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }

  /**************************************************************************************************************************************/

  #ifdef _USB_DIAGNOSIS_
    printf("\nIN part");
  #endif

    // async list points to QH In
    pOpRegs->ASYNCLISTADDR = paging_getPhysAddr(QH_In);

  #ifdef _USB_DIAGNOSIS_
    printf("\nasyncList: %Xh <-- QH_In", pOpRegs->ASYNCLISTADDR);

    // IN qTDs
    // No handshake!

    textColor(0x03);
    printf("\ntoggle IN: status: %u", usbDevices[device].ToggleEndpointInMSD);
    textColor(TEXT);
  #endif

    StatusQTD = createQTD_MSDStatus(0x1, usbDevices[device].ToggleEndpointInMSD); // next, toggle, IN 13 byte
    usbDevices[device].ToggleEndpointInMSD = !(usbDevices[device].ToggleEndpointInMSD);    // switch toggle

  #ifdef _USB_DIAGNOSIS_
    printf("\tStatusQTD: %Xh", paging_getPhysAddr((void*)StatusQTD));
  #endif

    // QH IN with status qTD only
    createQH(QH_In, paging_getPhysAddr(QH_In), StatusQTD, 1, device+1, endpointIn, 512); // endpoint IN for MSD
    performAsyncScheduler(true, true, 0);
    checkSCSICommandUSBTransfer(device, TransferLength, bulkTransfer);

    free(QH_In);
    for (uint8_t i=0; i<=0; i++)
    {
        free(globalqTD[i]);
        free(globalqTDbuffer[i]);
    }
}

static uint8_t getStatusByte()
{
    return BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));
}

static uint8_t testDeviceReady(uint8_t devAddr, usbBulkTransfer_t* bulkTransferTestUnitReady, usbBulkTransfer_t* bulkTransferRequestSense)
{
    const uint8_t maxTest = 3;
    int32_t timeout = maxTest;
    int32_t sense = -1;
    uint8_t statusByte;

    while (true)
    {
        timeout--;
      #ifdef _USB_DIAGNOSIS_
        textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: test unit ready"); textColor(TEXT);
      #endif

        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0, bulkTransferTestUnitReady); // dev, endp, cmd, LBA, transfer length

        uint8_t statusByteTestReady = getStatusByte();
        showUSBSTS();

        if (timeout != maxTest-1)
        {
            ///////// send SCSI command "request sense"

          #ifdef _USB_DIAGNOSIS_
            textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: request sense"); textColor(TEXT);
          #endif

            usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18, bulkTransferRequestSense); // dev, endp, cmd, LBA, transfer length

            statusByte = getStatusByte();
            showUSBSTS();

            sense = showResultsRequestSense();
            if ( ((statusByteTestReady == 0) && ((sense == 0) || (sense == 6))) || (timeout <= 0) )
            {
                break;
            }
        }
    }
    waitForKeyStroke();

    return statusByte;
}

static void startLogBulkTransfer(usbBulkTransfer_t* pTransferLog, uint8_t SCSIopcode, uint32_t DataBytesToTransferIN, uint32_t DataBytesToTransferOUT)
{
    pTransferLog->SCSIopcode             = SCSIopcode;
    pTransferLog->successfulCommand      = false;
    pTransferLog->successfulDataOUT      = false;
    pTransferLog->successfulDataIN       = false;
    pTransferLog->successfulCSW          = false;
    pTransferLog->DataBytesToTransferIN  = DataBytesToTransferIN;
    pTransferLog->DataBytesToTransferOUT = DataBytesToTransferOUT;
}

// http://en.wikipedia.org/wiki/SCSI_Inquiry_Command
static void analyzeInquiry()
{
    void* addr = (void*)DataQTDpage0;
    // cf. Jan Axelson, USB Mass Storage, page 140
    uint8_t PeripheralDeviceType = getField(addr, 0, 0, 5); // byte, shift, len
 // uint8_t PeripheralQualifier  = getField(addr, 0, 5, 3);
 // uint8_t DeviceTypeModifier   = getField(addr, 1, 0, 7);
    uint8_t RMB                  = getField(addr, 1, 7, 1);
  #ifdef _USB_DIAGNOSIS_
    uint8_t ANSIapprovedVersion  = getField(addr, 2, 0, 3);
  #endif
 // uint8_t ECMAversion          = getField(addr, 2, 3, 3);
 // uint8_t ISOversion           = getField(addr, 2, 6, 2);
    uint8_t ResponseDataFormat   = getField(addr, 3, 0, 4);
    uint8_t HISUP                = getField(addr, 3, 4, 1);
    uint8_t NORMACA              = getField(addr, 3, 5, 1);
 // uint8_t AdditionalLength     = getField(addr, 4, 0, 8);
    uint8_t CmdQue               = getField(addr, 7, 1, 1);
    uint8_t Linked               = getField(addr, 7, 3, 1);

    char vendorID[9];
    for (uint8_t i=0; i<8;i++)
    {
        vendorID[i]= getField(addr, i+8, 0, 8);
    }
    vendorID[8]=0;

    char productID[17];
    for (uint8_t i=0; i<16;i++)
    {
        productID[i]= getField(addr, i+16, 0, 8);
    }
    productID[16]=0;

  #ifdef _USB_DIAGNOSIS_
    char productRevisionLevel[5];
    for (uint8_t i=0; i<4;i++)
    {
        productRevisionLevel[i]= getField(addr, i+32, 0, 8);
    }
    productRevisionLevel[4]=0;
  #endif

    printf("\nVendor ID:  %s", vendorID);
    printf("\nProduct ID: %s", productID);

  #ifdef _USB_DIAGNOSIS_
    printf("\nRevision:   %s", productRevisionLevel);

    // Book of Jan Axelson, "USB Mass Storage", page 140:
    // printf("\nVersion ANSI: %u  ECMA: %u  ISO: %u", ANSIapprovedVersion, ECMAversion, ISOversion);
    printf("\nVersion: %u (4: SPC-2, 5: SPC-3)", ANSIapprovedVersion);
  #endif

    // Jan Axelson, USB Mass Storage, page 140
    if (ResponseDataFormat == 2)
    {
        textColor(SUCCESS);
        printf("\nResponse Data Format OK");
    }
    else
    {
        textColor(ERROR);
        printf("\nResponse Data Format is not OK: %u (should be 2)", ResponseDataFormat);
    }
    textColor(TEXT);

    printf("\nRemovable device type:            %s", RMB     ? "yes" : "no");
    printf("\nSupports hierarch. addr. support: %s", HISUP   ? "yes" : "no");
    printf("\nSupports normal ACA bit support:  %s", NORMACA ? "yes" : "no");
    printf("\nSupports linked commands:         %s", Linked  ? "yes" : "no");
    printf("\nSupports tagged command queuing:  %s", CmdQue  ? "yes" : "no");

    switch (PeripheralDeviceType)
    {
        case 0x00: printf("\ndirect-access device (e.g., magnetic disk)");            break;
        case 0x01: printf("\nsequential-access device (e.g., magnetic tape)");        break;
        case 0x02: printf("\nprinter device");                                        break;
        case 0x03: printf("\nprocessor device");                                      break;
        case 0x04: printf("\nwrite-once device");                                     break;
        case 0x05: printf("\nCD/DVD device");                                         break;
        case 0x06: printf("\nscanner device");                                        break;
        case 0x07: printf("\noptical memory device (non-CD optical disk)");           break;
        case 0x08: printf("\nmedium Changer (e.g. jukeboxes)");                       break;
        case 0x09: printf("\ncommunications device");                                 break;
        case 0x0A: printf("\ndefined by ASC IT8 (Graphic arts pre-press devices)");   break;
        case 0x0B: printf("\ndefined by ASC IT8 (Graphic arts pre-press devices)");   break;
        case 0x0C: printf("\nStorage array controller device (e.g., RAID)");          break;
        case 0x0D: printf("\nEnclosure services device");                             break;
        case 0x0E: printf("\nSimplified direct-access device (e.g., magnetic disk)"); break;
        case 0x0F: printf("\nOptical card reader/writer device");                     break;
        case 0x10: printf("\nReserved for bridging expanders");                       break;
        case 0x11: printf("\nObject-based Storage Device");                           break;
        case 0x12: printf("\nAutomation/Drive Interface");                            break;
        case 0x13: printf("\nReserved");                                              break;
        case 0x1D: printf("\nReserved");                                              break;
        case 0x1E: printf("\nReduced block command (RBC) direct-access device");      break;
        case 0x1F: printf("\nUnknown or no device type");                             break;
    }
}

void testMSD(uint8_t devAddr, disk_t* disk)
{
    if (usbDevices[devAddr].InterfaceClass != 0x08)
    {
        textColor(ERROR);
        printf("\nThis is no Mass Storage Device! MSD test cannot be carried out.");
        textColor(TEXT);
    }
    else
    {
        currentDevice = devAddr; // now active usb msd

        // maxLUN (0 for USB-sticks)
        usbDevices[devAddr].maxLUN = 0;

        // start with correct endpoint toggles and reset interface
        usbDevices[devAddr].ToggleEndpointInMSD = usbDevices[devAddr].ToggleEndpointOutMSD = 0;
        usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface

        ///////// send SCSI command "inquiry (opcode: 0x12)"
      #ifdef _USB_DIAGNOSIS_
        textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: inquiry"); textColor(TEXT);
      #endif
        usbBulkTransfer_t inquiry;
        startLogBulkTransfer(&inquiry, 0x12, 36, 0);

        usbSendSCSIcmd(devAddr,
                       usbDevices[devAddr].numInterfaceMSD,
                       usbDevices[devAddr].numEndpointOutMSD,
                       usbDevices[devAddr].numEndpointInMSD,
                       inquiry.SCSIopcode,
                       0, // LBA
                       inquiry.DataBytesToTransferIN,
                       &inquiry);

        analyzeInquiry();
        showUSBSTS();
        logBulkTransfer(&inquiry);

        ///////// send SCSI command "test unit ready(6)"

        usbBulkTransfer_t testUnitReady, requestSense;
        startLogBulkTransfer(&testUnitReady, 0x00,  0, 0);
        startLogBulkTransfer(&requestSense,  0x03, 18, 0);
        testDeviceReady(devAddr, &testUnitReady, &requestSense);
        logBulkTransfer(&testUnitReady);
        logBulkTransfer(&requestSense);

        ///////// send SCSI command "read capacity(10)"
      #ifdef _USB_DIAGNOSIS_
        textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: read capacity"); textColor(TEXT);
      #endif

        usbBulkTransfer_t readCapacity;
        startLogBulkTransfer(&readCapacity, 0x25, 8, 0);

        usbSendSCSIcmd(devAddr,
                       usbDevices[devAddr].numInterfaceMSD,
                       usbDevices[devAddr].numEndpointOutMSD,
                       usbDevices[devAddr].numEndpointInMSD,
                       readCapacity.SCSIopcode,
                       0, // LBA
                       readCapacity.DataBytesToTransferIN,
                       &readCapacity);

        uint32_t lastLBA    = (*((uint8_t*)DataQTDpage0+0)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+1)) * 0x10000 + (*((uint8_t*)DataQTDpage0+2)) * 0x100 + (*((uint8_t*)DataQTDpage0+3));
        uint32_t blocksize  = (*((uint8_t*)DataQTDpage0+4)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+5)) * 0x10000 + (*((uint8_t*)DataQTDpage0+6)) * 0x100 + (*((uint8_t*)DataQTDpage0+7));
        uint32_t capacityMB = ((lastLBA+1)/1000000) * blocksize;

        usbMSDVolumeMaxLBA = lastLBA;

        textColor(IMPORTANT);
        printf("\nCapacity: %u MB, Last LBA: %u, block size %u\n", capacityMB, lastLBA, blocksize);
        textColor(TEXT);

        showUSBSTS();
        logBulkTransfer(&readCapacity);

        analyzeDisk(disk);
    } // else
}

FS_ERROR usbRead(uint32_t sector, void* buffer, void* device)
{
    ///////// send SCSI command "read(10)", read one block from LBA ..., get Status
  #ifdef _USB_DIAGNOSIS_
    textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: read   sector: %u", sector); textColor(TEXT);
  #endif

    uint8_t           devAddr = currentDevice;
    uint32_t          blocks  = 1; // number of blocks to be read
    usbBulkTransfer_t read;

    startLogBulkTransfer(&read, 0x28, blocks, 0);

    usbSendSCSIcmd(devAddr,
                   usbDevices[devAddr].numInterfaceMSD,
                   usbDevices[devAddr].numEndpointOutMSD,
                   usbDevices[devAddr].numEndpointInMSD,
                   read.SCSIopcode,
                   sector, // LBA
                   read.DataBytesToTransferIN,
                   &read);

    memcpy((void*)buffer,(void*)DataQTDpage0,512);
    showUSBSTS();
    logBulkTransfer(&read);

    return(CE_GOOD); // SUCCESS // TEST
}

FS_ERROR usbWrite(uint32_t sector, void* buffer, void* device)
{
        ///////// send SCSI command "write(10)", write one block to LBA ..., get Status

    textColor(IMPORTANT); printf("\n\n>>> SCSI: write  sector: %u", sector); textColor(TEXT);

    uint8_t           devAddr = currentDevice;
    uint32_t          blocks  = 1; // number of blocks to be written
    usbBulkTransfer_t write;

    startLogBulkTransfer(&write, 0x2A, 0, blocks);

    usbSendSCSIcmdOUT(devAddr,
                   usbDevices[devAddr].numInterfaceMSD,
                   usbDevices[devAddr].numEndpointOutMSD,
                   usbDevices[devAddr].numEndpointInMSD,
                   write.SCSIopcode,
                   sector, // LBA
                   write.DataBytesToTransferOUT,
                   &write, buffer);

    showUSBSTS();
    logBulkTransfer(&write);

    return(CE_GOOD);
}

void usbResetRecoveryMSD(uint32_t device, uint32_t Interface, uint32_t endpointOUT, uint32_t endpointIN)
{
    // Reset Interface
    usbTransferBulkOnlyMassStorageReset(device, Interface);

    // TEST ////////////////////////////////////
    //usbSetFeatureHALT(device, endpointIN,  512);
    //usbSetFeatureHALT(device, endpointOUT, 512);

    // Clear Feature HALT to the Bulk-In  endpoint
    printf("\nGetStatus: %u", usbGetStatus(device, endpointIN, 512));
    usbClearFeatureHALT(device, endpointIN,  512);
    printf("\nGetStatus: %u", usbGetStatus(device, endpointIN, 512));

    // Clear Feature HALT to the Bulk-Out endpoint
    printf("\nGetStatus: %u", usbGetStatus(device, endpointOUT, 512));
    usbClearFeatureHALT(device, endpointOUT, 512);
    printf("\nGetStatus: %u", usbGetStatus(device, endpointOUT, 512));

    // set configuration to 1 and endpoint IN/OUT toggles to 0
    textColor(SUCCESS);
    usbTransferSetConfiguration(device, 1); // set first configuration
    uint8_t config = usbTransferGetConfiguration(device);
    if (config != 1)
    {
        textColor(ERROR);
        printf("\tconfiguration: %u (to be: 1)",config);
        textColor(TEXT);
    }

    // start with correct endpoint toggles and reset interface
    usbDevices[device].ToggleEndpointInMSD = usbDevices[device].ToggleEndpointOutMSD = 0;
    usbTransferBulkOnlyMassStorageReset(device, usbDevices[device].numInterfaceMSD); // Reset Interface
    textColor(TEXT);
}

int32_t showResultsRequestSense()
{
    void* addr = (void*)DataQTDpage0;

    uint32_t Valid        = getField(addr, 0, 7, 1); // byte 0, bit 7
    uint32_t ResponseCode = getField(addr, 0, 0, 7); // byte 0, bit 6:0
    uint32_t SenseKey     = getField(addr, 2, 0, 4); // byte 2, bit 3:0

    textColor(IMPORTANT);
    printf("\n\nResults of \"request sense\":\n");
    if (ResponseCode >= 0x70 && ResponseCode <= 0x73)
    {
        textColor(TEXT);
        printf("Valid: \t\t");
        if (Valid == 0)
        {
            printf("Sense data are not SCSI compliant");
        }
        else
        {
            printf("Sense data are SCSI compliant");
        }
        printf("\nResponse Code: \t");
        switch (ResponseCode)
        {
            case 0x70:
                printf("Current errors, fixed format");
                break;
            case 0x71:
                printf("Deferred errors, fixed format");
                break;
            case 0x72:
                printf("Current error, descriptor format");
                break;
            case 0x73:
                printf("Deferred error, descriptor format");
                break;
            default:
                printf("No vaild response code!");
                break;
        }
        printf("\nSense Key: \t");
        switch (SenseKey)
        {
            case 0x0:
                printf("No Sense");
                break;
            case 0x1:
                printf("Recovered Error - last command completed with some recovery action");
                break;
            case 0x2:
                printf("Not Ready - logical unit addressed cannot be accessed");
                break;
            case 0x3:
                printf("Medium Error - command terminated with a non-recovered error condition");
                break;
            case 0x4:
                printf("Hardware Error");
                break;
            case 0x5:
                printf("Illegal Request - illegal parameter in the command descriptor block ");
                break;
            case 0x6:
                printf("Unit Attention - disc drive may have been reset.");
                break;
            case 0x7:
                printf("Data Protect - command read/write on a protected block");
                break;
            case 0x8:
                printf("not defined");
                break;
            case 0x9:
                printf("Firmware Error");
                break;
            case 0xA:
                printf("not defined");
                break;
            case 0xB:
                printf("Aborted Command - disc drive aborted the command");
                break;
            case 0xC:
                printf("Equal - SEARCH DATA command has satisfied an equal comparison");
                break;
            case 0xD:
                printf("Volume Overflow - buffered peripheral device has reached the end of medium partition");
                break;
            case 0xE:
                printf("Miscompare - source data did not match the data read from the medium");
                break;
            case 0xF:
                printf("not defined");
                break;
            default:
                printf("sense key not known!");
                break;
        }
        return SenseKey;
    }

    textColor(ERROR);
    printf("No vaild response code!");
    textColor(TEXT);
    return -1;
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
