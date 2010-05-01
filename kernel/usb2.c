/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "usb2.h"
#include "console.h"
#include "timer.h"
#include "util.h"

usb2_Device_t usbDevices[17]; // ports 1-16 // 0 not used

static void performAsyncScheduler()
{
	// Enable Async...
    USBINTflag = false;
    pOpRegs->USBSTS |= STS_USBINT;
    pOpRegs->USBCMD |= CMD_ASYNCH_ENABLE;

    int8_t timeout=40;
    while (!USBINTflag) // set by interrupt
    {
        timeout--;
        if(timeout>0)
        {
            sleepMilliSeconds(20);
            //printf("#");
        }
        else
        {
            settextcolor(12,0);
            printf("\nError: no USB interrupt! Transfer not completed?");
            settextcolor(15,0);
            break;
        }
    };
    USBINTflag = false;
    pOpRegs->USBSTS |= STS_USBINT;
    pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE;

	sleepMilliSeconds(80);
}

uint8_t usbTransferEnumerate(uint8_t j)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: SET_ADDRESS"); settextcolor(15,0);
    #endif

    uint8_t new_address = j+1; // indicated port number

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 5, 0, new_address, 0, 0); // SETUP DATA0, 8 byte, ..., SET_ADDRESS, hi, 0...127 (new address), index=0, length=0

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, 0, 0,64);

	performAsyncScheduler();

	return new_address; // new_address
}

void usbTransferDevice(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR device, dev: %d endpoint: 0", device); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(              0x1, OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 18);  // IN DATA1, 18 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 1, 0, 0, 18); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, device, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");

	// showPacket(DataQTDpage0,18);
    addDevice ( (struct usb2_deviceDescriptor*)DataQTDpage0, &usbDevices[device] );
    showDevice( &usbDevices[device] );
}

void usbTransferConfig(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR config, dev: %d endpoint: 0", device); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, PAGESIZE);  // IN DATA1, 4096 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 2, 0, 0, PAGESIZE); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, configuration, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");

    // parsen auf config (len=9,type=2), interface (len=9,type=4), endpoint (len=7,type=5)
	uintptr_t addrPointer = (uintptr_t)DataQTDpage0;
    uintptr_t lastByte    = addrPointer + (*(uint16_t*)(addrPointer+2)); // totalLength (WORD)
    // printf("\nlastByte: %X\n",lastByte); // test

    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,(*(uint16_t*)(addrPointer+2)));
    #endif

	while(addrPointer<lastByte)
	{
		bool found = false;
		// printf("addrPointer: %X\n",addrPointer); // test
		if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 2) ) // length, type
		{
			showConfigurationDescriptor((struct usb2_configurationDescriptor*)addrPointer);
			addrPointer += 9;
			found = true;
		}

		if ( ((*(uint8_t*)addrPointer) == 9) && ((*(uint8_t*)(addrPointer+1)) == 4) ) // length, type
		{
			showInterfaceDescriptor((struct usb2_interfaceDescriptor*)addrPointer);

			if (((struct usb2_interfaceDescriptor*)addrPointer)->interfaceClass == 8)
			{
                // store interface number for mass storage transfers
				usbDevices[device].numInterfaceMSD = ((struct usb2_interfaceDescriptor*)addrPointer)->interfaceNumber;
			}
			addrPointer += 9;
			found = true;
		}

		if ( ((*(uint8_t*)addrPointer) == 7) && ((*(uint8_t*)(addrPointer+1)) == 5) ) // length, type
		{
			showEndpointDescriptor ((struct usb2_endpointDescriptor*)addrPointer);

			// store endpoint numbers for IN/OUT mass storage transfers
			if (((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0x80)
			{
			    usbDevices[device].numEndpointInMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
			}
			else
			{
			    usbDevices[device].numEndpointOutMSD = ((struct usb2_endpointDescriptor*)addrPointer)->endpointAddress & 0xF;
			}
			addrPointer += 7;
			found = true;
		}

		if ( ((*(uint8_t*)(addrPointer+1)) != 2 ) && ((*(uint8_t*)(addrPointer+1)) != 4 ) && ((*(uint8_t*)(addrPointer+1)) != 5 ) ) // length, type
		{
			settextcolor(9,0);
			printf("\nlength: %d type: %d unknown\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
			settextcolor(15,0);
			addrPointer += *(uint8_t*)addrPointer;
			found = true;
		}

		if (found == false)
		{
			printf("\nlength: %d type: %d not found\n",*(uint8_t*)addrPointer,*(uint8_t*)(addrPointer+1));
			break;
		}

		settextcolor(13,0);
        printf("\n>>> Press key to go on with data analysis from config descriptor. <<<");
        settextcolor(15,0);
        while(!keyboard_getChar());
        printf("\n");
	}
}

void usbTransferString(uint32_t device)
{
    #ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: 0 languageIDs", device); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 12);  // IN DATA1, 12 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, 0, 0, 12); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, lo, index, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();

    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,12);
    #endif
	showStringDescriptor((struct usb2_stringDescriptor*)DataQTDpage0);
}

