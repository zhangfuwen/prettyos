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

typedef struct arp
{
    uint8_t hardware_addresstype[2];
    uint8_t protocol_addresstype[2];
    uint8_t hardware_addresssize;
    uint8_t protocol_addresssize;
    uint8_t operation[2];
    uint8_t source_mac[6];
    uint8_t sourceIP[4];
    uint8_t dest_mac[6];
    uint8_t destIP[4];
} __attribute__((packed)) arp_t;

typedef struct arpPacket
{
    ethernet_t eth;
    arp_t      arp;
} __attribute__((packed)) arpPacket_t;

typedef struct ip
{    
    uint8_t  ipHeaderLength   :4;
    uint8_t  version          :4;
    uint8_t  typeOfService;
    uint16_t length;
    uint16_t identification;
    uint16_t fragmentation;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  sourceIP[4];
    uint8_t  destIP[4];
} __attribute__((packed)) ip_t;

void EthernetRecv(void* data, uint32_t length);
bool EthernetSend(void* data, uint32_t length);

#endif
