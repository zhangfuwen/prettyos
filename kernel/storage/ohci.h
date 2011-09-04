#ifndef OHCI_H
#define OHCI_H

#include "os.h"
#include "pci.h"
#include "devicemanager.h"

#define OHCIMAX      8  // max number of OHCI devices
#define OHCIPORTMAX  4  // max number of OHCI device ports

#define OHCI_HCCA_ALIGN 0x100

// control
#define OHCI_CTRL_CBSR (BIT(0)|BIT(1)) // relation between control und bulk (cbsr+1 vs. 1)
#define OHCI_CTRL_PLE   BIT(2)        // activate periodical transfers
#define OHCI_CTRL_IE    BIT(3)        // activate isochronous transfers
#define OHCI_CTRL_CLE   BIT(4)        // activate control transfers
#define OHCI_CTRL_BLE   BIT(5)        // activate bulk transfers
#define OHCI_CTRL_HCFS (BIT(6)|BIT(7)) // HC status
#define OHCI_CTRL_IR    BIT(8)        // redirect IRQ to SMB
#define OHCI_CTRL_RWC   BIT(9)        // remote wakeup
#define OHCI_CTRL_RW    BIT(10)       // activate remote wakeup

// Interrupts
#define OHCI_INT_SO     BIT(0)        // scheduling overrun
#define OHCI_INT_WDH    BIT(1)        // HcDoneHead writeback
#define OHCI_INT_SF     BIT(2)        // start of frame
#define OHCI_INT_RD     BIT(3)        // resume detect
#define OHCI_INT_UE     BIT(4)        // unrecoverable error
#define OHCI_INT_FNO    BIT(5)        // frame number overflow
#define OHCI_INT_RHSC   BIT(6)        // root hub status change
#define OHCI_INT_OC     BIT(30)       // ownership change
#define OHCI_INT_MIE    BIT(31)       // master interrupt enable

// Command Status
#define OHCI_STATUS_RESET BIT(0)        // reset
#define OHCI_STATUS_CLF   BIT(1)        // control list filled
#define OHCI_STATUS_BLF   BIT(2)        // bulk list filled
#define OHCI_STATUS_OCR   BIT(3)        // ownership change request
#define OHCI_STATUS_SOC  (BIT(6)|BIT(7)) // scheduling overrun count

//
#define OHCI_RHA_NDP    0x000000FF // number downstream rootports (max. 15)
#define OHCI_RHA_PSM    BIT(8)     //
#define OHCI_RHA_NPS    BIT(9)     //
#define OHCI_RHA_DT     BIT(10)    // always 0
#define OHCI_RHA_OCPM   BIT(11)    //
#define OHCI_RHA_NOCP   BIT(12)    //
#define OHCI_RHA_POTPGT 0xFF000000 //

//
#define OHCI_RHS_LPS    BIT(0)     //
#define OHCI_RHS_OCI    BIT(1)     //
#define OHCI_RHS_DRWE   BIT(15)    //
#define OHCI_RHS_LPSC   BIT(16)    //
#define OHCI_RHS_OCIC   BIT(17)    //
#define OHCI_RHS_CRWE   BIT(31)    //

// Port 
#define OHCI_PORT_CCS   BIT(0)     //
#define OHCI_PORT_PES   BIT(1)     //
#define OHCI_PORT_PSS   BIT(2)     //
#define OHCI_PORT_POCI  BIT(3)     //
#define OHCI_PORT_PRS   BIT(4)     //
#define OHCI_PORT_PPS   BIT(8)     //
#define OHCI_PORT_LSDA  BIT(9)     //
#define OHCI_PORT_CSC   BIT(16)    //
#define OHCI_PORT_PESC  BIT(17)    //
#define OHCI_PORT_PSSC  BIT(18)    //
#define OHCI_PORT_OCIC  BIT(19)    //
#define OHCI_PORT_PRSC  BIT(20)    //
#define OHCI_PORT_WTC	(OHCI_PORT_CSC|OHCI_PORT_PESC|OHCI_PORT_PSSC|OHCI_PORT_OCIC|OHCI_PORT_PRSC)

