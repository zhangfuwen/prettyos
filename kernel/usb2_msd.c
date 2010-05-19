/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "paging.h"
#include "kheap.h"
#include "console.h"
#include "util.h"
#include "ehci.h"
#include "ehciQHqTD.h"
#include "usb2.h"
#include "usb2_msd.h"

#include "fat12.h" // for first tests only

extern const uint32_t CSWMagicNotOK;
const uint32_t CSWMagicOK    = 0x53425355; // USBS
const uint32_t CBWMagic      = 0x43425355; // USBC

uint8_t currCSWtag;

void* cmdQTD;
void* StatusQTD;

uint32_t startSectorPartition;

extern usb2_Device_t usbDevices[17]; // ports 1-16 // 0 not used


// Bulk-Only Mass Storage get maximum number of Logical Units
uint8_t usbTransferBulkOnlyGetMaxLUN(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: usbTransferBulkOnlyGetMaxLUN, dev: %d interface: %d", device, numInterface); textColor(0x0F);
    #endif

    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next      = createQTD_Handshake(OUT); // Handshake is the opposite direction of Data
    next = DataQTD  = createQTD_IO( (uintptr_t)next, IN, 1, 1);  // IN DATA1, 1 byte
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0xA1, 0xFE, 0, 0, numInterface, 1);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 10100001b     11111110b 0000h  Interface 0001h     1 byte

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);
    
    return *((uint8_t*)DataQTDpage0);
}

