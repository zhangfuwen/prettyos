#ifndef ARP_H
#define ARP_H

#include "os.h"
#include "list.h"

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
    uint8_t sourceIP[4];
    uint8_t dest_mac[6];
    uint8_t destIP[4];
} __attribute__((packed)) arpPacket_t;


typedef struct
{
    listHead_t* table;
    uint32_t    lastCheck;
} arpTable_t;

typedef struct
{
    uint8_t  MAC[6];
    uint8_t  IP[4];
    uint32_t seconds;
    bool     dynamic;
} arpTableEntry_t;


struct network_adapter;

void arp_initTable(arpTable_t* table);
void arp_deleteTable(arpTable_t* table);
void arp_addTableEntry(arpTable_t* table, uint8_t MAC[6], uint8_t IP[4], bool dynamic);
void arp_deleteTableEntry(arpTable_t* table, arpTableEntry_t* entry);
arpTableEntry_t* arp_findEntry(arpTable_t* table, uint8_t IP[4]);
void arp_showTable(arpTable_t* table);

void arp_received(struct network_adapter* adapter, arpPacket_t* packet);
void arp_sendGratitiousRequest(struct network_adapter* adapter);


#endif
