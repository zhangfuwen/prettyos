#ifndef NETBIOS_H
#define NETBIOS_H

#include "network/network.h"

typedef struct
{
    uint8_t   messageType;
    uint8_t   flags;
    uint16_t  ID;
    IP_t      sourceIP;
    uint16_t  sourcePort;
    uint16_t  Length;
    uint16_t  packetOffset;
} __attribute__((packed)) NetBIOSDatagramHeader_t;


void NetBIOS_Datagramm_Receive(network_adapter_t* adapter, NetBIOSDatagramHeader_t* NetBIOSdgm);


#endif
