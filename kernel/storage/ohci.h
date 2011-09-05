#ifndef OHCI_H
#define OHCI_H

#include "os.h"
#include "pci.h"
#include "devicemanager.h"

#define OHCIMAX      4  // max number of OHCI devices
#define OHCIPORTMAX  8  // max number of OHCI device ports

#define OHCI_HCCA_ALIGN 0x100

// HcControl Register
#define OHCI_CTRL_CBSR (BIT(0)|BIT(1))   // relation between control und bulk (cbsr+1 vs. 1)
#define OHCI_CTRL_PLE   BIT(2)           // activate periodical transfers
#define OHCI_CTRL_IE    BIT(3)           // activate isochronous transfers
#define OHCI_CTRL_CLE   BIT(4)           // activate control transfers
#define OHCI_CTRL_BLE   BIT(5)           // activate bulk transfers
#define OHCI_CTRL_HCFS (BIT(6)|BIT(7))   // HostControllerFunctionalState for USB
#define OHCI_CTRL_IR    BIT(8)           // redirect IRQ to SMB
#define OHCI_CTRL_RWC   BIT(9)           // remote wakeup
#define OHCI_CTRL_RW    BIT(10)          // activate remote wakeup

// HcCommandStatus Register
#define OHCI_STATUS_RESET BIT(0)         // reset
#define OHCI_STATUS_CLF   BIT(1)         // control list filled
#define OHCI_STATUS_BLF   BIT(2)         // bulk list filled
#define OHCI_STATUS_OCR   BIT(3)         // ownership change request
#define OHCI_STATUS_SOC  (BIT(16)|BIT(17))  // scheduling overrun count

// HcInterruptStatus
// HcInterruptEnable, HcInterruptDisable Register
#define OHCI_INT_SO     BIT(0)           // scheduling overrun
#define OHCI_INT_WDH    BIT(1)           // writeback DoneHead
#define OHCI_INT_SF     BIT(2)           // start of frame
#define OHCI_INT_RD     BIT(3)           // resume detected
#define OHCI_INT_UE     BIT(4)           // unrecoverable error
#define OHCI_INT_FNO    BIT(5)           // frame number overflow
#define OHCI_INT_RHSC   BIT(6)           // root hub status change
#define OHCI_INT_OC     BIT(30)          // ownership change
#define OHCI_INT_MIE    BIT(31)          // master interrupt enable


// Root Hub Partition

// HcRhDescriptorA Register
#define OHCI_RHA_NDP    0x000000FF       // number downstream ports (max. 15)
#define OHCI_RHA_PSM    BIT(8)           // PowerSwitchingMode
#define OHCI_RHA_NPS    BIT(9)           // NoPowerSwitching
#define OHCI_RHA_DT     BIT(10)          // DeviceType (always 0)
#define OHCI_RHA_OCPM   BIT(11)          // OverCurrentProtectionMode
#define OHCI_RHA_NOCP   BIT(12)          // NoOverCurrentProtection
#define OHCI_RHA_POTPGT 0xFF000000       // PowerOnToPowerGoodTime

// HcRhStatus Register
#define OHCI_RHS_LPS    BIT(0)           // LocalPowerStatus
#define OHCI_RHS_OCI    BIT(1)           // OverCurrentIndicator
#define OHCI_RHS_DRWE   BIT(15)          // DeviceRemoteWakeupEnable
#define OHCI_RHS_LPSC   BIT(16)          // LocalPowerStatusChange
#define OHCI_RHS_OCIC   BIT(17)          // OverCurrentIndicatorChange
#define OHCI_RHS_CRWE   BIT(31)          // ClearRemoteWakeupEnable

// HcRhPortStatus[1:NDP] Register
#define OHCI_PORT_CCS   BIT(0)           // CurrentConnectStatus (0 = no device connected, 1 = device connected)
#define OHCI_PORT_PES   BIT(1)           // PortEnableStatus (0 = port is disabled, 1 = port is enabled)
#define OHCI_PORT_PSS   BIT(2)           // PortSuspendStatus
#define OHCI_PORT_POCI  BIT(3)           // PortOverCurrentIndicator
#define OHCI_PORT_PRS   BIT(4)           // PortResetStatus
#define OHCI_PORT_PPS   BIT(8)           // PortPowerStatus
#define OHCI_PORT_LSDA  BIT(9)           // LowSpeedDeviceAttached (0 = full speed device, 1 = low speed device)
#define OHCI_PORT_CSC   BIT(16)          // ConnectStatusChange
#define OHCI_PORT_PESC  BIT(17)          // PortEnableStatusChange
#define OHCI_PORT_PSSC  BIT(18)          // PortSuspendStatusChange
#define OHCI_PORT_OCIC  BIT(19)          // PortOverCurrentIndicatorChange
#define OHCI_PORT_PRSC  BIT(20)          // PortResetStatusChange

#define OHCI_PORT_WTC   (OHCI_PORT_CSC | OHCI_PORT_PESC | OHCI_PORT_PSSC | OHCI_PORT_OCIC | OHCI_PORT_PRSC)

