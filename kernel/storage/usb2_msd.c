/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "usb_hc.h"
#include "usb2_msd.h"
#include "paging.h"
#include "kheap.h"
#include "video/console.h"
#include "util.h"


static const uint32_t CSWMagicNotOK = 0x01010101;
static const uint32_t CSWMagicOK    = 0x53425355; // USBS
static const uint32_t CBWMagic      = 0x43425355; // USBC

static uint8_t currCSWtag;

uint32_t usbMSDVolumeMaxLBA;


usb2_Device_t* usb2_createDevice(disk_t* disk)
{
    usb2_Device_t* device = malloc(sizeof(usb2_Device_t), 0, "usb2_Device_t");
    device->disk = disk;
    device->mps = malloc(sizeof(*device->mps)*3, 0, "usbDev->mps"); // Assume that we have three endpoints (only interface 1)
    device->mps[0] = 64;
    disk->data = device;
    return(device);
}

void usb2_destroyDevice(usb2_Device_t* device)
{
    device->disk->data = 0;
    free(device);
}

// Bulk-Only Mass Storage get maximum number of Logical Units
uint8_t usbTransferBulkOnlyGetMaxLUN(usb2_Device_t* device, uint8_t numInterface)
{
  #ifdef _USB2_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nUSB2: usbTransferBulkOnlyGetMaxLUN, dev: %X interface: %u", device, numInterface);
    textColor(TEXT);
  #endif

    uint8_t maxLUN;

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);

    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 10100001b     11111110b 0000h  Interface 0001h     1 byte
    usb_setupTransaction(&transfer, 0, 8, 0xA1, 0xFE, 0, 0, numInterface, 1);
    usb_inTransaction(&transfer, 1, &maxLUN, 1);
    usb_outTransaction(&transfer, 1, 0, 0); // handshake

    usb_issueTransfer(&transfer);

    return maxLUN;
}

// Bulk-Only Mass Storage Reset
void usbTransferBulkOnlyMassStorageReset(usb2_Device_t* device, uint8_t numInterface)
{
  #ifdef _USB2_DIAGNOSIS_
    textColor(LIGHT_CYAN);
    printf("\nUSB2: usbTransferBulkOnlyMassStorageReset, dev: %X interface: %u", device, numInterface);
    textColor(TEXT);
  #endif

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_CONTROL, 0, 64);

    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 00100001b     11111111b 0000h  Interface 0000h     none
    usb_setupTransaction(&transfer, 0, 8, 0x21, 0xFF, 0, 0, numInterface, 0);
    usb_inTransaction(&transfer, 1, 0, 0); // handshake

    usb_issueTransfer(&transfer);
}

