#ifndef ARP_H
#define ARP_H

#include "list.h"
#include "network/netutils.h"

#define ARP_TABLE_TIME_TO_CHECK   2    // time in minutes
#define ARP_TABLE_TIME_TO_DELETE 10    // time in minutes


typedef struct
{
    uint8_t hardware_addresstype[2];
    uint8_t protocol_addresstype[2];
    uint8_t hardware_addresssize;
    uint8_t protocol_addresssize;
    uint8_t operation[2];
    uint8_t source_mac[6];
    IP_t    sourceIP;
    uint8_t dest_mac[6];
    IP_t    destIP;
} __attribute__((packed)) arpPacket_t;

typedef struct
{
    list_t*  table;
    uint32_t lastCheck;
} arpTable_t;

typedef struct
{
    uint8_t  MAC[6];
    IP_t     IP;
    uint32_t seconds;
    bool     dynamic;
} arpTableEntry_t;


struct network_adapter;

void arp_initTable(arpTable_t* cache);
void arp_deleteTable(arpTable_t* cache);
void arp_addTableEntry(arpTable_t* cache, uint8_t MAC[6], IP_t IP, bool dynamic);
void arp_deleteTableEntry(arpTable_t* cache, arpTableEntry_t* entry);
arpTableEntry_t* arp_findEntry(arpTable_t* cache, IP_t IP);
void arp_showTable(arpTable_t* cache);
void arp_received(struct network_adapter* adapter, arpPacket_t* packet);
bool arp_sendRequest(struct network_adapter* adapter, IP_t searchedIP); // Pass adapter->IP to it, to issue a gratuitous request
bool arp_waitForReply(struct network_adapter* adapter, IP_t searchedIP);


#endif
