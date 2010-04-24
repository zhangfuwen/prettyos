#ifndef USB_HC_H
#define USB_HC_H

#include "os.h"
#include "ehci.h"
#include "pci.h"

void install_USB_HostController(uint32_t num);

#endif