void usbTransferStringUnicode(uint32_t device, uint32_t stringIndex)
{
	#ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_DESCRIPTOR string, dev: %d endpoint: 0 stringIndex: %d", device, stringIndex); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next   = createQTD_IO(0x1,               OUT, 1,  0);  // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 64);  // IN DATA1, 64 byte
    SetupQTD = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 6, 3, stringIndex, 0x0409, 64); // SETUP DATA0, 8 byte, Device->Host, GET_DESCRIPTOR, string, stringIndex, languageID, length

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();

    #ifdef _USB_DIAGNOSIS_
	  showPacket(DataQTDpage0,64);
    #endif

	showStringDescriptorUnicode((struct usb2_stringDescriptorUnicode*)DataQTDpage0);
}

// http://www.lowlevel.eu/wiki/USB#SET_CONFIGURATION
void usbTransferSetConfiguration(uint32_t device, uint32_t configuration)
{
    //#ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: SET_CONFIGURATION %d",configuration); settextcolor(15,0);
    //#endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, IN, 1,  0); // Handshake IN directly after Setup
    SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x00, 9, 0, configuration, 0, 0); // SETUP DATA0, 8 byte, request type, SET_CONFIGURATION(9), hi(reserved), configuration, index=0, length=0

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

	performAsyncScheduler();
}

uint8_t usbTransferGetConfiguration(uint32_t device)
{
    //#ifdef _USB_DIAGNOSIS_
	  settextcolor(11,0); printf("\nUSB2: GET_CONFIGURATION"); settextcolor(15,0);
    //#endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // Create QTDs (in reversed order)
    void* next = createQTD_IO(0x1, OUT, 1,  0); // Handshake is the opposite direction of Data, therefore OUT after IN
    next = DataQTD = createQTD_IO((uintptr_t)next, IN,  1, 1);  // IN DATA1, 1 byte
	SetupQTD   = createQTD_SETUP((uintptr_t)next, 0, 8, 0x80, 8, 0, 0, 0, 1); // SETUP DATA0, 8 byte, request type, GET_CONFIGURATION(9), hi, lo, index=0, length=1

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

	performAsyncScheduler();

	uint8_t configuration = *((uint8_t*)DataQTDpage0);
	return configuration;
}

// Bulk-Only Mass Storage get maximum number of Logical Units
uint8_t usbTransferBulkOnlyGetMaxLUN(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: usbTransferBulkOnlyGetMaxLUN, dev: %d interface: %d", device, numInterface); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // bulk transfer
	// Create QTDs (in reversed order)
    void* next      = createQTD_IO(             0x1,  OUT, 1, 0); // Handshake is the opposite direction of Data
    next = DataQTD  = createQTD_IO( (uintptr_t)next, IN, 1, 1);  // IN DATA1, 1 byte
	next = SetupQTD = createQTD_MSD((uintptr_t)next, 0, 8, 0xA1, 0xFE, 0, 0, numInterface, 1);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 10100001b     11111110b 0000h  Interface 0001h     1 byte

	// Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
	return *((uint8_t*)DataQTDpage0);
}

// Bulk-Only Mass Storage Reset
void usbTransferBulkOnlyMassStorageReset(uint32_t device, uint8_t numInterface)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: usbTransferBulkOnlyMassStorageReset, dev: %d interface: %d", device, numInterface); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // bulk transfer
	// Create QTDs (in reversed order)
    void* next      = createQTD_IO(0x1,  IN, 1, 0); // Handshake is the opposite direction of Data
    next = SetupQTD = createQTD_MSD((uintptr_t)next, 0, 8, 0x21, 0xFF, 0, 0, numInterface, 0);
    // bmRequestType bRequest  wValue wIndex    wLength   Data
    // 00100001b     11111111b 0000h  Interface 0000h     none

	// Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), SetupQTD, 1, device, 0, 64); // endpoint 0

    performAsyncScheduler();
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
}

