#ifndef USB_HC_H
#define USB_HC_H

#include "os.h"
#include "pci.h"

#define UHCI    0x00
#define OHCI    0x10
#define EHCI    0x20
#define XHCI    0x30
#define NO_HCI  0x80
#define ANY_HCI 0xFE

void install_USB_HostController(pciDev_t* PCIdev);

#endif
