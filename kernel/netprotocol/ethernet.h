#ifndef IPTCPSTACK_H
#define IPTCPSTACK_H

#include "os.h"

typedef struct ethernet
{
    uint8_t recv_mac[6];
    uint8_t send_mac[6];
    uint8_t type_len[2]; // Type(Ethernet 2) or Length(Ethernet 1)
} __attribute__((packed)) ethernet_t;

void EthernetRecv(void* data, uint32_t length);
bool EthernetSend(void* data, uint32_t length);

#endif
