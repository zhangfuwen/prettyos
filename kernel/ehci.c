/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "os.h"
#include "pci.h"
#include "ehci.h"
#include "kheap.h"
#include "paging.h"
#include "sys_speaker.h"
#include "usb2.h"

// pci devices list
extern pciDev_t pciDev_Array[PCIARRAYSIZE];

void createQH(void* address, void* firstQTD, uint32_t device)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: createQH\n");
	settextcolor(15,0);

	struct ehci_qhd* head = (struct ehci_qhd*)address;
	memset(address, 0, sizeof(struct ehci_qhd));
	uint32_t phys = paging_get_phys_addr(kernel_pd, address);
	head->horizontalPointer      =   phys | 0x2;
	head->deviceAddress          =   device; // The device address
	head->inactive               =   0;
	head->endpoint               =   0;	// Endpoint 0 contains Device infos such as name
	head->endpointSpeed          =   2;	// 00b = full speed; 01b = low speed; 10b = high speed
	head->dataToggleControl      =   1;	// Get the Data Toggle bit out of the included qTD
	head->H                      =   1;
	head->maxPacketLength        =  64;	// It's 64 bytes for a control transfer to a high speed device.
	head->controlEndpointFlag    =   0;	// Only used if Endpoint is a control endpoint and not high speed
	head->nakCountReload         =   0;	// This value is used by HC to reload the Nak Counter field.
	head->interruptScheduleMask  =   0;	// not used for async schedule
	head->splitCompletionMask    =   0;	// unused if (not low/full speed and in periodic schedule)
	head->hubAddr                =   0;	// unused if high speed (Split transfer)
	head->portNumber             =   0;	// unused if high speed (Split transfer)
	head->mult                   =   1;	// One transaction to be issued for this endpoint per micro-frame.
                                        // Maybe unused for non interrupt queue head in async list
	uint32_t physNext = paging_get_phys_addr(kernel_pd, firstQTD);
	head->qtd.next = physNext;
}

void* createQTD(uint32_t next, uint8_t pid, bool toggle, uint32_t tokenBytes)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: createQTD\n");
	settextcolor(15,0);

	void* address = malloc(sizeof(struct ehci_qtd), PAGESIZE); // Can be changed to 32 Byte alignment
	struct ehci_qtd* td = (struct ehci_qtd*)address;

	if(next != 0x1)
	{
		uint32_t phys = paging_get_phys_addr(kernel_pd, (void*)next);
		td->next = phys;
	}
	else
	{
		td->next = 0x1;
	}
	td->nextAlt = td->next; /// 0x1;	// No alternative next, so T-Bit is set to 1
	td->token.status       = 0x80;	 // This will be filled by the Host Controller
	td->token.pid          = pid;	 // Setup Token
	td->token.errorCounter = 0x3;    // Written by the Host Controller.
	td->token.currPage     = 0x0;	 // Start with first page. After that it's written by Host Controller???
	td->token.interrupt    = 0x1;	 // We want an interrupt after complete transfer
	td->token.bytes        = tokenBytes; // dependent on transfer
	td->token.dataToggle   = toggle; // Should be toggled every list entry

	void* data = malloc(0x20, PAGESIZE);	// Enough for a full page
	memset(data,0,0x20);



	if(pid == 0x2) // SETUP
	{
	    SetupQTDpage0  = (uint32_t)data;

	    struct ehci_request* request = (struct ehci_request*)data;
		request->type    = 0x80;	// Device->Host
		request->request =  0x6;	// GET_DESCRIPTOR
		request->valueHi =    1;	// Type:1 (Device)
		request->valueLo =    0;	// Index: 0, used only for String or Configuration descriptors
		request->index   =    0;	// Language ID: Default
		request->length  =   18;    // according to the the requested data bytes in IN qTD
	}
	else if(pid == 0x1)// IN
	{
		InQTDpage0  = (uint32_t)data;
		inBuffer = data;
	}

	uint32_t dataPhysical = paging_get_phys_addr(kernel_pd, data);
	td->buffer0 = dataPhysical;
	td->buffer1 = 0x0;
	td->buffer2 = 0x0;
	td->buffer3 = 0x0;
	td->buffer4 = 0x0;

	return address;
}