// Bulk-Only Mass Storage Reset
void usbTransferBulkOnlyMassStorageReset(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
    textColor(0x0B); printf("\nUSB2: usbTransferBulkOnlyMassStorageReset, dev: %d interface: %d", device, numInterface); textColor(0x0F);
    #endif

    void* QH = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH);

    // Create QTDs (in reversed order)
    void* next = createQTD_Handshake(IN);
    next = SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x21, 0xFF, 0, 0, numInterface, 0);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 00100001b     11111111b 0000h  Interface 0000h     none

    // Create QH
    createQH(QH, paging_get_phys_addr(kernel_pd, QH), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler(true, false);    
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
    }// switch   
    
    currCSWtag = SCSIcommand;
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(uint32_t device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer)
{
  #ifdef _USB_DIAGNOSIS_
    printf("\nOUT part");
  #endif

    void* QH_Out = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    void* QH_In  = malloc(sizeof(ehci_qhd_t), PAGESIZE);

    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH_Out);
    
  #ifdef _USB_DIAGNOSIS_
    printf("\nasyncList: %X <-- QH_Out", pOpRegs->ASYNCLISTADDR); 
  #endif

    // OUT qTD
    // No handshake!
    
  #ifdef _USB_DIAGNOSIS_
     textColor(0x03);
     printf("\ntoggle OUT %d", usbDevices[device].ToggleEndpointOutMSD);
     textColor(0x0F);
  #endif

    cmdQTD = createQTD_IO(0x01, OUT, usbDevices[device].ToggleEndpointOutMSD, 31);        // OUT DATA0 or DATA1, 31 byte
    usbDevices[device].ToggleEndpointOutMSD = !(usbDevices[device].ToggleEndpointOutMSD); // switch toggle

  #ifdef _USB_DIAGNOSIS_
    printf("\tCommandQTD: %X",paging_get_phys_addr(kernel_pd, (void*)cmdQTD));
  #endif

    // http://en.wikipedia.org/wiki/SCSI_CDB
    struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
    memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw    
    SCSIcmd(SCSIcommand, cbw, LBA, TransferLength);
    if (SCSIcommand == 0x28)   // read(10)        
        TransferLength *= 512; // byte = 512 * block
    
    createQH(QH_Out, paging_get_phys_addr(kernel_pd, QH_In), cmdQTD,  1, device, endpointOut, 512); // endpoint OUT for MSD

  /**************************************************************************************************************************************/

  #ifdef _USB_DIAGNOSIS_
    printf("\nIN part");
  #endif

    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH_In);
    
  #ifdef _USB_DIAGNOSIS_    
    printf("\nasyncList: %X <-- QH_In", pOpRegs->ASYNCLISTADDR);
  #endif

    // IN qTDs
    // No handshake!
    void* QTD_In;
    void* next = NULL;

    if (TransferLength > 0)
    {
      #ifdef _USB_DIAGNOSIS_
        textColor(0x03);
        printf("\ntoggles IN: data: %d  status: %d", usbDevices[device].ToggleEndpointInMSD, !(usbDevices[device].ToggleEndpointInMSD));
        textColor(0x0F);
      #endif

        next = StatusQTD = createQTD_MSDStatus(0x1, !(usbDevices[device].ToggleEndpointInMSD));   // next, toggle, IN 13 byte
        QTD_In = DataQTD = createQTD_IO((uintptr_t)next, IN, usbDevices[device].ToggleEndpointInMSD, TransferLength); // IN/OUT DATA0, ... byte
        /*do not switch toggle*/
    
      #ifdef _USB_DIAGNOSIS_    
        printf("\tStatusQTD: %X", paging_get_phys_addr(kernel_pd, (void*)StatusQTD));
        printf("\tDataQTD: %X",   paging_get_phys_addr(kernel_pd, (void*)DataQTD));
      #endif
    }
    else
    {
      #ifdef _USB_DIAGNOSIS_    
        textColor(0x03);
        printf("\ntoggle IN: status: %d", usbDevices[device].ToggleEndpointInMSD);
        textColor(0x0F); 
      #endif

        QTD_In = StatusQTD = createQTD_MSDStatus(0x1, usbDevices[device].ToggleEndpointInMSD); // next, toggle, IN 13 byte
        usbDevices[device].ToggleEndpointInMSD = !(usbDevices[device].ToggleEndpointInMSD);    // switch toggle
      
      #ifdef _USB_DIAGNOSIS_    
        printf("\tStatusQTD: %X", paging_get_phys_addr(kernel_pd, (void*)StatusQTD));
      #endif
    }

    createQH(QH_In, paging_get_phys_addr(kernel_pd, QH_Out), QTD_In, 0, device, endpointIn, 512); // endpoint IN for MSD

    performAsyncScheduler(true, true); 

    if (TransferLength) // byte
    {
        printf("\n");
        showPacket(DataQTDpage0,TransferLength);
        if ((TransferLength==512) || (TransferLength==36)) // data block (512 byte), inquiry feedback (36 byte)
        {
            showPacketAlphaNumeric(DataQTDpage0,TransferLength);
        }
    }

    // CSW Status
    printf("\n");
    showPacket(MSDStatusQTDpage0,13);
    
    // check signature 0x53425355 // DWORD 0 (byte 0:3)
    uint32_t CSWsignature = *(uint32_t*)MSDStatusQTDpage0; // DWORD 0 
    if (CSWsignature == CSWMagicOK)
    {
    //#ifdef _USB_DIAGNOSIS_ 
        textColor(0x0A);
        printf("\nCSW signature OK    "); 
        textColor(0x0F);
    //#endif
    }
    else if (CSWsignature == CSWMagicNotOK)
    {
        textColor(0x0C);
        printf("\nCSW signature wrong (not processed)");
        textColor(0x0F);
    }
    else
    {
        textColor(0x0C);
        printf("\nCSW signature wrong (processed, but wrong value)");
        textColor(0x0F);
    }

    // check matching tag 
    uint32_t CSWtag = *(((uint32_t*)MSDStatusQTDpage0)+1); // DWORD 1 (byte 4:7)
    if ((BYTE1(CSWtag) == currCSWtag) && (BYTE2(CSWtag) == 0x42) && (BYTE3(CSWtag) == 0x42) && (BYTE4(CSWtag) == 0x42))
    {
    //#ifdef _USB_DIAGNOSIS_ 
        textColor(0x0A);
        printf("CSW tag %y OK    ",BYTE1(CSWtag));
        textColor(0x0F);
    //#endif
    }
    else
    {
        textColor(0x0C);
        printf("\nError: CSW tag wrong");
        textColor(0x0F);
    }
    
    // check CSWDataResidue
    uint32_t CSWDataResidue = *(((uint32_t*)MSDStatusQTDpage0)+2); // DWORD 2 (byte 8:11)
    if (CSWDataResidue == 0)
    {
    //#ifdef _USB_DIAGNOSIS_ 
        textColor(0x0A);
        printf("\tCSW data residue OK    ");
        textColor(0x0F);
    //#endif
    }
    else
    {
        textColor(0x06);
        printf("\nCSW data residue: %d",CSWDataResidue);
        textColor(0x0F);
    }
    
    // check status byte // DWORD 3 (byte 12)
    // uint8_t CSWstatusByte = (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF;
    uint8_t CSWstatusByte = *(((uint8_t*)MSDStatusQTDpage0)+12); // byte 12 (last byte of 13 bytes)                            

    switch (CSWstatusByte)
    {
    case 0x00:
    //#ifdef _USB_DIAGNOSIS_   
        textColor(0x0A);
        printf("\tCSW status OK");
        textColor(0x0F);
    //#endif
    break;
    case 0x01 :
        textColor(0x0C);
        printf("\nCommand failed");
        textColor(0x0F);
    break;
    case 0x02:
        textColor(0x0C);
        printf("\nPhase Error");
        textColor(0x0F);
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    break;
    default:
        textColor(0x0C);
        printf("\nCSW status byte: undefined value (error)");
        textColor(0x0F);
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    break;
    }

    // transfer diagnosis (qTD status)
    uint32_t statusCommand = showStatusbyteQTD(cmdQTD);    
    if (statusCommand) 
    {
        printf("<-- command"); // In/Out Data
        bulkTransfer->successfulCommand = false;
        if (statusCommand == 0x01) // Do Ping
        {
            bulkTransfer->successfulCommand = true;
        }
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
        {
            bulkTransfer->successfulDataIN = true; // TEST: currently only IN data are used
        }
    }
    
    uint32_t statusStatus = showStatusbyteQTD(StatusQTD);  
    if (statusStatus) 
    {
        printf("<-- status");   // In CSW 
    }
    else
    {
        bulkTransfer->successfulCSW = true;
    }

    if ((statusCommand & 0x40) || (statusData & 0x40) || (statusStatus & 0x40))
    {
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    }
}

