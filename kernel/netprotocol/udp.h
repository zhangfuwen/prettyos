#ifndef UDP_H
#define UDP_H

#include "network/network.h"


typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udpPacket_t;


void udp_send(network_adapter_t* adapter, void* data, uint32_t length, uint16_t srcPort, IP_t srcIP, uint16_t destPort, IP_t destIP);
void udp_receive(network_adapter_t* adapter, udpPacket_t* packet, uint32_t length);


#endif
