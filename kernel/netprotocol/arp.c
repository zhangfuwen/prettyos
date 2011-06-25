/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "arp.h"
#include "ethernet.h"
#include "video/console.h"
#include "timer.h"
#include "util.h"
#include "kheap.h"
#include "scheduler.h"
#include "network/netutils.h"


void arp_deleteTableEntry(arpTable_t* table, arpTableEntry_t* entry)
{
    list_Delete(table->table, entry);
}

static void arp_checkTable(arpTable_t* table)
{
    if (timer_getSeconds() > (table->lastCheck + ARP_TABLE_TIME_TO_CHECK * 60 )) // Check only every ... minutes
    {
        table->lastCheck = timer_getSeconds();
        for(element_t* e = table->table->head; e != 0; e = e->next)
        {
            arpTableEntry_t* entry = e->data;
            if(entry->dynamic &&                                                    // Only dynamic entries should be killed.
               timer_getSeconds() > entry->seconds + ARP_TABLE_TIME_TO_DELETE * 60) // Entry is older than ... minutes -> Obsolete entry. Delete it.
            {
                arp_deleteTableEntry(table, entry);
            }
        }
    }
}

void arp_addTableEntry(arpTable_t* table, uint8_t MAC[6], IP_t IP, bool dynamic)
{
    arpTableEntry_t* entry = arp_findEntry(table, IP); // Check if there is already an entry with the same IP.
    if(entry == 0) // No entry found. Create new one.
    {
        entry = malloc(sizeof(arpTableEntry_t), 0, "arp entry");
        list_Append(table->table, entry);
    }
    entry->IP.iIP = IP.iIP;
    memcpy(entry->MAC, MAC, 6);
    entry->dynamic = dynamic;
    entry->seconds = timer_getSeconds();
}

arpTableEntry_t* arp_findEntry(arpTable_t* table, IP_t IP)
{
    arp_checkTable(table); // We check the table for obsolete entries.

    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        if(entry->IP.iIP == IP.iIP)
        {
            entry->seconds = timer_getSeconds(); // Update time stamp.
            return(entry);
        }
    }
    return(0);
}

void arp_showTable(arpTable_t* table)
{
    arp_checkTable(table); // We check the table for obsolete entries.

    textColor(TABLE_HEADING);
    printf("\nIP\t\t  MAC\t\t\tType\t  Time(sec)");
    printf("\n--------------------------------------------------------------------------------");
    textColor(TEXT);
    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        size_t length = printf("%I\t", entry->IP);
        if(length < 9) printf("\t");
        printf("  %M\t%s\t  %u\n", entry->MAC, entry->dynamic?"dynamic":"static", entry->seconds);
    }
    textColor(TABLE_HEADING);
    printf("--------------------------------------------------------------------------------");
}

void arp_initTable(arpTable_t* table)
{
    table->table = list_Create();
    table->lastCheck = timer_getSeconds();

    // Create default entries
    // We use only the first 4 bytes of the array as IP, all 6 bytes are used as MAC
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    IP_t broadcastIP = {.iIP = 0xFFFFFFFF};
    arp_addTableEntry(table, broadcast, broadcastIP, false);
}

void arp_deleteTable(arpTable_t* table)
{
    list_DeleteAll(table->table);
}

