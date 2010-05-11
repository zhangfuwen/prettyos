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

const uint32_t CBWMagic = 0x43425355;

void* cmdQTD;
void* StatusQTD;

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
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
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
    printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(uint32_t device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength)
{
    printf("\nOUT part");

    void* QH_Out = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    void* QH_In  = malloc(sizeof(ehci_qhd_t), PAGESIZE);

    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH_Out);
    printf("\nasyncList: %X <-- QH_Out", pOpRegs->ASYNCLISTADDR);

    // OUT qTD
    // No handshake!
    cmdQTD = createQTD_IO(0x01, OUT, 0, 31); // OUT DATA0, 31 byte
    printf("\tCommandQTD: %X",paging_get_phys_addr(kernel_pd, (void*)cmdQTD));

    // http://en.wikipedia.org/wiki/SCSI_CDB
    struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
    memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw

    switch (SCSIcommand)
    {
    case 0x00: // test unit ready(6)

        cbw->CBWSignature          = CBWMagic;      // magic
        cbw->CBWTag                = 0x42424200;    // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 0;
        cbw->CBWFlags              = 0x00;          // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;             // only bits 3:0
        cbw->CBWCBLength           = 6;             // only bits 4:0
        cbw->commandByte[0]        = 0x00;          // Operation code
        cbw->commandByte[1]        = 0;             // Reserved
        cbw->commandByte[2]        = 0;             // Reserved
        cbw->commandByte[3]        = 0;             // Reserved
        cbw->commandByte[4]        = 0;             // Reserved
        cbw->commandByte[5]        = 0;             // Control
        break;

    case 0x03: // Request Sense(6)

        cbw->CBWSignature          = CBWMagic;      // magic
        cbw->CBWTag                = 0x42424203;    // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 18;
        cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;             // only bits 3:0
        cbw->CBWCBLength           = 6;             // only bits 4:0
        cbw->commandByte[0]        = 0x03;          // Operation code
        cbw->commandByte[1]        = 0;             // Reserved
        cbw->commandByte[2]        = 0;             // Reserved
        cbw->commandByte[3]        = 0;             // Reserved
        cbw->commandByte[4]        = 18;            // Allocation length (max. bytes)
        cbw->commandByte[5]        = 0;             // Control
        break;

    case 0x12: // Inquiry(6)

        cbw->CBWSignature          = CBWMagic;      // magic
        cbw->CBWTag                = 0x42424212;    // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 36;
        cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;             // only bits 3:0
        cbw->CBWCBLength           = 6;             // only bits 4:0
        cbw->commandByte[0]        = 0x12;          // Operation code
        cbw->commandByte[1]        = 0;             // Reserved
        cbw->commandByte[2]        = 0;             // Reserved
        cbw->commandByte[3]        = 0;             // Reserved
        cbw->commandByte[4]        = 36;            // Allocation length (max. bytes)
        cbw->commandByte[5]        = 0;             // Control
        break;

    case 0x25: // read capacity(10)

        cbw->CBWSignature          = CBWMagic;      // magic
        cbw->CBWTag                = 0x42424225;    // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength = 8;
        cbw->CBWFlags              = 0x80;          // Out: 0x00  In: 0x80
        cbw->CBWLUN                = 0;             // only bits 3:0
        cbw->CBWCBLength           = 10;            // only bits 4:0
        cbw->commandByte[0]        = 0x25;          // Operation code
        cbw->commandByte[1]        = 0;             // 7:5 LUN  4:1 reserved  0 RelAddr
        cbw->commandByte[2]        = BYTE4(LBA);    // LBA MSB
        cbw->commandByte[3]        = BYTE3(LBA);    // LBA
        cbw->commandByte[4]        = BYTE2(LBA);    // LBA
        cbw->commandByte[5]        = BYTE1(LBA);    // LBA LSB
        cbw->commandByte[6]        = 0;             // Reserved
        cbw->commandByte[7]        = 0;             // Reserved
        cbw->commandByte[8]        = 0;             // 7:1 Reserved  0 PMI
        cbw->commandByte[9]        = 0;             // Control
        break;

    case 0x28: // read(10)

        cbw->CBWSignature           = CBWMagic;             // magic
        cbw->CBWTag                 = 0x42424228;           // device echoes this field in the CSWTag field of the associated CSW
        cbw->CBWDataTransferLength  = TransferLength*512;   // byte = 512 * block
        cbw->CBWFlags               = 0x80;                 // Out: 0x00  In: 0x80
        cbw->CBWLUN                 = 0;                    // only bits 3:0
        cbw->CBWCBLength            = 10;                   // only bits 4:0
        cbw->commandByte[0]         = 0x28;                 // Operation code
        cbw->commandByte[1]         = 0;                    // Reserved // RDPROTECT/DPO/FUA/FUA_NV
        cbw->commandByte[2]         = BYTE4(LBA);           // LBA MSB
        cbw->commandByte[3]         = BYTE3(LBA);           // LBA
        cbw->commandByte[4]         = BYTE2(LBA);           // LBA
        cbw->commandByte[5]         = BYTE1(LBA);           // LBA LSB
        cbw->commandByte[6]         = 0;                    // Reserved
        cbw->commandByte[7]         = BYTE2(TransferLength); // MSB <--- blocks not byte!
        cbw->commandByte[8]         = BYTE1(TransferLength); // LSB
        cbw->commandByte[9]         = 0;                    // Control

        TransferLength *= 512; // byte = 512 * block
        break;
    }

    // OUT QH
    createQH(QH_Out, paging_get_phys_addr(kernel_pd, QH_Out), cmdQTD,  1, device, endpointOut, 512); // endpoint OUT for MSD

    performAsyncScheduler(true, true);

    /***************************************************************************/

    printf("\nIN part");

    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, QH_In);
    printf("\nasyncList: %X <-- QH_In", pOpRegs->ASYNCLISTADDR);

    // IN qTDs
    // No handshake!
    void* QTD_In;
    void* next = NULL;

    if (TransferLength > 0)
    {
        next = StatusQTD = createQTD_MSDStatus(0x1, 1); // next, toggle // IN 13 byte
        printf("\tStatusQTD: %X",paging_get_phys_addr(kernel_pd, (void*)StatusQTD));

        QTD_In = DataQTD = createQTD_IO((uintptr_t)next, IN,  0, TransferLength); // IN/OUT DATA0, ... byte
        printf("\tDataQTD: %X",paging_get_phys_addr(kernel_pd, (void*)DataQTD));
    }
    else
    {
        QTD_In = StatusQTD = createQTD_MSDStatus(0x1, 0); // next, toggle // IN 13 byte
        printf("\tStatusQTD: %X",paging_get_phys_addr(kernel_pd, (void*)StatusQTD));
    }

    // IN QH
    createQH(QH_In, paging_get_phys_addr(kernel_pd, QH_In), QTD_In, 1, device, endpointIn, 512); // endpoint IN for MSD

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

    printf("\n");
    showPacket(MSDStatusQTDpage0,13);

    if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x0 )
    {
        printf("\nCommand Passed (\"good status\") ");
    }
    if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x1 )
    {
        printf("\nCommand failed");
    }
    if( ( (*(((uint32_t*)MSDStatusQTDpage0)+3)) & 0x000000FF ) == 0x2 )
    {
        printf("\nPhase Error");
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    }

    // transfer diagnosis
    uint32_t statusCommand = showStatusbyteQTD(cmdQTD);    printf("<-- command");    // In/Out Data
    uint32_t statusData = 0x00;
    if (TransferLength)
    {
        statusData = showStatusbyteQTD(DataQTD);   printf("<-- data");   // In/Out Data
    }
    uint32_t statusStatus = showStatusbyteQTD(StatusQTD); printf("<-- status");     // In CSW

    if ( (statusCommand & 0x40) || (statusData & 0x40) || (statusStatus & 0x40) )
    {
        usbResetRecoveryMSD(device, usbDevices[device].numInterfaceMSD, usbDevices[device].numEndpointOutMSD, usbDevices[device].numEndpointInMSD);
    }
}


