#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"
#include "netprotocol/arp.h"
#include "netprotocol/dhcp.h"
#include "netutils.h"

#define QEMU_HACK

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

// gateway IP for routing to the internet
#define GW_IP_1  10
#define GW_IP_2   0
#define GW_IP_3   2
#define GW_IP_4   2

// HACK for qemu, MAC of qemu gateway (for TCP experiments)
#define GW_MAC_1 0x52
#define GW_MAC_2 0x55
#define GW_MAC_3 0x0A
#define GW_MAC_4 0x00
#define GW_MAC_5 0x02
#define GW_MAC_6 0x02

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
    IP_t              IP;
    arpTable_t        arpTable;
    DHCP_state        DHCP_State;
    IP_t              Gateway_IP;
    IP_t              Subnet;
};

typedef struct 
{
    IP_t IP;
    uint8_t MAC[6];
} Packet_t;

bool network_installDevice(pciDev_t* device);
bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length);
void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length); // Called by driver
void network_displayArpTables();
network_adapter_t* network_getAdapter(IP_t IP);
network_adapter_t* network_getFirstAdapter();


#endif