static uint8_t getStatusByte()
{
    return BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));
}

static uint8_t testDeviceReady(uint8_t devAddr, usbBulkTransfer_t* bulkTransferTestUnitReady, usbBulkTransfer_t* bulkTransferRequestSense)
{
    int32_t timeout = 2;
    int32_t sense = -1;
    bool repeat = true;
    uint8_t statusByte;

    while (repeat)
    {
        textColor(0x09); printf("\n\n>>> SCSI: test unit ready"); textColor(0x0F);

        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0, bulkTransferTestUnitReady); // dev, endp, cmd, LBA, transfer length

        uint8_t statusByteTestReady = getStatusByte();
        showUSBSTS();
        waitForKeyStroke();

        ///////// send SCSI command "request sense"

        textColor(0x09); printf("\n\n>>> SCSI: request sense"); textColor(0x0F);

        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18, bulkTransferRequestSense); // dev, endp, cmd, LBA, transfer length

        statusByte = getStatusByte();
        showUSBSTS();        
        timeout--;

        sense = showResultsRequestSense();
        if ( ((statusByteTestReady == 0) && ((sense == 0) || (sense == 6))) || (timeout <= 0) )
        {
            repeat = false;
        }
        waitForKeyStroke();
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
static void analyzeInquiry()
{
    void* addr = (void*)DataQTDpage0;

    uint8_t PeripheralDeviceType = getField(addr, 0, 0, 5); // byte, shift, len
    // uint8_t PeripheralQualifier  = getField(addr, 0, 5, 3); 

    // uint8_t DeviceTypeModifier   = getField(addr, 1, 0, 7);  
    uint8_t RMB                  = getField(addr, 1, 7, 1); 
    
    // uint8_t ANSIapprovedVersion  = getField(addr, 2, 0, 3); 
    // uint8_t ECMAversion          = getField(addr, 2, 3, 3); 
    // uint8_t ISOversion           = getField(addr, 2, 6, 2); 

    // uint8_t ResponseDataFormat   = getField(addr, 3, 0, 4);
    // uint8_t Reserved1            = getField(addr, 3, 4, 2); 
    // uint8_t TrmIOP               = getField(addr, 3, 6, 1); 
    // uint8_t AENC                 = getField(addr, 3, 7, 1);   
    
    // uint8_t AdditionalLength     = getField(addr, 4, 0, 8);   
    
    // uint8_t Reserved2            = getField(addr, 5, 0, 8);   

    // uint8_t Reserved3            = getField(addr, 6, 0, 8);   

    uint8_t SftRe                = getField(addr, 7, 0, 1);
    uint8_t CmdQue               = getField(addr, 7, 1, 1);
    // uint8_t Reserved4            = getField(addr, 7, 2, 1); 
    uint8_t Linked               = getField(addr, 7, 3, 1);
    uint8_t Sync                 = getField(addr, 7, 4, 1);
    // uint8_t WBus16               = getField(addr, 7, 5, 1); 
    // uint8_t WBus32               = getField(addr, 7, 6, 1); 
    // uint8_t RelAddr              = getField(addr, 7, 7, 1); 
    
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

    char productRevisionLevel[5];
    for (uint8_t i=0; i<4;i++)
    {
        productRevisionLevel[i]= getField(addr, i+32, 0, 8); 
    }
    productRevisionLevel[4]=0;
    
    printf("\nVendor ID:  %s", vendorID);
    printf("\nProduct ID: %s", productID);
    printf("\nRevision:   %s", productRevisionLevel);
    printf("\nRemovable device type:           %s", RMB    ? "yes" : "no");    
    printf("\nSupports synchronous transfers:  %s", Sync   ? "yes" : "no"); 
    printf("\nSupports linked commands:        %s", Linked ? "yes" : "no"); 
    printf("\nSupports tagged command queuing: %s", CmdQue ? "yes" : "no"); 
    printf("\nPerforms soft resets:            %s", SftRe  ? "yes" : "no");
    
    switch (PeripheralDeviceType)
    {
    case 0x00: printf("\ndirect-access device (e.g., magnetic disk)");break;
    case 0x01: printf("\nsequential-access device (e.g., magnetic tape)");break;
    case 0x02: printf("\nprinter device");break;
    case 0x03: printf("\nprocessor device");break;
    case 0x04: printf("\nwrite-once device");break;
    case 0x05: printf("\nCDROM device");break;
    case 0x06: printf("\nscanner device");break;
    case 0x07: printf("\noptical memory device (e.g., some optical disks)");break;
    case 0x08: printf("\nmedium Changer (e.g. jukeboxes)");break;
    case 0x09: printf("\ncommunications device");break;
    case 0x0A: printf("\ndefined by ASC IT8 (Graphic arts pre-press devices)");break;
    case 0x0B: printf("\ndefined by ASC IT8 (Graphic arts pre-press devices)");break;
    case 0x0C: printf("\nStorage array controller device (e.g., RAID)");break;
    case 0x0D: printf("\nEnclosure services device");break;
    case 0x0E: printf("\nSimplified direct-access device (e.g., magnetic disk)");break;
    case 0x0F: printf("\nOptical card reader/writer device");break;
    case 0x10: printf("\nReserved for bridging expanders");break;
    case 0x11: printf("\nObject-based Storage Device");break;
    case 0x12: printf("\nAutomation/Drive Interface");break;
    case 0x13: printf("\nReserved");break;
    case 0x1D: printf("\nReserved");break;
    case 0x1E: printf("\nWell known logical unit");break;
    case 0x1F: printf("\nUnknown or no device type");break;
    }
}