/// cf. http://www.beyondlogic.org/usbnutshell/usb4.htm#Bulk
void usbSendSCSIcmd(uint32_t device, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, bool MSDStatus)
{
    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    void* QH1              = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);
    
    // OUT qTD
    void* cmdQTD  = createQTD_IO(0x01, OUT, 0, 31); // OUT DATA0, 31 byte

    // http://en.wikipedia.org/wiki/SCSI_CDB
	struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
	memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw

	switch (SCSIcommand)
	{
	case 0x00: // test unit ready(6)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 0;
	    cbw->CBWFlags              = 0x00; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                = 0;    // only bits 3:0
	    cbw->CBWCBLength           = 6;    // only bits 4:0
		cbw->commandByte[0] = 0x00; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = 0;    // Reserved
		cbw->commandByte[3] = 0;    // Reserved
		cbw->commandByte[4] = 0;    // Reserved
		cbw->commandByte[5] = 0;    // Control
	    break;

	case 0x03: // Request Sense(6)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 0;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                = 0;    // only bits 3:0
	    cbw->CBWCBLength           = 6;    // only bits 4:0
		cbw->commandByte[0] = 0x03; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = 0;    // Reserved
		cbw->commandByte[3] = 0;    // Reserved
		cbw->commandByte[4] = 19;   // Allocation length (max. bytes)
		cbw->commandByte[5] = 0;    // Control
	    break;

	case 0x25: // read capacity(10)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 8;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                =  0; // only bits 3:0
	    cbw->CBWCBLength           = 10; // only bits 4:0
		cbw->commandByte[0] = 0x25; // Operation code
		cbw->commandByte[1] = 0;    // 7:5 LUN  4:1 reserved  0 RelAddr
		cbw->commandByte[2] = BYTE4(LBA);    // LBA MSB
		cbw->commandByte[3] = BYTE3(LBA);    // LBA
		cbw->commandByte[4] = BYTE2(LBA);    // LBA
		cbw->commandByte[5] = BYTE1(LBA);    // LBA LSB
        cbw->commandByte[6] = 0;    // Reserved
		cbw->commandByte[7] = 0;    // Reserved
		cbw->commandByte[8] = 0;    // 7:1 Reserved  0 PMI
		cbw->commandByte[9] = 0;    // Control
	    break;


	case 0x28: // read(10)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = TransferLength;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                =  0; // only bits 3:0
	    cbw->CBWCBLength           = 10; // only bits 4:0
		cbw->commandByte[0] = 0x28; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = BYTE4(LBA);    // LBA MSB
		cbw->commandByte[3] = BYTE3(LBA);    // LBA
		cbw->commandByte[4] = BYTE2(LBA);    // LBA
		cbw->commandByte[5] = BYTE1(LBA);    // LBA LSB
        cbw->commandByte[6] = 0;    // Reserved
		cbw->commandByte[7] = BYTE2(TransferLength);
		cbw->commandByte[8] = BYTE1(TransferLength);
		cbw->commandByte[9] = 0;    // Control
	    break;
	}

    // OUT QH
    createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, QH1), cmdQTD,  1, device, endpointOut, 512);
    
    // IN qTDs
    void* next = createQTD_HS(OUT); // Handshake 
    
    if (MSDStatus==true)
    {
        if (TransferLength > 0)
        {
            next = createQTD_MSDStatus((uintptr_t)next, 1); // next, toggle // IN 13 byte
            DataQTD = createQTD_IO((uintptr_t)next, IN,  0, TransferLength); // IN/OUT DATA0, ... byte
        }
        else
        {
            DataQTD = createQTD_MSDStatus((uintptr_t)next, 0); // next, toggle // IN 13 byte    
        }
    }    
    else
    { 
        if (TransferLength > 0)       
        {
            DataQTD = createQTD_IO((uintptr_t)next, IN, 0, TransferLength); // IN DATA0, ... byte
        }
        else
        {
            DataQTD = NULL; // no qTD
        }
    }
    
    // IN QH
    createQH(QH1, paging_get_phys_addr(kernel_pd, virtualAsyncList), DataQTD, 0, device, endpointIn, 512); // endpoint IN/OUT for MSD
    
    performAsyncScheduler();

	if (TransferLength)
    {
        printf("\n");
	    showPacket(DataQTDpage0,TransferLength);
	    showPacketAlphaNumeric(DataQTDpage0,TransferLength);
    }
	
    if (MSDStatus)
    {
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
        }
    }
}