/*
void testMSD(uint8_t devAddr, uint8_t config)
{
    // maxLUN (0 for USB-sticks)
    usbDevices[devAddr].maxLUN = 0;
    uint8_t statusByte;
    
    usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface

    ///////// send SCSI comamnd "inquiry (opcode: 0x12)"

    textColor(0x09); printf("\n>>> SCSI: inquiry"); textColor(0x0F);
              
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x12, 0, 36); // dev, endp, cmd, LBA, transfer length
    
    /// Important TEST on correct usbSendSCSIcmd //////////////////////////////
        uint8_t currConfig = usbTransferGetConfiguration(devAddr);
        if (currConfig != config) 
        {
            usbTransferSetConfiguration(devAddr,config); // try 
            currConfig = usbTransferGetConfiguration(devAddr);
            if (currConfig != config) 
            {
                textColor(0x0C);
                printf("Error! Current configuration: %d",currConfig); 
                textColor(0x0F);
            }
        }
    /// Important TEST ////////////////////////////////////////////////////////

    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));
    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }
    showUSBSTS();
    waitForKeyStroke();

    for (int i=0; i<2; i++)
    {
        ///////// send SCSI comamnd "test unit ready (opcode: 0x00)"
        
        textColor(0x09); printf("\n>>> SCSI: test unit ready"); textColor(0x0F);

        usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0); // dev, endp, cmd, LBA, transfer length
        
        /// Important TEST on correct usbSendSCSIcmd //////////////////////////////
        currConfig = usbTransferGetConfiguration(devAddr);
        if (currConfig != config) 
        {
            usbTransferSetConfiguration(devAddr,config); // try 
            currConfig = usbTransferGetConfiguration(devAddr);
            if (currConfig != config) 
            {
                textColor(0x0C);
                printf("Error! Current configuration: %d",currConfig); 
                textColor(0x0F);
            }
        }
        /// Important TEST ////////////////////////////////////////////////////////
        
        statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

        if (statusByte == 0x00)
        {
            textColor(0x02);
            printf("\n\nCommand Block Status Values in \"good status\"\n");
            textColor(0x0F);
        }
        showUSBSTS();


        ///////// send SCSI comamnd "request sense (opcode: 0x03)"

        textColor(0x09); printf("\n>>> SCSI: request sense"); textColor(0x0F);
        
        usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18); // dev, endp, cmd, LBA, transfer length
       
        statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));
        if (statusByte == 0x00)
        {
            textColor(0x02);
            printf("\n\nCommand Block Status Values in \"good status\"\n");
            textColor(0x0F);
        }
        
        showUSBSTS();        
        showResultsRequestSense();
        waitForKeyStroke();
    }

    ///////// send SCSI comamnd "read capacity (opcode: 0x25)"

    textColor(0x09); printf("\n>>> SCSI: read capacity"); textColor(0x0F);
    
    usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x25, 0, 8); // dev, endp, cmd, LBA, transfer length
    
    /// Important TEST on correct usbSendSCSIcmd //////////////////////////////
        currConfig = usbTransferGetConfiguration(devAddr);
        if (currConfig != config) 
        {
            usbTransferSetConfiguration(devAddr,config); // try 
            currConfig = usbTransferGetConfiguration(devAddr);
            if (currConfig != config) 
            {
                textColor(0x0C);
                printf("Error! Current configuration: %d",currConfig); 
                textColor(0x0F);
            }
        }
    /// Important TEST ////////////////////////////////////////////////////////
    
    uint32_t lastLBA    = (*((uint8_t*)DataQTDpage0+0)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+1)) * 0x10000 + (*((uint8_t*)DataQTDpage0+2)) * 0x100 + (*((uint8_t*)DataQTDpage0+3));
    uint32_t blocksize  = (*((uint8_t*)DataQTDpage0+4)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+5)) * 0x10000 + (*((uint8_t*)DataQTDpage0+6)) * 0x100 + (*((uint8_t*)DataQTDpage0+7));
    uint32_t capacityMB = ((lastLBA+1)/1000000) * blocksize;

    textColor(0x0E);
    printf("\nCapacity: %d MB, Last LBA: %d, block size %d\n", capacityMB, lastLBA, blocksize);
    textColor(0x0F);
    showUSBSTS();
    waitForKeyStroke();

    ///////// send SCSI comamnd "read (opcode: 0x28)", read one block (512 byte) from LBA ..., get Status

    uint32_t blocks = 1; // number of blocks to be read
    for(uint32_t sector=0; sector<5; sector++)
    {
        textColor(0x09); printf("\n>>> SCSI: read (opcode: 0x28) sector: %d", sector); textColor(0x0F);
        
        usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x28, sector, blocks); // dev, endp, cmd, LBA, transfer length
        
        /// Important TEST on correct usbSendSCSIcmd //////////////////////////////
        currConfig = usbTransferGetConfiguration(devAddr);
        if (currConfig != config) 
        {
            usbTransferSetConfiguration(devAddr,config); // try 
            currConfig = usbTransferGetConfiguration(devAddr);
            if (currConfig != config) 
            {
                textColor(0x0C);
                printf("Error! Current configuration: %d",currConfig); 
                textColor(0x0F);
            }
        }
        /// Important TEST ////////////////////////////////////////////////////////
        
        showUSBSTS();
        waitForKeyStroke();
    }
}
*/