void testMSD(uint8_t devAddr, uint8_t config)
{
    if (usbDevices[devAddr].InterfaceClass != 0x08)
    {
        textColor(0x0C);
        printf("\nThis is no Mass Storage Device! MSD test cannot be carried out.");
        textColor(0x0F);
        waitForKeyStroke();
    }
    else
    {
        // maxLUN (0 for USB-sticks)
        usbDevices[devAddr].maxLUN = 0;

        // start with correct endpoint toggles and reset interface
        usbDevices[devAddr].ToggleEndpointInMSD = usbDevices[devAddr].ToggleEndpointOutMSD = 0;
        usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface

        ///////// send SCSI command "inquiry (opcode: 0x12)"

        textColor(0x09); printf("\n\n>>> SCSI: inquiry"); textColor(0x0F);
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
        waitForKeyStroke();
        

        ///////// send SCSI command "test unit ready(6)"
        
        usbBulkTransfer_t testUnitReady, requestSense;
        startLogBulkTransfer(&testUnitReady, 0x00,  0, 0);
        startLogBulkTransfer(&requestSense,  0x03, 18, 0);
        testDeviceReady(devAddr, &testUnitReady, &requestSense);
        logBulkTransfer(&testUnitReady);
        logBulkTransfer(&requestSense);


        ///////// send SCSI command "read capacity(10)"

        textColor(0x09); printf("\n\n>>> SCSI: read capacity"); textColor(0x0F);
     
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

        textColor(0x0E);
        printf("\nCapacity: %d MB, Last LBA: %d, block size %d\n", capacityMB, lastLBA, blocksize);
        textColor(0x0F);

        showUSBSTS();
        logBulkTransfer(&readCapacity);
        waitForKeyStroke();


        ///////// send SCSI command "read(10)", read one block (512 byte) from LBA ..., get Status

        uint32_t blocks = 1; // number of blocks to be read

        uint32_t start = 0;
label1:        
        for(uint32_t sector=start; sector<start+3; sector++)
        {
            textColor(0x09); printf("\n\n>>> SCSI: read   sector: %d", sector); textColor(0x0F);

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
            showUSBSTS();
            logBulkTransfer(&read);
            waitForKeyStroke();
            
            if ( (sector == 0) || (sector == startSectorPartition) || (((*((uint8_t*)DataQTDpage0+510))==0x55)&&((*((uint8_t*)DataQTDpage0+511))==0xAA)) )
            {                
                analyzeBootSector((void*)DataQTDpage0); // for first tests only
                waitForKeyStroke();
            }
        }
        if (startSectorPartition)
        {
            start = startSectorPartition;
            goto label1;
        }
    }// else    
}

