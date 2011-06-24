#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"
#include "netprotocol/arp.h"
#include "netprotocol/dhcp.h"

#define QEMU_HACK

// own IP at start
#define IP_1  192
#define IP_2  168
#define IP_3    44
#define IP_4   97

// requested IP
#define RIP_1 192
#define RIP_2 168
#define RIP_3   44
#define RIP_4  97

// gateway IP for routing to the internet
#define GW_IP_1 192
#define GW_IP_2 168
#define GW_IP_3   44
#define GW_IP_4   29

// HACK for qemu, MAC of the external gateway (for TCP experiments)
#define GW_MAC_1 0x00
#define GW_MAC_2 0x40
#define GW_MAC_3 0x63
#define GW_MAC_4 0xE7
#define GW_MAC_5 0xDA
#define GW_MAC_6 0xDE

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
network_adapter_t* network_getFirstAdapter();


#endif
