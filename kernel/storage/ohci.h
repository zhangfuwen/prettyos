#ifndef OHCI_H
#define OHCI_H

#include "os.h"
#include "pci.h"
#include "synchronisation.h"
#include "devicemanager.h"

#define OHCIMAX      8  // max number of OHCI devices
#define OHCIPORTMAX  4  // max number of OHCI device ports





// OHCI device
typedef struct
{
    pciDev_t*  PCIdevice;          // PCI device
    uint16_t   bar;                // start of I/O space (base address register
    //
    //
    uint8_t    rootPorts;          // number of rootports
    size_t     memSize;            // memory size of IO space
    bool       enabledPorts;       // root ports enabled
    port_t     port[OHCIPORTMAX];  // root ports
    bool       run;                // hc running (RS bit)
} ohci_t;


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize);
void startOHCI();
int32_t initOHCIHostController(ohci_t* o);
void ohci_resetHostController(ohci_t* o);
void ohci_handler(registers_t* r, pciDev_t* device);



#endif