void analyzeBootSector(void* addr) // for first tests only
{
    struct boot_sector* sec0 = (struct boot_sector*)addr;

    char SysName[9];
    char FATn[9];
    strncpy(SysName, sec0->SysName,   8); 
    strncpy(FATn,    sec0->Reserved2, 8);
    SysName[8]=0;
    FATn[8]   =0;
    if (sec0->charsPerSector == 0x200)
    {
        printf("\nOEM name:           %s"    ,SysName);
        printf("\tbyte per sector:        %d",sec0->charsPerSector); 
        printf("\nsectors per cluster:    %d",sec0->SectorsPerCluster); 
        printf("\tnumber of FAT copies:   %d",sec0->FATcount);
        printf("\nmax root dir entries:   %d",sec0->MaxRootEntries);
        printf("\ttotal sectors (<65536): %d",sec0->TotalSectors1);
        printf("\nmedia Descriptor:       %y",sec0->MediaDescriptor);
        printf("\tsectors per FAT:        %d",sec0->SectorsPerFAT);
        printf("\nsectors per track:      %d",sec0->SectorsPerTrack);
        printf("\theads/pages:            %d",sec0->HeadCount);
        printf("\nhidden sectors:         %d",sec0->HiddenSectors);
        printf("\ttotal sectors (>65535): %d",sec0->TotalSectors2);
        printf("\nFAT 12/16:              %s",FATn); 
        
        startSectorPartition = 0;
    }
    else if ( ( ((*((uint8_t*)addr)) == 0xFA) || ((*((uint8_t*)addr)) == 0x33) )  && ((*((uint16_t*)((uint8_t*)addr+444))) == 0x0000) )
    {
        textColor(0x0E);
        if ( ((*((uint8_t*)addr+510))==0x55) && ((*((uint8_t*)addr+511))==0xAA) )
        {
            printf("\nThis seems to be a Master Boot Record:"); 
            textColor(0x0F);
            uint32_t discSignature = *((uint32_t*)((uint8_t*)addr+440));
            uint16_t twoBytesZero  = *((uint16_t*)((uint8_t*)addr+444));
            printf("\n\nDisc Signature: %X\t",discSignature);
            printf("Null (check): %x",twoBytesZero);

            uint8_t partitionTable[4][16]; 
            for (int i=0;i<4;i++) // four tables
            {
                for (int j=0;j<16;j++) // 16 bytes
                {
                    partitionTable[i][j] = *((uint8_t*)addr+446+i*j);                    
                }
            }
            printf("\n");
            for (int i=0;i<4;i++) // four tables
            {
                if (*((uint32_t*)(&partitionTable[i][0x0C]))) // number of sectors
                {
                    textColor(0x0E);
                    printf("\npartition table %d:",i);                    
                    if (partitionTable[i][0x00] == 0x80)
                    {
                        printf("  bootable");
                    }
                    else
                    {
                        printf("  not bootable");
                    }                    
                    textColor(0x0F);
                    printf("\ntype:               %y", partitionTable[i][0x04]);
                    textColor(0x07);
                    printf("\nfirst sector (CHS): %d %d %d", partitionTable[i][0x01],partitionTable[i][0x02],partitionTable[i][0x03]);
                    printf("\nlast  sector (CHS): %d %d %d", partitionTable[i][0x05],partitionTable[i][0x06],partitionTable[i][0x07]);
                    textColor(0x0F);
                    
                    startSectorPartition = *((uint32_t*)(&partitionTable[i][0x08]));
                    printf("\nstart sector:       %d", startSectorPartition);

                    printf("\nnumber of sectors:  %d", *((uint32_t*)(&partitionTable[i][0x0C])));
                    printf("\n");
                }
                else
                {
                    textColor(0x0E);
                    printf("\nno partition table %d",i);
                    textColor(0x0F);
                }
            }
        }
    }
    else
    {
        textColor(0x0C);
        printf("\nThis seems to be neither a FAT description nor a MBR."); 
        textColor(0x0F);
    }
}

