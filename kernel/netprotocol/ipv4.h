#ifndef IPV4_H
#define IPV4_H

#include "network/network.h"


typedef struct
{
    uint8_t  ipHeaderLength   : 4;
    uint8_t  version          : 4;
    uint8_t  typeOfService;
    uint16_t length;
    uint16_t identification;
    uint16_t fragmentation;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  sourceIP[4];
    uint8_t  destIP[4];
} __attribute__((packed)) ipv4Packet_t;


void ipv4_received(network_adapter_t* adapter, ipv4Packet_t* packet, uint32_t length);
void ipv4_send(network_adapter_t* adapter, void* data, uint32_t length, uint8_t IP[4],int protocol);


#endif