void SCSIcmd(uint8_t SCSIcommand, struct usb2_CommandBlockWrapper* cbw, uint32_t LBA, uint16_t TransferLength)
{
    memset(cbw, 0, sizeof(struct usb2_CommandBlockWrapper));
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

static int checkSCSICommandUSBTransfer(void* MSDStatus, usb2_Device_t* device, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer)
{
    // CSW Status
  #ifdef _USB2_DIAGNOSIS_
    putch('\n');
    memshow(MSDStatus,13, false);
    putch('\n');
  #endif

    int error = 0;

    // check signature 0x53425355 // DWORD 0 (byte 0:3)
    uint32_t CSWsignature = *(uint32_t*)MSDStatus; // DWORD 0
    if (CSWsignature == CSWMagicOK)
    {
      #ifdef _USB2_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\nCSW signature OK    ");
        textColor(TEXT);
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
        textColor(TEXT);
        error = -2;
    }

    // check matching tag
    uint32_t CSWtag = *(((uint32_t*)MSDStatus)+1); // DWORD 1 (byte 4:7)

    if ((BYTE1(CSWtag) == currCSWtag) && (BYTE2(CSWtag) == 0x42) && (BYTE3(CSWtag) == 0x42) && (BYTE4(CSWtag) == 0x42))
    {
      #ifdef _USB2_DIAGNOSIS_
        textColor(SUCCESS);
        printf("CSW tag %yh OK    ",BYTE1(CSWtag));
        textColor(TEXT);
      #endif
    }
    else
    {
        textColor(ERROR);
        printf("\nError: CSW tag wrong");
        textColor(TEXT);
        error = -3;
    }

    // check CSWDataResidue
    uint32_t CSWDataResidue = *(((uint32_t*)MSDStatus)+2); // DWORD 2 (byte 8:11)
    if (CSWDataResidue == 0)
    {
      #ifdef _USB2_DIAGNOSIS_
        textColor(SUCCESS);
        printf("\tCSW data residue OK    ");
        textColor(TEXT);
      #endif
    }
    else
    {
        textColor(0x06);
        printf("\nCSW data residue: %d",CSWDataResidue);
        textColor(TEXT);
    }

    // check status byte // DWORD 3 (byte 12)
    uint8_t CSWstatusByte = *(((uint8_t*)MSDStatus)+12); // byte 12 (last byte of 13 bytes)

    textColor(ERROR);
    switch (CSWstatusByte)
    {
        case 0x00:
          #ifdef _USB2_DIAGNOSIS_
            textColor(SUCCESS);
            printf("\tCSW status OK");
          #endif
            break;
        case 0x01:
            printf("\nCommand failed");
            error = -4;
            break;
        case 0x02:
            printf("\nPhase Error");
            textColor(IMPORTANT);
            printf("\nReset recovery is needed");
            usbResetRecoveryMSD(device, device->numInterfaceMSD, device->numEndpointOutMSD, device->numEndpointInMSD);
            error = -5;
            break;
        default:
            printf("\nCSW status byte: undefined value (error)");
            error = -6;
            break;
    }
    textColor(TEXT);

    return error;
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(usb2_Device_t* device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer, void* dataBuffer, void* statusBuffer)
{
  #ifdef _USB2_DIAGNOSIS_
    printf("\nOUT part");
    textColor(0x03);
    printf("\ntoggle OUT %u", device->ToggleEndpointOutMSD);
    textColor(TEXT);
  #endif

    struct usb2_CommandBlockWrapper cbw;
    SCSIcmd(SCSIcommand, &cbw, LBA, TransferLength);

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_BULK, endpointOut, 512);
    usb_outTransaction(&transfer, device->ToggleEndpointOutMSD, &cbw, 31);
    usb_issueTransfer(&transfer);

    device->ToggleEndpointOutMSD = !device->ToggleEndpointOutMSD; // switch toggle

    if (SCSIcommand == 0x28 || SCSIcommand == 0x2A)   // read(10) and write(10)
    {
        TransferLength *= 512; // byte = 512 * block
    }

  /**************************************************************************************************************************************/

  #ifdef _USB2_DIAGNOSIS_
    printf("\nIN part");
  #endif

    uint32_t timeout = 0;

    char tempStatusBuffer[13];
    if(statusBuffer == 0)
        statusBuffer = tempStatusBuffer;

    usb_setupTransfer(device->disk->port, &transfer, USB_BULK, endpointIn, 512);
    if (TransferLength > 0)
    {
        usb_inTransaction(&transfer, device->ToggleEndpointInMSD, dataBuffer, TransferLength);
        usb_inTransaction(&transfer, !device->ToggleEndpointInMSD, statusBuffer, 13);
    }
    else
    {
        usb_inTransaction(&transfer, device->ToggleEndpointInMSD, statusBuffer, 13);
        device->ToggleEndpointInMSD = !device->ToggleEndpointInMSD; // switch toggle
    }
    usb_issueTransfer(&transfer);

  #ifdef _USB2_DIAGNOSIS_
    if (TransferLength) // byte
    {
        putch('\n');
        memshow(dataBuffer, TransferLength, false);
        putch('\n');

        if ((TransferLength==512) || (TransferLength==36)) // data block (512 byte), inquiry feedback (36 byte)
        {
            memshow(dataBuffer, TransferLength, true); // alphanumeric
            putch('\n');
        }
    }
  #endif

    if(checkSCSICommandUSBTransfer(statusBuffer, device, TransferLength, bulkTransfer) != 0 && timeout < 5)
    {
        timeout++;
    }
    // TODO: Handle failure/timeout
}

void usbSendSCSIcmdOUT(usb2_Device_t* device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer, void* dataBuffer, void* statusBuffer)
{
  #ifdef _USB2_DIAGNOSIS_
    printf("\nOUT part");
    textColor(0x03);
    printf("\ntoggle OUT %u", device->ToggleEndpointOutMSD);
    textColor(TEXT);
  #endif

    struct usb2_CommandBlockWrapper cbw;
    SCSIcmd(SCSIcommand, &cbw, LBA, TransferLength);

    if (SCSIcommand == 0x2A)   // write(10)
    {
        TransferLength *= 512; // byte = 512 * block
    }

    usb_transfer_t transfer;
    usb_setupTransfer(device->disk->port, &transfer, USB_BULK, endpointOut, 512);
    usb_outTransaction(&transfer, device->ToggleEndpointOutMSD, &cbw, 31);
    usb_outTransaction(&transfer, !device->ToggleEndpointOutMSD, dataBuffer, TransferLength);
    usb_issueTransfer(&transfer);

  /**************************************************************************************************************************************/

  #ifdef _USB2_DIAGNOSIS_
    printf("\nIN part");
  #endif

    char tempStatusBuffer[13];
    if(statusBuffer == 0)
        statusBuffer = tempStatusBuffer;

    usb_setupTransfer(device->disk->port, &transfer, USB_BULK, endpointIn, 512);
    usb_inTransaction(&transfer, device->ToggleEndpointInMSD, statusBuffer, 13);
    usb_issueTransfer(&transfer);

    device->ToggleEndpointInMSD = !device->ToggleEndpointInMSD; // switch toggle
}

static uint8_t testDeviceReady(usb2_Device_t* device, usbBulkTransfer_t* bulkTransferTestUnitReady, usbBulkTransfer_t* bulkTransferRequestSense)
{
    const uint8_t maxTest = 5;
    uint32_t timeout = maxTest;
    uint8_t statusByte;

    while (timeout != 0)
    {
        timeout--;

        textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: test unit ready"); textColor(TEXT);

        char statusBuffer[13];
        usbSendSCSIcmd(device, device->numInterfaceMSD, device->numEndpointOutMSD, device->numEndpointInMSD, 0x00, 0, 0, bulkTransferTestUnitReady, 0, statusBuffer); // dev, endp, cmd, LBA, transfer length

        uint8_t statusByteTestReady = BYTE1(*(((uint32_t*)statusBuffer)+3));

        if(timeout >= maxTest/2 && statusByteTestReady != 0) continue;


        textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: request sense"); textColor(TEXT);

        char dataBuffer[18];
        usbSendSCSIcmd(device, device->numInterfaceMSD, device->numEndpointOutMSD, device->numEndpointInMSD, 0x03, 0, 18, bulkTransferRequestSense, dataBuffer, statusBuffer); // dev, endp, cmd, LBA, transfer length

        statusByte = BYTE1(*(((uint32_t*)statusBuffer)+3));

        int32_t sense = showResultsRequestSense(dataBuffer);
        if (sense == 0 || sense == 6)
        {
            break;
        }

    }

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
static void analyzeInquiry(void* addr)
{
    // cf. Jan Axelson, USB Mass Storage, page 140
    uint8_t PeripheralDeviceType = getField(addr, 0, 0, 5); // byte, shift, len
 // uint8_t PeripheralQualifier  = getField(addr, 0, 5, 3);
 // uint8_t DeviceTypeModifier   = getField(addr, 1, 0, 7);
    uint8_t RMB                  = getField(addr, 1, 7, 1);
  #ifdef _USB2_DIAGNOSIS_
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
    memcpy(vendorID, addr+8, 8);
    vendorID[8]=0;

    char productID[17];
    memcpy(productID, addr+16, 16);
    productID[16]=0;

  #ifdef _USB2_DIAGNOSIS_
    char productRevisionLevel[5];
    memcpy(productRevisionLevel, addr+32, 4);
    productRevisionLevel[4]=0;
  #endif

    printf("\nVendor ID:  %s", vendorID);
    printf("\nProduct ID: %s", productID);

  #ifdef _USB2_DIAGNOSIS_
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
        case 0x13:
        case 0x1D: printf("\nReserved");                                              break;
        case 0x1E: printf("\nReduced block command (RBC) direct-access device");      break;
        case 0x1F: printf("\nUnknown or no device type");                             break;
    }
}

static void logBulkTransfer(usbBulkTransfer_t* bT)
{
  #ifdef _USB2_DIAGNOSIS_
    if (!bT->successfulCommand ||
        !bT->successfulCSW     ||
        (bT->DataBytesToTransferOUT && !bT->successfulDataOUT) ||
        (bT->DataBytesToTransferIN  && !bT->successfulDataIN))
    {
        textColor(IMPORTANT);
        printf("\nopcode: %yh", bT->SCSIopcode);
        printf("  cmd: %s",    bT->successfulCommand ? "OK" : "Error");
        if (bT->DataBytesToTransferOUT)
        {
            printf("  data out: %s", bT->successfulDataOUT ? "OK" : "Error");
        }
        if (bT->DataBytesToTransferIN)
        {
            printf("  data in: %s", bT->successfulDataIN ? "OK" : "Error");
        }
        printf("  CSW: %s", bT->successfulCSW ? "OK" : "Error");
        textColor(TEXT);
    }
  #endif
}

void testMSD(usb2_Device_t* device)
{
    // maxLUN (0 for USB-sticks)
    device->maxLUN  = 0;

    // start with correct endpoint toggles and reset interface
    device->ToggleEndpointInMSD = device->ToggleEndpointOutMSD = 0;
    usbTransferBulkOnlyMassStorageReset(device, device->numInterfaceMSD); // Reset Interface

    ///////// send SCSI command "inquiry (opcode: 0x12)"
    usbBulkTransfer_t inquiry;
    startLogBulkTransfer(&inquiry, 0x12, 36, 0);
    char inquiryBuffer[36];
    usbSendSCSIcmd(device,
                   device->numInterfaceMSD,
                   device->numEndpointOutMSD,
                   device->numEndpointInMSD,
                   inquiry.SCSIopcode,
                   0, // LBA
                   inquiry.DataBytesToTransferIN,
                   &inquiry, inquiryBuffer, 0);

    analyzeInquiry(inquiryBuffer);
    logBulkTransfer(&inquiry);

    ///////// send SCSI command "test unit ready(6)"
    usbBulkTransfer_t testUnitReady, requestSense;
    startLogBulkTransfer(&testUnitReady, 0x00,  0, 0);
    startLogBulkTransfer(&requestSense,  0x03, 18, 0);
    testDeviceReady(device, &testUnitReady, &requestSense);
    logBulkTransfer(&testUnitReady);
    logBulkTransfer(&requestSense);


    ///////// send SCSI command "read capacity(10)"
    textColor(LIGHT_BLUE); printf("\n\n>>> SCSI: read capacity"); textColor(TEXT);
    usbBulkTransfer_t readCapacity;
    startLogBulkTransfer(&readCapacity, 0x25, 8, 0);
    char capacityBuffer[8];
    usbSendSCSIcmd(device,
                   device->numInterfaceMSD,
                   device->numEndpointOutMSD,
                   device->numEndpointInMSD,
                   readCapacity.SCSIopcode,
                   0, // LBA
                   readCapacity.DataBytesToTransferIN,
                   &readCapacity, capacityBuffer, 0);

    uint32_t lastLBA    = (*((uint8_t*)capacityBuffer+0)) * 0x1000000 + (*((uint8_t*)capacityBuffer+1)) * 0x10000 + (*((uint8_t*)capacityBuffer+2)) * 0x100 + (*((uint8_t*)capacityBuffer+3));
    uint32_t blocksize  = (*((uint8_t*)capacityBuffer+4)) * 0x1000000 + (*((uint8_t*)capacityBuffer+5)) * 0x10000 + (*((uint8_t*)capacityBuffer+6)) * 0x100 + (*((uint8_t*)capacityBuffer+7));
    uint32_t capacityMiB = ((lastLBA+1)/0x100000) * blocksize;

    usbMSDVolumeMaxLBA = lastLBA;

    textColor(IMPORTANT);
    printf("\nCapacity: %u MiB, Last LBA: %u, block size: %u\n", capacityMiB, lastLBA, blocksize);
    textColor(TEXT);

    logBulkTransfer(&readCapacity);

    analyzeDisk(device->disk);
}

FS_ERROR usbRead(uint32_t sector, void* buffer, void* dev)
{
  #ifdef _USB2_DIAGNOSIS_
    textColor(LIGHT_BLUE);
    printf("\n\n>>> SCSI: read   sector: %u", sector);
    textColor(TEXT);
  #endif

    usb2_Device_t* device = dev;

    uint32_t blocks = 1; // number of blocks to be read
    usbBulkTransfer_t read;

    startLogBulkTransfer(&read, 0x28, blocks, 0);

    usbSendSCSIcmd(device,
                    device->numInterfaceMSD,
                    device->numEndpointOutMSD,
                    device->numEndpointInMSD,
                    read.SCSIopcode,
                    sector, // LBA
                    read.DataBytesToTransferIN,
                    &read, buffer, 0);
    logBulkTransfer(&read);

    memshow(buffer,512,false);
    waitForKeyStroke();

    return(CE_GOOD);
}

FS_ERROR usbWrite(uint32_t sector, void* buffer, void* dev)
{
  #ifdef _USB2_DIAGNOSIS_
    textColor(IMPORTANT);
    printf("\n\n>>> SCSI: write  sector: %u", sector);
    textColor(TEXT);
  #endif

    usb2_Device_t* device = dev;

    uint32_t          blocks  = 1; // number of blocks to be written
    usbBulkTransfer_t write;

    startLogBulkTransfer(&write, 0x2A, 0, blocks);

    usbSendSCSIcmdOUT(device,
                        device->numInterfaceMSD,
                        device->numEndpointOutMSD,
                        device->numEndpointInMSD,
                        write.SCSIopcode,
                        sector, // LBA
                        write.DataBytesToTransferOUT,
                        &write, buffer, 0);

    logBulkTransfer(&write);

    return(CE_GOOD);
}

void usbResetRecoveryMSD(usb2_Device_t* device, uint32_t Interface, uint32_t endpointOUT, uint32_t endpointIN)
{
    // Reset Interface
    usbTransferBulkOnlyMassStorageReset(device, Interface);

    // TEST ////////////////////////////////////
    //usbSetFeatureHALT(device, endpointIN);
    //usbSetFeatureHALT(device, endpointOUT);

    // Clear Feature HALT to the Bulk-In  endpoint
    printf("\nGetStatus: %u", usbGetStatus(device, endpointIN));
    usbClearFeatureHALT(device, endpointIN);
    printf("\nGetStatus: %u", usbGetStatus(device, endpointIN));

    // Clear Feature HALT to the Bulk-Out endpoint
    printf("\nGetStatus: %u", usbGetStatus(device, endpointOUT));
    usbClearFeatureHALT(device, endpointOUT);
    printf("\nGetStatus: %u", usbGetStatus(device, endpointOUT));

    // set configuration to 1 and endpoint IN/OUT toggles to 0
    usbTransferSetConfiguration(device, 1); // set first configuration
    uint8_t config = usbTransferGetConfiguration(device);
    if (config != 1)
    {
        textColor(ERROR);
        printf("\tconfiguration: %u (to be: 1)",config);
        textColor(TEXT);
    }

    // start with correct endpoint toggles and reset interface
    device->ToggleEndpointInMSD = device->ToggleEndpointOutMSD = 0;
    usbTransferBulkOnlyMassStorageReset(device, device->numInterfaceMSD); // Reset Interface
}

int32_t showResultsRequestSense(void* addr)
{
    uint32_t Valid        = getField(addr, 0, 7, 1); // byte 0, bit 7
    uint32_t ResponseCode = getField(addr, 0, 0, 7); // byte 0, bit 6:0
    uint32_t SenseKey     = getField(addr, 2, 0, 4); // byte 2, bit 3:0

    textColor(IMPORTANT);
    printf("\n\nResults of \"request sense\":\n");
    if (ResponseCode >= 0x70 && ResponseCode <= 0x73)
    {
        textColor(TEXT);
        printf("Valid:\t\t");
        if (Valid == 0)
        {
            printf("Sense data are not SCSI compliant");
        }
        else
        {
            printf("Sense data are SCSI compliant");
        }
        printf("\nResponse Code:\t");
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
                printf("No valid response code!");
                break;
        }

        static const char* const SenseKeys[] =
        {
            "No Sense",
            "Recovered Error - last command completed with some recovery action",
            "Not Ready - logical unit addressed cannot be accessed",
            "Medium Error - command terminated with a non-recovered error condition",
            "Hardware Error",
            "Illegal Request - illegal parameter in the command descriptor block",
            "Unit Attention - disc drive may have been reset.",
            "Data Protect - command read/write on a protected block",
            "not defined",
            "Firmware Error",
            "not defined",
            "Aborted Command - disc drive aborted the command",
            "Equal - SEARCH DATA command has satisfied an equal comparison",
            "Volume Overflow - buffered peripheral device has reached the end of medium partition",
            "Miscompare - source data did not match the data read from the medium",
            "not defined"
        };
        printf("\nSense Key:\t");
        if(SenseKey <= 0xF)
            puts(SenseKeys[SenseKey]);
        else
            printf("sense key not known!");
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