void showPacket(uint32_t virtAddrBuf0, uint32_t size)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: showPacket\n");
	settextcolor(15,0);

    printformat("virtAddrBuf0: %X\n", virtAddrBuf0);
    for(uint32_t c=0; c<size; c++)
    {
        settextcolor(3,0);
        printformat("%y ", *((uint8_t*)virtAddrBuf0+c));
        settextcolor(15,0);
    }
}

void showStatusbyteQTD(void* addressQTD)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: showStatusbyteQTD\n");
	settextcolor(15,0);

    uint8_t statusbyte = *((uint8_t*)addressQTD+8);
    printformat("QTD: %X Statusbyte: %y", addressQTD, statusbyte);

    // analyze status byte (cf. EHCI 1.0 spec, Table 3-16 Status in qTD Token)
    settextcolor(14,0);
    if( statusbyte & (1<<7) )
    {
        printformat("\nqTD Status: Active - HC transactions enabled");
    }
    if( statusbyte & (1<<6) )
    {
        printformat("\nqTD Status: Halted - serious error at the device/endpoint");
    }
    if( statusbyte & (1<<5) )
    {
        printformat("\nqTD Status: Data Buffer Error (overrun or underrun)");
    }
    if( statusbyte & (1<<4) )
    {
        printformat("\nqTD Status: Babble (fatal error leads to Halted)");
    }
    if( statusbyte & (1<<3) )
    {
        printformat("\nqTD Status: Transaction Error (XactErr)- host received no valid response device");
    }
    if( statusbyte & (1<<2) )
    {
        printformat("\nqTD Status: Missed Micro-Frame");
    }
    if( statusbyte & (1<<1) )
    {
        printformat("\nqTD Status: Do Complete Split");
    }
    if( statusbyte & (1<<0) )
    {
        printformat("\nqTD Status: Do Ping");
    }
    if( statusbyte == 0 )
    {
        printformat("\n");
    }
	settextcolor(15,0);
}

void ehci_handler(struct regs* r)
{
    if( !(pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER) )
    {
      delay(2000000);settextcolor(9,0);
      printformat("\n>>> >>> function: ehci_handler: ");
	  settextcolor(15,0);
    }

	settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )
    {
        printformat("\nUSB Interrupt");
        pOpRegs->USBSTS |= STS_USBINT;
    }

    if( pOpRegs->USBSTS & STS_USBERRINT )
    {
        printformat("\nUSB Error Interrupt");
        pOpRegs->USBSTS |= STS_USBERRINT;
    }

    if( pOpRegs->USBSTS & STS_PORT_CHANGE )
    {
        settextcolor(9,0);
        printformat("Port Change");
        settextcolor(15,0);

        pOpRegs->USBSTS |= STS_PORT_CHANGE;
        showPORTSC();
        checkPortLineStatus();
    }

    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER )
    {
        //printformat("\nFrame List Rollover Interrupt");
        pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;
    }

    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )
    {
        settextcolor(4,0);
        printformat("\nHost System Error");
        settextcolor(15,0);
        pOpRegs->USBCMD &= ~CMD_ASYNCH_ENABLE; // necessary?
        pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;
        settextcolor(14,0);
        printformat("\nInit EHCI after fatal error");
        settextcolor(15,0);
        int32_t retVal = initEHCIHostController(pciEHCINumber);
        if(retVal==-1)
        {
            goto leave_handler;
        }
    }

    if( pOpRegs->USBSTS & STS_ASYNC_INT )
    {
        printformat("\nInterrupt on Async Advance");
        pOpRegs->USBSTS |= STS_ASYNC_INT;
    }
leave_handler:
	settextcolor(15,0);
}

