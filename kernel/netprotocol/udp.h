#ifndef UDP_H
#define UDP_H

#include "network/network.h"

#define MAX_UDP_PORTS 65536


typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udpPacket_t;


void UDPConnect();
void UDPBind();
void UDPSend(struct network_adapter* adapter, void* data, uint32_t length);
void UDPRecv(udpPacket_t* packet);
void UDPDebug(udpPacket_t* upd);


#endif
