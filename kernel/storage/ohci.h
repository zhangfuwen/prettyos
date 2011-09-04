#ifndef OHCI_H
#define OHCI_H

#include "os.h"
#include "pci.h"
#include "devicemanager.h"

#define OHCIMAX      8  // max number of OHCI devices
#define OHCIPORTMAX  4  // max number of OHCI device ports

// control
#define OHCI_CTRL_CBSR  0x00000003 // relation between control und bulk (cbsr+1 vs. 1)
#define OHCI_CTRL_PLE   0x00000004 // activate periodical transfers
#define OHCI_CTRL_IE    0x00000008 // activate isochronous transfers
#define OHCI_CTRL_CLE   0x00000010 // activate control transfers
#define OHCI_CTRL_BLE   0x00000020 // activate bulk transfers
#define OHCI_CTRL_HCFS  0x000000C0 // HC status
#define OHCI_CTRL_IR    0x00000100 // redirect IRQ to SMB
#define OHCI_CTRL_RWC   0x00000200 // remote wakeup
#define OHCI_CTRL_RW    0x00000400 // activate remote wakeup

// Interrupts
#define OHCI_INT_SO     0x00000001 // scheduling overrun
#define OHCI_INT_WDH    0x00000002 // write back done head
#define OHCI_INT_SF     0x00000004 // start of frame
#define OHCI_INT_RD     0x00000008 // resume detected
#define OHCI_INT_UE     0x00000010 // unrecoverable error
#define OHCI_INT_FNO    0x00000020 // frame number overflow
#define OHCI_INT_RHSC   0x00000040 // root hub status change
#define OHCI_INT_OC     0x40000000 // ownership change
#define OHCI_INT_MIE    0x80000000 // (de)activates interrupts

// Command Status
#define OHCI_CMST_RESET 0x00000001 // reset
#define OHCI_CMST_CLF   0x00000002 // control list filled
#define OHCI_CMST_BLF   0x00000004 // bulk list filled
#define OHCI_CMST_OCR   0x00000008 // ownership change request
#define OHCI_CMST_SOC   0x00030000 // scheduling overrun count

#define OHCI_RHA_NDP    0x000000FF // number downstream rootports (max. 15)
#define OHCI_RHA_PSM    0x00000100 //
#define OHCI_RHA_NPS    0x00000200 //
#define OHCI_RHA_DT     0x00000400 // always 0
#define OHCI_RHA_OCPM   0x00000800 //
#define OHCI_RHA_NOCP   0x00001000 //
#define OHCI_RHA_POTPGT 0xFF000000 //
#define OHCI_RHS_LPS    0x00000001 //
#define OHCI_RHS_OCI    0x00000002 //
#define OHCI_RHS_DRWE   0x00008000 //
#define OHCI_RHS_LPSC   0x00010000 //
#define OHCI_RHS_OCIC   0x00020000 //
#define OHCI_RHS_CRWE   0x80000000 //
#define OHCI_RP_CCS     0x00000001 //
#define OHCI_RP_PES     0x00000002 //
#define OHCI_RP_PSS     0x00000004 //
#define OHCI_RP_POCI    0x00000008 //
#define OHCI_RP_PRS     0x00000010 //
#define OHCI_RP_PPS     0x00000100 //
#define OHCI_RP_LSDA    0x00000200 //
#define OHCI_RP_CSC     0x00010000 //
#define OHCI_RP_PESC    0x00020000 //
#define OHCI_RP_PSSC    0x00040000 //
#define OHCI_RP_OCIC    0x00080000 //
#define OHCI_RP_PRSC    0x00100000 //

// USB
#define OHCI_USB_RESET       0x00000000
#define OHCI_USB_RESUME      0x00000040
#define OHCI_USB_OPERATIONAL 0x00000080
#define OHCI_USB_SUSPEND     0x000000C0


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

    ohci_OpRegs_t* pOpRegs;           // operational registers
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