void analyzeEHCI(uint32_t bar)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: analyzeEHCI\n");
	settextcolor(15,0);

    ubar = bar;
    pCapRegs = (struct ehci_CapRegs*) ubar;
	pOpRegs  = (struct ehci_OpRegs*) (ubar + pCapRegs->CAPLENGTH);
    numPorts = (pCapRegs->HCSPARAMS & 0x000F);

    printformat("HCIVERSION: %x ", pCapRegs->HCIVERSION);                // Interface Version Number
    printformat("HCSPARAMS: %X ", pCapRegs->HCSPARAMS);                  // Structural Parameters
    printformat("Ports: %d ", numPorts);
    printformat("\nHCCPARAMS: %X ", pCapRegs->HCCPARAMS);                // Capability Parameters
    if(BYTE2(pCapRegs->HCCPARAMS)==0) printformat("No ext. capabil. ");  // Extended Capabilities Pointer
    printformat("\nOpRegs Address: %X ", pOpRegs);                       // Host Controller Operational Registers
}

void startHostController()
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: startHostController\n");
	settextcolor(15,0);

    settextcolor(14,0);
    printformat("\nstarting HC\n");
    settextcolor(15,0);
    pOpRegs->USBCMD &= ~CMD_RUN_STOP; // set Run-Stop-Bit to 0
    delay(30000); //sleepMilliSeconds(30); // wait at least 16 microframes ( = 16*125 ?s = 2 ms )
    pOpRegs->USBCMD |= CMD_HCRESET;  // set Reset-Bit to 1

    int32_t timeout=10;
    while( (pOpRegs->USBCMD & CMD_HCRESET) != 0 ) // Reset-Bit still set to 1
    {
        printformat("waiting for HC reset\n");
        delay(20000); //sleepMilliSeconds(20);
        timeout--;
        if(timeout<=0)
        {
            settextcolor(4,0);
            printformat("Error: HC Reset-Bit still set to 1\n");
            settextcolor(15,0);
            break;
        }
    }

    pOpRegs->USBINTR = 0; // EHCI interrupts disabled

    // Program the CTRLDSSEGMENT register with 4-Gigabyte segment where all of the interface data structures are allocated.
    pOpRegs->CTRLDSSEGMENT = 0;

    // Turn the host controller ON via setting the Run/Stop bit. Software must not write a one to this field unless the host controller is in the Halted state
    pOpRegs->USBCMD |= CMD_8_MICROFRAME;
    if( pOpRegs->USBSTS & STS_HCHALTED )
    {
        pOpRegs->USBCMD |= CMD_RUN_STOP; // set Run-Stop-Bit
    }

    // Write a 1 to CONFIGFLAG register to route all ports to the EHCI controller
    pOpRegs->CONFIGFLAG = CF;

    // Write the appropriate value to the USBINTR register to enable the appropriate interrupts.
    pOpRegs->USBINTR = STS_INTMASK; // all interrupts allowed  // ---> VMWare works!

	delay(100000); //sleepMilliSeconds(100);
}

void enablePorts()
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: enablePorts\n");
	settextcolor(15,0);

    for(uint8_t j=0; j<numPorts; j++)
    {
         resetPort(j);

         if( pOpRegs->PORTSC[j] == 0x1005 ) // high speed idle, enabled, SE0
         {
             settextcolor(14,0);
             printformat("Port %d: %s\n",j+1,"high speed idle, enabled, SE0");
             settextcolor(15,0);

             testTransfer(0,j+1); // device address, port number

             printformat("\nsetup packet: "); showPacket(SetupQTDpage0,8);
             printformat("\nsetup status: "); showStatusbyteQTD(SetupQTD);
             printformat("\nin    status: "); showStatusbyteQTD(InQTD);
         }
    }
}

