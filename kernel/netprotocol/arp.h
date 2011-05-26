#ifndef ARP_H
#define ARP_H

#include "os.h"
#include "ethernet.h"


typedef struct
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

typedef struct
{
    ethernet_t eth;
    arp_t      arp;
} __attribute__((packed)) arpPacket_t;


void arp_received(network_adapter_t* adapter, arpPacket_t* packet);


#endif