// USB
#define OHCI_USB_RESET       0
#define OHCI_USB_RESUME      BIT(6)
#define OHCI_USB_OPERATIONAL BIT(7)
#define OHCI_USB_SUSPEND    (BIT(6)|BIT(7))


// OHCI operational registers
typedef struct
{
    uint32_t HcRevision;          // BCD version of HCI spec implemented by this HC
    uint32_t HcControl;           // operating modes for the HC
    uint32_t HcCommandStatus;     // current status of the HC
    uint32_t HcInterruptStatus;   // hardware interrupts
    uint32_t HcInterruptEnable;   // enabled intterupts
    uint32_t HcInterruptDisable;  // disabled intterupts
    uint32_t HcHCCA;              // physical address of the HC Communication Area
    uint32_t HcPeriodCurrentED;   // physical address of the current Isochronous or Interrupt Endpoint Descriptor
    uint32_t HcControlHeadED;     // physical address of the first Endpoint Descriptor of the Control list
    uint32_t HcControlCurrentED;  // physical address of the current Endpoint Descriptor of the Control list
    uint32_t HcBulkHeadED;        // physical address of the first Endpoint Descriptor of the Bulk list
    uint32_t HcBulkCurrentED;     // physical address of the current endpoint of the Bulk list
    uint32_t HcDoneHead;          // physical address of the last completed Transfer Descriptor that was added to the Done queue
    uint32_t HcFmInterval;        // 14-bit: bit time interval in a Frame, 15-bit: Full Speed maximum packet size
    uint32_t HcFmRemaining;       // 14-bit down counter showing the bit time remaining in the current Frame
    uint32_t HcFmNumber;          // 16-bit counter
    uint32_t HcPeriodicStart;     // 14-bit: earliest time HC should start processing the periodic list
    uint32_t HcLSThreshold;       // 11-bit: HC determines whether to commit to the transfer of a maximum of 8-byte LS packet before EOF
    uint32_t HcRhDescriptorA;     // characteristics of the Root Hub
    uint32_t HcRhDescriptorB;     // characteristics of the Root Hub
    uint32_t HcRhStatus;          // lower word: Root Hub Status field, upper word: Root Hub Status Change field
    uint32_t HcRhPortStatus[8];   // port events on a per-port basis, max. NDP = 15, we assume 8 (taken from VBox)
} __attribute__((packed)) ohci_OpRegs_t;

/*
HCCA is a 256-byte structure of system memory used to send and receive control/status information
to and from the HC. This structure must be located on a 256-byte boundary.
Write the address of this structure in HcHCCA in the HC OpRegs.
*/
typedef struct
{
    uint32_t interrruptTable[32]; // pointers to interrupt EDs
    uint16_t frameNumber;         // current frame number
    uint16_t pad1;                // when the HC updates frameNumber, it sets this word to 0
    uint32_t doneHead;            // holds at frame end the current value of HcDoneHead, and interrupt is sent
    uint8_t  reserved[116];
 } __attribute__((packed)) ohci_HCCA_t;


// OHCI device
typedef struct
{
    pciDev_t*      PCIdevice;         // PCI device
    uintptr_t      bar;               // MMIO space (base address register)

    ohci_OpRegs_t* OpRegs;           // operational registers
    ohci_HCCA_t*   hcca;              // HC Communications Area

    uint8_t        rootPorts;         // number of rootports
    size_t         memSize;           // memory size of IO space
    bool           enabledPorts;      // root ports enabled
    port_t         port[OHCIPORTMAX]; // root ports
    bool           run;               // hc running (RS bit)
    uint8_t        num;               // number of the OHCI
} ohci_t;


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize);
void ohci_pollDisk(void* dev);
void ohci_initHC(ohci_t* o);
void ohci_resetHC(ohci_t* o);


#endif