void resetPort(uint8_t j)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: resetPort\n");
	settextcolor(15,0);

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
     if( !(pOpRegs->USBSTS & STS_HCHALTED) ) // TEST
     {
         settextcolor(2,0);
         printformat("\nHCHalted set to 0 (OK)");
         settextcolor(15,0);
     }
     else
     {
         settextcolor(4,0);
         printformat("\nHCHalted set to 1 (Not OK!)");
         showUSBSTS();
         settextcolor(15,0);
     }

     printformat("\nstart port reset sequence");
     pOpRegs->USBINTR = 0;

     pOpRegs->PORTSC[j] |=  PSTS_PORT_RESET; // start reset sequence
     delay(250000);                          // wait
     pOpRegs->PORTSC[j] &= ~PSTS_PORT_RESET; // stop reset sequence

     // wait and check, whether really zero
     uint32_t timeout=20;
     while((pOpRegs->PORTSC[j] & PSTS_PORT_RESET) != 0)
     {
         delay(20000);
         timeout--;
         if( timeout <= 0 )
         {
             settextcolor(4,0);
             printformat("\nerror: port %d did not reset! ",j+1);
             settextcolor(15,0);
             printformat("PortStatus: %X",pOpRegs->PORTSC[j]);
             break;
         }
     }
     printformat("\nport reset sequence successful\n");
     pOpRegs->USBINTR = STS_INTMASK;
}

int32_t initEHCIHostController(uint32_t num)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: initEHCIHostController\n");
	settextcolor(15,0);

    static uint32_t timeout = 3;
    irq_install_handler(32 + pciDev_Array[num].irq, ehci_handler);
    irq_install_handler(32 + pciDev_Array[num].irq-1, ehci_handler); /// work-around for VirtualBox Bug!

    DeactivateLegacySupport(num);

    startHostController();

    if( !(pOpRegs->USBSTS & STS_HCHALTED) ) // TEST
    {
         settextcolor(2,0);
         printformat("\nHCHalted set to 0 (OK)");
         enablePorts();
         settextcolor(15,0);
    }
    else
    {
         settextcolor(4,0);
         printformat("\nHCHalted set to 1 (Not OK!) --> Ports cannot be enabled");
         showUSBSTS();
         if(timeout-- <=0)
         {
             return -1;
         }
         else
         {
             initEHCIHostController(pciEHCINumber);
         }
         settextcolor(15,0);
    }
    return 0;
}

void showPORTSC()
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: showPORTSC\n");
	settextcolor(15,0);

    for(uint8_t j=0; j<numPorts; j++)
    {
        if( pOpRegs->PORTSC[j] & PSTS_CONNECTED_CHANGE )
        {
            char str[80], PortNumber[5], PortStatus[40];
            itoa(j+1,PortNumber);

            if( pOpRegs->PORTSC[j] & PSTS_CONNECTED )
            {
                strcpy(PortStatus,"Device attached");
                resetPort(j);
            }
            else
            {
                strcpy(PortStatus,"Device not attached");
            }
            pOpRegs->PORTSC[j] |= PSTS_CONNECTED_CHANGE; // reset interrupt

            str[0]='\0';
            strcpy(str,"Port ");
            strcat(str,PortNumber);
            strcat(str," Status: ");
            strcat(str,PortStatus);
            uint8_t color = 14;
            printf("                                                                              ",46,color);
            printf(str, 46, color); // output to info screen area
            printf("                                                                              ",47,color);
            printf("                                                                              ",48,color);

            beep(1000,100);
        }
    }
}

