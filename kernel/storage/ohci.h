#ifndef OHCI_H
#define OHCI_H

#include "os.h"
#include "pci.h"
#include "devicemanager.h"

#define OHCIMAX      8  // max number of OHCI devices
#define OHCIPORTMAX  4  // max number of OHCI device ports

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


// OHCI device
typedef struct
{
    pciDev_t*  PCIdevice;         // PCI device
    uintptr_t  bar;               // MMIO space (base address register)
    
    ohci_OpRegs_t* ohci_pOpRegs;  // 
    
    uint8_t    rootPorts;         // number of rootports
    size_t     memSize;           // memory size of IO space
    bool       enabledPorts;      // root ports enabled
    port_t     port[OHCIPORTMAX]; // root ports
    bool       run;               // hc running (RS bit)
    uint8_t    num;               // Number of the OHCI
} ohci_t;


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize);
void ohci_pollDisk(void* dev);
void ohci_initHC(ohci_t* o);
void ohci_resetHC(ohci_t* o);


#endif