void usbResetRecoveryMSD(uint32_t device, uint32_t Interface, uint32_t endpointOUT, uint32_t endpointIN)
{
    // Reset Interface
    usbTransferBulkOnlyMassStorageReset(device, Interface);

    // Clear Feature HALT to the Bulk-In  endpoint
    printf("\nGetStatus: %d", usbGetStatus(device, endpointIN, 512));
    usbClearFeatureHALT(device, endpointIN,  512);
    printf("\nGetStatus: %d", usbGetStatus(device, endpointIN, 512));

    // Clear Feature HALT to the Bulk-Out endpoint
    printf("\nGetStatus: %d", usbGetStatus(device, endpointOUT, 512));
    usbClearFeatureHALT(device, endpointOUT, 512);
    printf("\nGetStatus: %d", usbGetStatus(device, endpointOUT, 512));

    // set configuration to 1 and endpoint IN/OUT toggles to 0
    textColor(0x02);
    usbTransferSetConfiguration(device, 1); // set first configuration
    printf("\nset configuration (1)"); 
    uint8_t config = usbTransferGetConfiguration(device);
    printf("\tconfiguration: %d",config); 

    // start with correct endpoint toggles and reset interface
    usbDevices[device].ToggleEndpointInMSD = usbDevices[device].ToggleEndpointOutMSD = 0;
    usbTransferBulkOnlyMassStorageReset(device, usbDevices[device].numInterfaceMSD); // Reset Interface
    textColor(0x0F);
}