int32_t usbTransferGetAnswerToCommandMSD(uint32_t device, uint32_t endpointIn)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: Command Block Wrapper Status, dev: %d endpoint: %d", device, endpointIn); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // bulk transfer
	// Create QTDs (in reversed order)
    //void* next = createQTD_IO(        0x1, OUT, 1,  0); // Handshake is the opposite direction of Data, therefore OUT after IN
    DataQTD = createQTD_IO(/*(uint32_t)next*/0x01, IN,  0, 13); // IN DATA0, 13 byte

	(*(((uint32_t*)DataQTDpage0)+0)) = 0x53425355; // magic USBS
	(*(((uint32_t*)DataQTDpage0)+1)) = 0xAAAAAAAA; // CSWTag
	(*(((uint32_t*)DataQTDpage0)+2)) = 0xAAAAAAAA; //
	(*(((uint32_t*)DataQTDpage0)+3)) = 0xFFFFFFAA; //

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), DataQTD, 1, device, endpointIn, 512); // endpoint IN for MSD

    performAsyncScheduler();

    printf("\n");
	showPacket(DataQTDpage0,16); // three bytes more (FF) for control
    settextcolor(9,0); printf("\nThis was the status answer"); settextcolor(15,0);

    int32_t retVal = 0;
    if( ( (*(((uint32_t*)DataQTDpage0)+3)) & 0x000000FF ) == 0x0 )
    {
        printf("\nCommand Passed (\"good status\") "); retVal = 0;

    }
    if( ( (*(((uint32_t*)DataQTDpage0)+3)) & 0x000000FF ) == 0x1 )
    {
        printf("\nCommand failed"); retVal = 1;
    }
    if( ( (*(((uint32_t*)DataQTDpage0)+3)) & 0x000000FF ) == 0x2 )
    {
        printf("\nPhase Error"); retVal = 2;
    }
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
    return retVal;
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
       settextcolor(10,0);
       printf("\nUSB specification: %d.%d\t\t", usbDev->usbSpec>>8, usbDev->usbSpec&0xFF);     // e.g. 0x0210 means 2.10
       printf("USB class:         %x\n",    usbDev->usbClass);
       printf("USB subclass:      %x\t",    usbDev->usbSubclass);
       printf("USB protocol       %x\n",    usbDev->usbProtocol);
       printf("max packet size:   %d\t\t",    usbDev->maxPacketSize);             // MPS0, must be 8,16,32,64
       #ifdef _USB_DIAGNOSIS_
       printf("vendor:            %x\n",    usbDev->vendor);
       printf("product:           %x\t",    usbDev->product);
       printf("release number:    %d.%d\n", usbDev->releaseNumber>>8, usbDev->releaseNumber&0xFF);  // release of the device
       printf("manufacturer:      %x\t",    usbDev->manufacturerStringID);
       printf("product:           %x\n",    usbDev->productStringID);
       printf("serial number:     %x\t",    usbDev->serNumberStringID);
       printf("number of config.: %d\n",    usbDev->numConfigurations); // number of possible configurations
       printf("numInterfaceMSD:   %d\n",    usbDev->numInterfaceMSD);
       #endif
       settextcolor(15,0);
}

void showConfigurationDescriptor(struct usb2_configurationDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);
	   printf("\n");
       #ifdef _USB_DIAGNOSIS_
	   printf("length:               %d\t\t",  d->length);
       printf("descriptor type:      %d\n",  d->descriptorType);
       #endif
	   settextcolor(7,0);
	   printf("total length:         %d\t",  d->totalLength);
       settextcolor(10,0);
       printf("number of interfaces: %d\n",  d->numInterfaces);
       #ifdef _USB_DIAGNOSIS_
       printf("ID of config:         %x\t",  d->configurationValue);
       printf("ID of config name     %x\n",  d->configuration);
       printf("remote wakeup:        %s\t",  d->attributes & (1<<5) ? "yes" : "no");
       printf("self-powered:         %s\n",  d->attributes & (1<<6) ? "yes" : "no");
       printf("max power (mA):       %d\n",  d->maxPower*2); // 2 mA steps used
       #endif
       settextcolor(15,0);
    }
}

