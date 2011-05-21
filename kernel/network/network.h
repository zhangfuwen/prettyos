#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"


typedef struct network_adapter network_adapter_t;
typedef struct
{
    void (*install)(network_adapter_t*); // Device
    void (*interruptHandler)(registers_t*); // Device
    bool (*sendPacket)(network_adapter_t*, uint8_t*, size_t); // Device, buffer, length
} network_driver_t;

struct network_adapter
{
    pciDev_t*         PCIdev;
    network_driver_t* driver;
    void*             data; // Drivers internal data
    uint16_t          IO_base;
    void*             MMIO_base;
    uint8_t           MAC_address[6];
    uint8_t           IP_address[4];
};


bool network_installDevice(pciDev_t* device);
bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length);
void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length); // Called by driver


#endif
