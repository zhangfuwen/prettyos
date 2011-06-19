#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"
#include "netprotocol/arp.h"
#include "netprotocol/dhcp.h"

// own IP at start
#define IP_1  192
#define IP_2  168
#define IP_3    1
#define IP_4   97

// requested IP
#define RIP_1 192
#define RIP_2 168
#define RIP_3   1
#define RIP_4  97

/*
//for qemu

// own IP at start
#define IP_1   10
#define IP_2    0
#define IP_3    2
#define IP_4   15

// requested IP
#define RIP_1  10
#define RIP_2   0
#define RIP_3   2
#define RIP_4  15
*/

// router MAC for routing to the internet
#define MAC_1 0x00 
#define MAC_2 0x22
#define MAC_3 0xcf
#define MAC_4 0x36
#define MAC_5 0x9d
#define MAC_6 0x1c

typedef struct network_adapter network_adapter_t;

typedef struct
{
    void (*install)(network_adapter_t*); // Device
    void (*interruptHandler)(registers_t*); // Device
    bool (*sendPacket)(network_adapter_t*, uint8_t*, size_t); // Device, buffer, length
} network_driver_t;

typedef struct
{
    network_adapter_t* adapter;
    void*              data;
    size_t             length;
} networkBuffer_t;

struct network_adapter
{
    pciDev_t*         PCIdev;
    network_driver_t* driver;
    void*             data; // Drivers internal data
    uint16_t          IO_base;
    void*             MMIO_base;
    uint8_t           MAC[6];
    uint8_t           IP[4];
    arpTable_t        arpTable;
    DHCP_state        DHCP_State;
    uint8_t           Gateway_IP[4];
    uint8_t           Subnet[4]; 
};


bool network_installDevice(pciDev_t* device);
bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length);
void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length); // Called by driver
void network_displayArpTables();
network_adapter_t* network_getAdapter(uint8_t IP[4]);


#endif