void showInterfaceDescriptor(struct usb2_interfaceDescriptor* d)
{
    if (d->length)
    {
       settextcolor(14,0);
	   printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
	   settextcolor(10,0);
       #ifdef _USB_DIAGNOSIS_
	   printf("length:               %d\t\t",  d->length);        // 9
       printf("descriptor type:      %d\n",  d->descriptorType);    // 4
       #endif
	   printf("interface number:     %d\t\t",  d->interfaceNumber);
       #ifdef _USB_DIAGNOSIS_
       printf("alternate Setting:    %d\n",  d->alternateSetting);
       #endif
       printf("number of endpoints:  %d\n",  d->numEndpoints);
       printf("interface class:      %d\n",  d->interfaceClass);
       printf("interface subclass:   %d\n",  d->interfaceSubclass);
       printf("interface protocol:   %d\n",  d->interfaceProtocol);
       #ifdef _USB_DIAGNOSIS_
       printf("interface:            %x\n",  d->interface);
       #endif
       settextcolor(15,0);
    }
}

void showEndpointDescriptor(struct usb2_endpointDescriptor* d)
{
    if (d->length)
    {
       settextcolor(14,0);
	   printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
	   settextcolor(10,0);
       #ifdef _USB_DIAGNOSIS_
	   printf("length:            %d\t\t",  d->length);       // 7
       printf("descriptor type:   %d\n",    d->descriptorType); // 5
       #endif
	   printf("endpoint in/out:   %s\t\t",  d->endpointAddress & 0x80 ? "in" : "out");
       printf("endpoint number:   %d\n",    d->endpointAddress & 0xF);
       printf("attributes:        %y\t\t",  d->attributes); // bit 1:0 00 control    01 isochronous    10 bulk                         11 interrupt
                                                            // bit 3:2 00 no sync    01 async          10 adaptive                     11 sync (only if isochronous)
                                                            // bit 5:4 00 data endp. 01 feedback endp. 10 explicit feedback data endp. 11 reserved (Iso Mode)
       if (d->attributes == 2)
       {
           printf("\nattributes: bulk - no sync - data endpoint\n");
       }
       printf("max packet size:   %d\n",  d->maxPacketSize);
       #ifdef _USB_DIAGNOSIS_
       printf("interval:          %d\n",  d->interval);
       #endif
       settextcolor(15,0);
    }
}

void showStringDescriptor(struct usb2_stringDescriptor* d)
{
    if (d->length)
    {
       settextcolor(10,0);

       #ifdef _USB_DIAGNOSIS_
	   printf("\nlength:            %d\t\t",  d->length);     // 12
       printf("descriptor type:   %d\n",  d->descriptorType); //  3
       #endif

	   for(int i=0; i<10;i++)
       {
           if (d->languageID[i])
		   {
			   if (d->languageID[i] == 0x409)
			   {
				   printf("\nlanguage: German\t");
			   }
			   else
			   {
			       printf("\nlanguage: %x\t", d->languageID[i]);
			   }
		   }
       }
	   printf("\n");
       settextcolor(15,0);
    }
}

