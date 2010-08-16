#ifndef IPTCPSTACK_H
#define IPTCPSTACK_H

#include "os.h"

#define htons(v) ((((v) >> 8) & 0xFF) | (((v) & 0xFF) << 8))
#define htonl(v) ((((v) >> 24) & 0xFF) | (((v) >> 8) & 0xFF00) | (((v) & 0xFF00) << 8) | (((v) & 0xFF) << 24))
#define ntohs(v) htons(v)
#define ntohl(v) htonl(v)

typedef struct ethernet
{
    uint8_t recv_mac[6];
    uint8_t send_mac[6];
    uint8_t type_len[2]; // Type(Ethernet 2) or Length(Ethernet 1)
} __attribute__((packed)) ethernet_t;

void EthernetRecv(void* data, uint32_t length);
bool EthernetSend(void* data, uint32_t length);

#endif