// USB - operational states of the HC
#define OHCI_USB_RESET       0
#define OHCI_USB_RESUME      BIT(6)
#define OHCI_USB_OPERATIONAL BIT(7)
#define OHCI_USB_SUSPEND    (BIT(6)|BIT(7))

// ED lists
#define ED_INTERRUPT_1ms     0
#define ED_INTERRUPT_2ms     1
#define ED_INTERRUPT_4ms     3
#define ED_INTERRUPT_8ms     7
#define ED_INTERRUPT_16ms   15
#define ED_INTERRUPT_32ms   31
#define ED_CONTROL          63
#define ED_BULK             64
#define ED_ISOCHRONOUS       0 // same as 1ms interrupt queue
#define NO_ED_LISTS         65
#define ED_EOF              0xFF




/*
There are two communication channels between the HC and the HC Driver.
The first channel uses a set of operational registers located on the HC. The HC is the target for all communication on this channel.
The operational registers contain control, status, and list pointer registers.
Within the operational register set is a pointer to a location in shared memory named the HC Communications Area (HCCA). ...
*/

typedef struct
{
             uint32_t HcRevision;                 // BCD version of HCI spec implemented by this HC
    volatile uint32_t HcControl;                  // operating modes for the HC
    volatile uint32_t HcCommandStatus;            // current status of the HC
    volatile uint32_t HcInterruptStatus;          // hardware interrupts
    volatile uint32_t HcInterruptEnable;          // enabled intterupts
    volatile uint32_t HcInterruptDisable;         // disabled intterupts
    volatile uint32_t HcHCCA;                     // physical address of the HC Communication Area
    volatile uint32_t HcPeriodCurrentED;          // physical address of the current Isochronous or Interrupt Endpoint Descriptor
    volatile uint32_t HcControlHeadED;            // physical address of the first Endpoint Descriptor of the Control list
    volatile uint32_t HcControlCurrentED;         // physical address of the current Endpoint Descriptor of the Control list
    volatile uint32_t HcBulkHeadED;               // physical address of the first Endpoint Descriptor of the Bulk list
    volatile uint32_t HcBulkCurrentED;            // physical address of the current endpoint of the Bulk list
    volatile uint32_t HcDoneHead;                 // physical address of the last completed Transfer Descriptor that was added to the Done queue
    volatile uint32_t HcFmInterval;               // 14-bit: bit time interval in a Frame, 15-bit: Full Speed maximum packet size
    volatile uint32_t HcFmRemaining;              // 14-bit down counter showing the bit time remaining in the current Frame
    volatile uint32_t HcFmNumber;                 // 16-bit counter
    volatile uint32_t HcPeriodicStart;            // 14-bit: earliest time HC should start processing the periodic list
    volatile uint32_t HcLSThreshold;              // 11-bit: HC determines whether to commit to the transfer of a maximum of 8-byte LS packet before EOF
    volatile uint32_t HcRhDescriptorA;            // characteristics of the Root Hub
    volatile uint32_t HcRhDescriptorB;            // characteristics of the Root Hub
    volatile uint32_t HcRhStatus;                 // lower word: Root Hub Status field, upper word: Root Hub Status Change field
    volatile uint32_t HcRhPortStatus[8];          // port events on a per-port basis, max. NDP = 15, we assume 8 (taken from VBox)
} __attribute__((packed)) ohci_OpRegs_t;

/*
... The HCCA is the second communication channel. The HC is the master for all communication on this channel.
The HCCA contains the head pointers to the interrupt Endpoint Descriptor lists, the head pointer to the done queue,
and status information associated with start-of-frame processing.
HCCA is a 256-byte structure of system memory used to send and receive control/status information to and from the HC.
This structure must be located on a 256-byte boundary. Phys. address of this structure has to be stored into the OpReg HcHCCA.
*/
typedef struct
{
    volatile uint32_t interrruptTable[32];        // pointers to interrupt EDs
    volatile uint16_t frameNumber;                // current frame number
    volatile uint16_t pad1;                       // when the HC updates frameNumber, it sets this word to 0
    volatile uint32_t doneHead;                   // holds at frame end the current value of HcDoneHead, and interrupt is sent
    uint8_t  reserved[116];
 } __attribute__((packed)) ohci_HCCA_t;


// OHCI device
typedef struct
{
    pciDev_t*      PCIdevice;            // PCI device
    uintptr_t      bar;                  // MMIO space (base address register)
    ohci_OpRegs_t* OpRegs;               // operational registers
    ohci_HCCA_t*   hcca;                 // HC Communications Area (virtual address)
    uint8_t        rootPorts;            // number of rootports
    size_t         memSize;              // memory size of IO space
    bool           enabledPorts;         // root ports enabled
    port_t         port[OHCIPORTMAX];    // root ports
    bool           run;                  // hc running (RS bit)
    uint8_t        num;                  // number of the OHCI
} ohci_t;


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize);
void ohci_initHC(ohci_t* o);
void ohci_resetHC(ohci_t* o);

#endif