void arp_received(network_adapter_t* adapter, arpPacket_t* packet)
{
    // 1 = Ethernet, 0x0800 = IPv4
    if ((((packet->hardware_addresstype[0] << 8) | packet->hardware_addresstype[1]) ==      1) &&
        (((packet->protocol_addresstype[0] << 8) | packet->protocol_addresstype[1]) == 0x0800) &&
        (packet->hardware_addresssize == 6) &&
        (packet->protocol_addresssize == 4))
    {
        // extract the operation
        switch ((packet->operation[0] << 8) | packet->operation[1])
        {
        case 1: // ARP-Request
            textColor(HEADLINE);
            if (packet->sourceIP.iIP == packet->destIP.iIP) // IP requ. and searched is identical
            {
                printf("\nARP Gratuitous Request:");
            }
            else
            {
                printf("\nARP Request:");
            }
            textColor(LIGHT_GRAY); printf("\nMAC Requesting: "); textColor(IMPORTANT); printf("%M", packet->source_mac);
            textColor(LIGHT_GRAY); printf("  IP Requesting: ");  textColor(IMPORTANT); printf("%I", packet->sourceIP);
            textColor(LIGHT_GRAY); printf("\nMAC Searched:   "); textColor(IMPORTANT); printf("%M", packet->dest_mac);
            textColor(LIGHT_GRAY); printf("  IP Searched:   ");  textColor(IMPORTANT); printf("%I", packet->destIP);

            // requested IP is our own IP?
            if (packet->destIP.iIP == adapter->IP.iIP)
            {
                arpPacket_t reply;

                for (uint8_t i = 0; i < 2; i++)
                {
                    reply.hardware_addresstype[i] = packet->hardware_addresstype[i];
                    reply.protocol_addresstype[i] = packet->protocol_addresstype[i];
                }
                reply.operation[0] = 0;
                reply.operation[1] = 2; // reply

                reply.hardware_addresssize = packet->hardware_addresssize;
                reply.protocol_addresssize = packet->protocol_addresssize;

                for (uint8_t i = 0; i < 6; i++)
                {
                    reply.dest_mac[i]   = packet->source_mac[i];
                    reply.source_mac[i] = adapter->MAC[i];
                }

                reply.destIP.iIP   = packet->sourceIP.iIP;
                reply.sourceIP.iIP = adapter->IP.iIP;

                EthernetSend(adapter, (void*)&reply, sizeof(arpPacket_t), packet->source_mac, 0x0806);
            }
            break;

        case 2: // ARP-Response
            textColor(HEADLINE);
            printf("\nARP Response\n");

            textColor(LIGHT_GRAY); printf("\nMAC Replying:   "); textColor(IMPORTANT); printf("%M", packet->source_mac);
            textColor(LIGHT_GRAY); printf("  IP Replying:   ");  textColor(IMPORTANT); printf("%I", packet->sourceIP);
            textColor(LIGHT_GRAY); printf("\nMAC Requesting: "); textColor(IMPORTANT); printf("%M", packet->dest_mac);
            textColor(LIGHT_GRAY); printf("  IP Requesting: ");  textColor(IMPORTANT); printf("%I", packet->destIP);
            break;
        } // switch
        arp_addTableEntry(&adapter->arpTable, packet->source_mac, packet->sourceIP, true); // ARP table entry
        scheduler_unblockEvent(BL_NETPACKET, (void*)BL_NET_ARP);
    } // if
    else
    {
        textColor(ERROR);
        printf("\nNo Ethernet and IPv4 - Unknown packet sent to ARP.");
    }
}

bool arp_sendRequest(network_adapter_t* adapter, IP_t searchedIP)
{
    arpPacket_t request;

    request.hardware_addresstype[0] = 0;    // Ethernet
    request.hardware_addresstype[1] = 1;
    request.protocol_addresstype[0] = 0x08; // IP
    request.protocol_addresstype[1] = 0x00;

    request.operation[0] = 0;
    request.operation[1] = 1; // Request

    request.hardware_addresssize = 6;
    request.protocol_addresssize = 4;

    for (uint8_t i = 0; i < 6; i++)
    {
        request.dest_mac[i]   = 0x00;
        request.source_mac[i] = adapter->MAC[i];
    }

    request.destIP.iIP   = searchedIP.iIP;
    request.sourceIP.iIP = adapter->IP.iIP;

    textColor(HEADLINE);
    printf("\nARP Request sent:");
    textColor(LIGHT_GRAY); printf(" IP Searched: "); textColor(IMPORTANT);  printf("%I", searchedIP);

    uint8_t destMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return EthernetSend(adapter, (void*)&request, sizeof(arpPacket_t), destMAC, 0x0806);
}

bool arp_waitForReply(struct network_adapter* adapter, IP_t searchedIP)
{
    uint32_t timewait = 2000; // ms
    while  (arp_findEntry(&adapter->arpTable, searchedIP) == 0 && scheduler_blockCurrentTask(BL_NETPACKET, (void*)BL_NET_ARP, timewait));
    return (arp_findEntry(&adapter->arpTable, searchedIP) != 0);
}

/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
