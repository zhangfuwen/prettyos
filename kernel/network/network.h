#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"
#include "netprotocol/arp.h"

/*
#define IP_1   0
#define IP_2   0
#define IP_3   0
#define IP_4   0
*/

// own IP at start
#define IP_1 192
#define IP_2 168
#define IP_3  10
#define IP_4  97

// server IP
#define SIP_1 192
#define SIP_2 168
#define SIP_3  10
#define SIP_4   1

// requested IP
#define RIP_1 192
#define RIP_2 168
#define RIP_3  10
#define RIP_4  96

/*
//for qemu
#define IP_1   10
#define IP_2    0
#define IP_3    2
#define IP_4   15

#define SIP_1  10
#define SIP_2   0
#define SIP_3   2
#define SIP_4   2

// requested IP
#define RIP_1  10
#define RIP_2   0
#define RIP_3   2
#define RIP_4  16
*/

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
    arpTable_t        arpTable;
};

typedef struct
{
    network_adapter_t* adapter;
    void*              data;
    size_t             length;
} networkBuffer_t;

bool network_installDevice(pciDev_t* device);
bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length);
void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length); // Called by driver
void network_displayArpTables();
network_adapter_t* network_getAdapter(uint8_t IP[4]);

#endif