int32_t showResultsRequestSense()
{
    void* addr = (void*)DataQTDpage0;
                            
    uint32_t Valid        = getField(addr, 0, 7, 1); // byte 0, bit 7
    uint32_t ResponseCode = getField(addr, 0, 0, 7); // byte 0, bit 6:0
    uint32_t SenseKey     = getField(addr, 2, 0, 4); // byte 2, bit 3:0

    char ValidStr[40];
    char ResponseCodeStr[40];
    char SenseKeyStr[80];

    if (Valid == 0)
    {
        strcpy(ValidStr,"Sense data are not SCSI compliant");
    }
    else
    {
        strcpy(ValidStr,"Sense data are SCSI compliant");
    }

    switch (ResponseCode)
    {
        case 0x70:
            strcpy(ResponseCodeStr,"Current errors, fixed format");
            break;
        case 0x71:
            strcpy(ResponseCodeStr,"Deferred errors, fixed format");
            break;
        case 0x72:
            strcpy(ResponseCodeStr,"Current error, descriptor format");
            break;
        case 0x73:
            strcpy(ResponseCodeStr,"Deferred error, descriptor format");
            break;
        default:
            strcpy(ResponseCodeStr,"No vaild response code!");
            break;
    }

    switch (SenseKey)
    {
        case 0x0:
            strcpy(SenseKeyStr,"No Sense");
            break;
        case 0x1:
            strcpy(SenseKeyStr,"Recovered Error - last command completed with some recovery action");
            break;
        case 0x2:
            strcpy(SenseKeyStr,"Not Ready - logical unit addressed cannot be accessed");
            break;
        case 0x3:
            strcpy(SenseKeyStr,"Medium Error - command terminated with a non-recovered error condition");
            break;
        case 0x4:
            strcpy(SenseKeyStr,"Hardware Error");
            break;
        case 0x5:
            strcpy(SenseKeyStr,"Illegal Request - illegal parameter in the command descriptor block ");
            break;
        case 0x6:
            strcpy(SenseKeyStr,"Unit Attention - disc drive may have been reset.");
            break;
        case 0x7:
            strcpy(SenseKeyStr,"Data Protect - command read/write on a protected block");
            break;
        case 0x8:
            strcpy(SenseKeyStr,"not defined");
            break;
        case 0x9:
            strcpy(SenseKeyStr,"Firmware Error");
            break;
        case 0xA:
            strcpy(SenseKeyStr,"not defined");
            break;
        case 0xB:
            strcpy(SenseKeyStr,"Aborted Command - disc drive aborted the command");
            break;
        case 0xC:
            strcpy(SenseKeyStr,"Equal - SEARCH DATA command has satisfied an equal comparison");
            break;
        case 0xD:
            strcpy(SenseKeyStr,"Volume Overflow - buffered peripheral device has reached the end of medium partition");
            break;
        case 0xE:
            strcpy(SenseKeyStr,"Miscompare - source data did not match the data read from the medium");
            break;
        case 0xF:
            strcpy(SenseKeyStr,"not defined");
            break;
        default:
            strcpy(SenseKeyStr,"sense key not known!");
            break;
    }

    textColor(0x0E);
    printf("\n\nResults of \"request sense\":");
    if ( (ResponseCode >= 0x70) && (ResponseCode <= 0x73) )
    {
         textColor(0x0F);
         printf("\nValid: \t\t%s \nResponse Code: \t%s \nSense Key: \t%s", ValidStr, ResponseCodeStr, SenseKeyStr);
         return SenseKey;
    }
    else
    {
        textColor(0x0C);
        printf("\nNo vaild response code!");
        textColor(0x0F);
        return -1;
    }
}


/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
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