void showStringDescriptorUnicode(struct usb2_stringDescriptorUnicode* d)
{
    if (d->length)
    {
       settextcolor(10,0);

       #ifdef _USB_DIAGNOSIS_
	     printf("\nlength:            %d\t\t",  d->length);
         printf("descriptor type:   %d\n",  d->descriptorType); // 3
         printf("string: ");
	   #endif

	   settextcolor(14,0);
	   for(int i=0; i<(d->length-2);i+=2) // show only low value of Unicode character
       {
		   if (d->widechar[i])
		   {
               putch(d->widechar[i]);
		   }
       }
	   printf("\n");
       settextcolor(15,0);
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



/// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST /// TEST ///





/// http://en.wikipedia.org/wiki/SCSI_command
void usbTransferSCSIcommandToMSD(uint32_t device, uint32_t endpointOut, uint8_t SCSIcommand)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: Command Block Wrapper, dev: %d endpoint: %d SCSI command: %y", device, endpointOut, SCSIcommand); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    // bulk transfer
	// Create QTDs (in reversed order)
    DataQTD = createQTD_IO(0x01, OUT, 0, 31); // OUT DATA0, 31 byte

    // http://en.wikipedia.org/wiki/SCSI_CDB
	struct usb2_CommandBlockWrapper* cbw = (struct usb2_CommandBlockWrapper*)DataQTDpage0;
	memset(cbw,0,sizeof(struct usb2_CommandBlockWrapper)); // zero of cbw

	switch (SCSIcommand)
	{
	case 0x00: // test unit ready(6)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 0;
	    cbw->CBWFlags              = 0x00; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                = 0;    // only bits 3:0
	    cbw->CBWCBLength           = 6;    // only bits 4:0
		cbw->commandByte[0] = 0x00; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = 0;    // Reserved
		cbw->commandByte[3] = 0;    // Reserved
		cbw->commandByte[4] = 0;    // Reserved
		cbw->commandByte[5] = 0;    // Control
	    break;

	case 0x03: // Request Sense(6)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 0;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                = 0;    // only bits 3:0
	    cbw->CBWCBLength           = 6;    // only bits 4:0
		cbw->commandByte[0] = 0x03; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = 0;    // Reserved
		cbw->commandByte[3] = 0;    // Reserved
		cbw->commandByte[4] = 19;   // Allocation length (max. bytes)
		cbw->commandByte[5] = 0;    // Control
	    break;

	case 0x25: // read capacity(10)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 8;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                =  0; // only bits 3:0
	    cbw->CBWCBLength           = 10; // only bits 4:0
		cbw->commandByte[0] = 0x25; // Operation code
		cbw->commandByte[1] = 0;    // 7:5 LUN  4:1 reserved  0 RelAddr
		cbw->commandByte[2] = 0;    // LBA
		cbw->commandByte[3] = 0;    // LBA
		cbw->commandByte[4] = 0;    // LBA
		cbw->commandByte[5] = 0;    // LBA
        cbw->commandByte[6] = 0;    // Reserved
		cbw->commandByte[7] = 0;    // Reserved
		cbw->commandByte[8] = 0;    // 7:1 Reserved  0 PMI
		cbw->commandByte[9] = 0;    // Control
	    break;


	case 0x28: // read(10)

		cbw->CBWSignature          = 0x43425355; // magic
        cbw->CBWTag                = 0x42424242; // device echoes this field in the CSWTag field of the associated CSW
	    cbw->CBWDataTransferLength = 512;
	    cbw->CBWFlags              = 0x80; // Out: 0x00  In: 0x80
	    cbw->CBWLUN                =  0; // only bits 3:0
	    cbw->CBWCBLength           = 10; // only bits 4:0
		cbw->commandByte[0] = 0x28; // Operation code
		cbw->commandByte[1] = 0;    // Reserved
		cbw->commandByte[2] = 0;    // LBA
		cbw->commandByte[3] = 0;    // LBA
		cbw->commandByte[4] = 0;    // LBA
		cbw->commandByte[5] = 0;    // LBA
        cbw->commandByte[6] = 0;    // Reserved
		cbw->commandByte[7] = 0x02; // Transfer length
		cbw->commandByte[8] = 0x00; // Transfer length
		cbw->commandByte[9] = 0;    // Control
	    break;
	}

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), DataQTD, 1, device, endpointOut, 512);

    performAsyncScheduler();

    printf("\n");
	showPacket(DataQTDpage0,31);
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
}

/*
void usbTransferAfterSCSIcommandToMSD(uint32_t device, uint32_t endpoint, uint8_t InOut, uint32_t TransferLength)
{
    #ifdef _USB_DIAGNOSIS_
	settextcolor(11,0); printf("\nUSB2: Command Block Wrapper Transfer, dev: %d endpoint: %d", device, endpoint); settextcolor(15,0);
    #endif

    void* virtualAsyncList = malloc(sizeof(ehci_qhd_t), PAGESIZE);
    pOpRegs->ASYNCLISTADDR = paging_get_phys_addr(kernel_pd, virtualAsyncList);

    uint8_t oppositeInOut;
    if(InOut==OUT){oppositeInOut=IN; }
    else          {oppositeInOut=OUT;}

    // bulk transfer
	// Create QTDs (in reversed order) // TODO: is handshake needed here?
    //void* next = createQTD_IO(        0x1, oppositeInOut, 1, 0             ); // Handshake is the opposite direction of Data, therefore OUT after IN
    DataQTD = createQTD_IO(0x01, InOut,   0, TransferLength); // IN/OUT DATA0, ... byte

    // Create QH
	createQH(virtualAsyncList, paging_get_phys_addr(kernel_pd, virtualAsyncList), DataQTD, 1, device, endpoint, 512); // endpoint IN/OUT for MSD

    performAsyncScheduler();

    printf("\n");
	showPacket(DataQTDpage0,TransferLength);
	showPacketAlphaNumeric(DataQTDpage0,TransferLength);
	printf("\n''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''");
}
*/