void showUSBSTS()
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: showUSBSTS\n");
	settextcolor(15,0);

    settextcolor(15,0);
    printformat("\nUSB status: ");
    settextcolor(2,0);
    printformat("%X",pOpRegs->USBSTS);
    settextcolor(14,0);
    if( pOpRegs->USBSTS & STS_USBINT )             { printformat("\nUSB Interrupt");                 pOpRegs->USBSTS |= STS_USBINT;              }
    if( pOpRegs->USBSTS & STS_USBERRINT )          { printformat("\nUSB Error Interrupt");           pOpRegs->USBSTS |= STS_USBERRINT;           }
    if( pOpRegs->USBSTS & STS_PORT_CHANGE )        { printformat("\nPort Change Detect");            pOpRegs->USBSTS |= STS_PORT_CHANGE;         }
    if( pOpRegs->USBSTS & STS_FRAMELIST_ROLLOVER ) { printformat("\nFrame List Rollover");           pOpRegs->USBSTS |= STS_FRAMELIST_ROLLOVER;  }
    if( pOpRegs->USBSTS & STS_HOST_SYSTEM_ERROR )  { printformat("\nHost System Error");             pOpRegs->USBSTS |= STS_HOST_SYSTEM_ERROR;   }
    if( pOpRegs->USBSTS & STS_ASYNC_INT )          { printformat("\nInterrupt on Async Advance");    pOpRegs->USBSTS |= STS_ASYNC_INT;           }
    if( pOpRegs->USBSTS & STS_HCHALTED )           { printformat("\nHCHalted");                      pOpRegs->USBSTS |= STS_HCHALTED;            }
    if( pOpRegs->USBSTS & STS_RECLAMATION )        { printformat("\nReclamation");                   pOpRegs->USBSTS |= STS_RECLAMATION;         }
    if( pOpRegs->USBSTS & STS_PERIODIC_ENABLED )   { printformat("\nPeriodic Schedule Status");      pOpRegs->USBSTS |= STS_PERIODIC_ENABLED;    }
    if( pOpRegs->USBSTS & STS_ASYNC_ENABLED )      { printformat("\nAsynchronous Schedule Status");  pOpRegs->USBSTS |= STS_ASYNC_ENABLED;       }
    settextcolor(15,0);
}

void checkPortLineStatus()
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: checkPortLineStatus\n");
	settextcolor(15,0);

    settextcolor(14,0);
    printformat("\n\n>>> Status of USB Ports <<<");

    for(uint8_t j=0; j<numPorts; j++)
    {
      //check line status
      settextcolor(11,0);
      printformat("\nport %d: %X, line: %y  ",j+1,pOpRegs->PORTSC[j],(pOpRegs->PORTSC[j]>>10)&3);
      settextcolor(14,0);
      if( ((pOpRegs->PORTSC[j]>>10)&3) == 0) // SE0
      {
        settextcolor(11,0);
        printformat("SE0");
        if( pOpRegs->PORTSC[j] == 0x1005 ) // high speed idle, enabled, SE0
        {
             settextcolor(14,0);
             printformat(" ,high speed, enabled");
             settextcolor(3,0);
             printformat("\nport status: %x\t",pOpRegs->PORTSC[j]);
             settextcolor(15,0);
             testTransfer(0,j+1); // device address, port number
             printformat("\nsetup packet: "); showPacket(SetupQTDpage0,8);
             printformat("\nsetup:        "); showStatusbyteQTD(SetupQTD);
             printformat("in:             "); showStatusbyteQTD(InQTD);
        }
      }

      if( ((pOpRegs->PORTSC[j]>>10)&3) == 1) // K_STATE
      {
        settextcolor(14,0);
        printformat("K-State");
      }

      if( ((pOpRegs->PORTSC[j]>>10)&3) == 2) // J_STATE
      {
        settextcolor(14,0);
        printformat("J-state");
      }

      if( ((pOpRegs->PORTSC[j]>>10)&3) == 3) // undefined
      {
        settextcolor(12,0);
        printformat("undefined");
      }
    }
    settextcolor(15,0);
}

