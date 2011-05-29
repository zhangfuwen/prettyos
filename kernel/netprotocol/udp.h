#ifndef UDP_H
#define UDP_H

#include "ethernet.h"
#include "ipv4.h"

#define MAX_UDP_PORTS 65536


typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udpheader_t;

typedef struct
{
    ethernet_t eth;
    ip_t ip;
    udpheader_t udp;
} __attribute__((packed)) udppacket_t;


void UDPConnect();
void UDPBind();
void UDPSend(void* data, uint32_t length);
void UDPRecv(udppacket_t* packet);
void UDPDebug(udpheader_t* upd);


#endif
