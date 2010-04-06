#ifndef IPTCPSTACK_H
#define IPTCPSTACK_H

#include "os.h"

struct ethernet
{
    uint8_t recv_mac[6];
    uint8_t send_mac[6];
    uint8_t type_len[2]; // Type(Ethernet 2) or Length(Ethernet 1)
} __attribute__((packed));

struct arp
{
    uint8_t hardware_addresstype[2];
    uint8_t protocol_addresstype[2];
    uint8_t hardware_addresssize;
    uint8_t protocol_addresssize;
    uint8_t operation[2];
    uint8_t source_mac[6];
    uint8_t source_ip[4];
    uint8_t dest_map[6];
    uint8_t dest_ip[4];
} __attribute__((packed));

struct ip
{
    uint8_t version_ihl;
    uint8_t tos;
    uint8_t lenght[2];

    //
    uint8_t identification[2];

    uint8_t flags_fragmentoffset0;
    uint8_t _fragmentoffset1;

    //
    uint8_t ttl;
    uint8_t protocol;
    uint8_t checksum0[2];

    //
    uint8_t src[4];

    //
    uint8_t dst[4];

    //
    uint8_t optn[4];
    uint8_t pad; // padding

} __attribute__((packed));

void ipTcpStack_recv(void* Data, uint32_t Length);

#endif

