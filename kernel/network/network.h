#ifndef NETWORK_H
#define NETWORK_H

#include "pci.h"
#include "netprotocol/arp.h"
#include "netprotocol/dhcp.h"
#include "netutils.h"

/*
typical qemu.bat:
del ser1.txt
del ser2.txt
del ser3.txt
del ser4.txt
set QEMU_AUDIO_DRV=wav
qemu.exe  -boot a -fda FloppyImage.img -soundhw pcspk -net nic,model=rtl8139 -redir tcp:5023::23 
-redir tcp:8080::80 -localtime -net user -net dump,file=netdump.pcap -usb
-serial file:ser1.txt -serial file:ser2.txt -serial file:ser3.txt -serial file:ser4.txt 
*/

// own IP at start
#define IP_1    192
#define IP_2    168 
#define IP_3      1
#define IP_4     22 

// requested IP
#define RIP_1   192
#define RIP_2   168
#define RIP_3     1
#define RIP_4    22 

// gateway IP for routing to the internet
#define GW_IP_1   192
#define GW_IP_2   168
#define GW_IP_3     1
#define GW_IP_4     1

// DNS IP for resolving name to IP
#define DNS_IP_1     0
#define DNS_IP_2     0
#define DNS_IP_3     0
#define DNS_IP_4     0

typedef struct network_adapter network_adapter_t;

typedef struct
{
    void (*install)(network_adapter_t*); // Device
    void (*interruptHandler)(registers_t*, pciDev_t*); // Device
    bool (*sendPacket)(network_adapter_t*, uint8_t*, size_t); // Device, buffer, length
} network_driver_t;

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
    IP_t              dnsServer_IP;  
};

typedef struct
{
    IP_t IP;
    uint8_t  MAC[6];
} Packet_t;

bool network_installDevice(pciDev_t* device);
bool network_sendPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length);
void network_receivedPacket(network_adapter_t* adapter, uint8_t* buffer, size_t length); // Called by driver
void network_displayArpTables();
network_adapter_t* network_getAdapter(IP_t IP);
network_adapter_t* network_getFirstAdapter();
uint32_t getMyIP();
void dns_setServer(IP_t server);
void dns_getServer(IP_t* server);

#endif