void testMSD(uint8_t devAddr, uint8_t config)

{
    // maxLUN (0 for USB-sticks)
    usbDevices[devAddr].maxLUN = 0;
    uint8_t statusByte;

    usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface

    ///////// send SCSI comamnd "inquiry (opcode: 0x12)"

    textColor(0x09); printf("\n>>> SCSI: inquiry"); textColor(0x0F);

    /// TEST
    uint8_t configOld = usbTransferGetConfiguration(devAddr);
    printf(" %d",configOld); // check configuration
    usbTransferSetConfiguration(devAddr,config);  
    /// TEST

    configOld = usbTransferGetConfiguration(devAddr);
    printf(" %d",configOld); // check configuration
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x12, 0, 36); // dev, endp, cmd, LBA, transfer length

    configOld = usbTransferGetConfiguration(devAddr);
    printf(" %d",configOld); // check configuration

    statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

    if (statusByte == 0x00)
    {
        textColor(0x02);
        printf("\n\nCommand Block Status Values in \"good status\"\n");
        textColor(0x0F);
    }
    showUSBSTS();
    waitForKeyStroke();
	configOld = usbTransferGetConfiguration(devAddr);
    printf("Alte config2: %d",configOld); // check configuration

    ///////// send SCSI comamnd "test unit ready(6)"

    int32_t timeout = 2;
    int32_t sense = -1;
    bool repeat = true;

    while (repeat)
    {
        textColor(0x09); printf("\n>>> SCSI: test unit ready"); textColor(0x0F);

        /// TEST
        configOld = usbTransferGetConfiguration(devAddr);
        printf(" %d",configOld); // check configuration
        usbTransferSetConfiguration(devAddr,config); 

        //usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x00, 0, 0); // dev, endp, cmd, LBA, transfer length

        statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));

        if (statusByte == 0x00)
        {
            textColor(0x02);
            printf("\n\nCommand Block Status Values in \"good status\"\n");
            textColor(0x0F);
        }
        showUSBSTS();


        ///////// send SCSI comamnd "request sense"

        textColor(0x09); printf("\n>>> SCSI: request sense"); textColor(0x0F);

        /// TEST
        configOld = usbTransferGetConfiguration(devAddr);
        printf(" %d",configOld); // check configuration
        usbTransferSetConfiguration(devAddr,config); 

        //usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x03, 0, 18); // dev, endp, cmd, LBA, transfer length

        statusByte = BYTE1(*(((uint32_t*)MSDStatusQTDpage0)+3));
        if (statusByte == 0x00)
        {
            textColor(0x02);
            printf("\n\nCommand Block Status Values in \"good status\"\n");
            textColor(0x0F);
        }

        showUSBSTS();        
        timeout--;

        sense = showResultsRequestSense();
        if ( (sense == 0) || (sense == 6) || (timeout <= 0) )
        {
            repeat = false;
        }
    }    


    ///////// send SCSI comamnd "read capacity(10)"

    textColor(0x09); printf("\n>>> SCSI: read capacity"); textColor(0x0F);

    /// TEST
    configOld = usbTransferGetConfiguration(devAddr);
    printf(" %d",configOld); // check configuration
    usbTransferSetConfiguration(devAddr,config); 

    //usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
    usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x25, 0, 8); // dev, endp, cmd, LBA, transfer length

    uint32_t lastLBA    = (*((uint8_t*)DataQTDpage0+0)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+1)) * 0x10000 + (*((uint8_t*)DataQTDpage0+2)) * 0x100 + (*((uint8_t*)DataQTDpage0+3));
    uint32_t blocksize  = (*((uint8_t*)DataQTDpage0+4)) * 0x1000000 + (*((uint8_t*)DataQTDpage0+5)) * 0x10000 + (*((uint8_t*)DataQTDpage0+6)) * 0x100 + (*((uint8_t*)DataQTDpage0+7));
    uint32_t capacityMB = ((lastLBA+1)/1000000) * blocksize;

    textColor(0x0E);
    printf("\nCapacity: %d MB, Last LBA: %d, block size %d\n", capacityMB, lastLBA, blocksize);
    textColor(0x0F);

    showUSBSTS();
    waitForKeyStroke();


    ///////// send SCSI comamnd "read(10)", read one block (512 byte) from LBA ..., get Status

    uint32_t blocks = 1; // number of blocks to be read
    for(uint32_t sector=0; sector<5; sector++)
    {
        /// TEST
        configOld = usbTransferGetConfiguration(devAddr);
        printf(" %d",configOld); // check configuration
        usbTransferSetConfiguration(devAddr,config); 

        textColor(0x09); printf("\n>>> SCSI: read(10)  sector: %d", sector); textColor(0x0F);

        //usbTransferBulkOnlyMassStorageReset(devAddr, usbDevices[devAddr].numInterfaceMSD); // Reset Interface
        usbSendSCSIcmd(devAddr, usbDevices[devAddr].numInterfaceMSD, usbDevices[devAddr].numEndpointOutMSD, usbDevices[devAddr].numEndpointInMSD, 0x28, sector, blocks); // dev, endp, cmd, LBA, transfer length

        showUSBSTS();
        waitForKeyStroke();
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
}

int32_t showResultsRequestSense()
{
    uint32_t Valid        = ((*(((uint8_t*)DataQTDpage0)+0))&0x80)>>7; // byte 0, bit 7
    uint32_t ResponseCode = ((*(((uint8_t*)DataQTDpage0)+0))&0x7F)>>0; // byte 0, bit 6:0
    uint32_t SenseKey     = ((*(((uint8_t*)DataQTDpage0)+2))&0x0F)>>0; // byte 2, bit 3:0

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
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
