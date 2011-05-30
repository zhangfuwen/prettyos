#ifndef IPTCPSTACK_H
#define IPTCPSTACK_H

#include "os.h"


typedef struct
{
    uint8_t recv_mac[6];
    uint8_t send_mac[6];
    uint8_t type_len[2]; // Type(Ethernet 2) or Length(Ethernet 1)
} __attribute__((packed)) ethernet_t;


struct network_adapter;

void EthernetRecv(struct network_adapter* adapter, ethernet_t* eth, uint32_t length);
bool EthernetSend(struct network_adapter* adapter, void* data, uint32_t length, uint8_t MAC[6], uint16_t type);


#endif