void DeactivateLegacySupport(uint32_t num)
{
    delay(2000000);settextcolor(9,0);
    printformat("\n>>> >>> function: DeactivateLegacySupport\n");
	settextcolor(15,0);

	bool failed = false;

    // pci bus data
    uint8_t bus  = pciDev_Array[num].bus;
    uint8_t dev  = pciDev_Array[num].device;
    uint8_t func = pciDev_Array[num].func;

    eecp = BYTE2(pCapRegs->HCCPARAMS);
    printformat("\nDeactivateLegacySupport: eecp = %x\n",eecp);
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

    //               eecp       // RO  - This field identifies the extended capability.
                                //       01h identifies the capability as Legacy Support.
    uint32_t NextEHCIExtCapPtr; // RO  - 00h indicates end of the ext. cap. list.

    if (eecp >= 0x40)
    {
        int32_t eecp_id=0;
        while(eecp)
        {
            printformat("eecp = %x, ",eecp);
            eecp_id = pci_config_read( bus, dev, func, 0x0100/*length 1 byte*/ | (eecp + 0) );
            printformat("eecp_id = %x\n",eecp_id);
            if(eecp_id == 1)
                 break;
            NextEHCIExtCapPtr = eecp + 1;
            eecp = pci_config_read( bus, dev, func, 0x0100 | NextEHCIExtCapPtr );
        }
        uint32_t BIOSownedSemaphore = eecp + 2; // R/W - only Bit 16 (Bit 23:17 Reserved, must be set to zero)
        uint32_t OSownedSemaphore   = eecp + 3; // R/W - only Bit 24 (Bit 31:25 Reserved, must be set to zero)
        uint32_t USBLEGCTLSTS       = eecp + 4; // USB Legacy Support Control/Status (DWORD, cf. EHCI 1.0 spec, 2.1.8)

        // Legacy-Support-EC found? BIOS-Semaphore set?
        if( (eecp_id == 1) && ( pci_config_read( bus, dev, func, 0x0100 | BIOSownedSemaphore ) & 0x01) )
        {
            printformat("set OS-Semaphore.\n");
            pci_config_write_byte( bus, dev, func, OSownedSemaphore, 0x01 );
            failed = true;

            int32_t timeout=200;
            // Wait for BIOS-Semaphore being not set
            while( ( pci_config_read( bus, dev, func, 0x0100 | BIOSownedSemaphore ) & 0x01 ) && ( timeout>0 ) )
            {
                printformat(".");
                timeout--;
                delay(20000);
            }
            if( !( pci_config_read( bus, dev, func, 0x0100 | BIOSownedSemaphore ) & 0x01) ) // not set
            {
                printformat("BIOS-Semaphore being not set.\n");
                timeout=200;
                while( !( pci_config_read( bus, dev, func, 0x0100 | OSownedSemaphore ) & 0x01) && (timeout>0) )
                {
                    printformat(".");
                    timeout--;
                    delay(20000);
                }
            }
            if( pci_config_read( bus, dev, func, 0x0100 | OSownedSemaphore ) & 0x01 )
            {
                failed = false;
                printformat("OS-Semaphore being set.\n");
            }
            if(failed==false)
            {
                /*
                USB SMI Enable R/W. 0=Default.
                When this bit is a one, and the SMI on USB Complete bit (above) in this register is a one,
                the host controller will issue an SMI immediately.
                */
                pci_config_write_dword( bus, dev, func, USBLEGCTLSTS, 0x0 ); // USB SMI disabled

                printformat("BIOSownedSemaphore: %d OSownedSemaphore: %d\n",
                             pci_config_read( bus, dev, func, 0x0100 | BIOSownedSemaphore ),
                             pci_config_read( bus, dev, func, 0x0100 | OSownedSemaphore   ) );
                settextcolor(2,0);
                printformat("Legacy Support Deactivated.\n");
                settextcolor(15,0);
            }
            else
            {
                settextcolor(4,0);
                printformat("Legacy Support Deactivated failed.\n");
                printformat("BIOSownedSemaphore: %d OSownedSemaphore: %d\n",
                             pci_config_read( bus, dev, func, 0x0100 | BIOSownedSemaphore ),
                             pci_config_read( bus, dev, func, 0x0100 | OSownedSemaphore   ) );
                settextcolor(15,0);
            }
        }
    }
    else
    {
        printformat("No valid eecp found.\n");
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
