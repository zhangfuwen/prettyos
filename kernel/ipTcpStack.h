#ifndef IPTCPSTACK_H
#define IPTCPSTACK_H

#include "os.h"

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
    uint8_t source_ip[4];
    uint8_t dest_mac[6];
    uint8_t dest_ip[4];
} __attribute__((packed)) arp_t;

typedef struct arpPacket
{
    ethernet_t eth;
    arp_t      arp;
} __attribute__((packed)) arpPacket_t;

typedef struct ip
{
    uint8_t version           :1;
    uint8_t ipHeaderLength    :4;
    uint8_t typeOfService;
    uint8_t length[2];
    uint8_t identification[2];
    uint8_t flags             :3;
    uint8_t fragmentoffset0   :5;
    uint8_t fragmentoffset1;
    uint8_t ttl;
    uint8_t protocol;
    uint8_t checksum[2];
    uint8_t source_ip[4];
    uint8_t dest_ip[4];
    uint8_t options[4];
    uint8_t padding; 
} __attribute__((packed)) ip_t;

void ipTcpStack_recv(void* data, uint32_t length);
bool ipTcpStack_send(void* data, uint32_t length);

#endif
