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

void arp_addTableEntry(arpTable_t* table, uint8_t MAC[6], uint8_t IP[4], bool dynamic)
{
    arpTableEntry_t* entry = arp_findEntry(table, IP); // Check if there is already an entry with the same IP.
    if(entry == 0) // No entry found. Create new one.
    {
        entry = malloc(sizeof(arpTableEntry_t), 0, "arp entry");
        list_Append(table->table, entry);
    }
    memcpy(entry->IP,  IP,  4);
    memcpy(entry->MAC, MAC, 6);
    entry->dynamic = dynamic;
    entry->seconds = timer_getSeconds();
}

arpTableEntry_t* arp_findEntry(arpTable_t* table, uint8_t IP[4])
{
    arp_checkTable(table); // We check the table for obsolete entries.

    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        if(memcmp(entry->IP, IP, 4) == 0)
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

    printf("\nIP\t\t  MAC\t\t\tType\t  Time(sec)");
    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        size_t length = printf("\n%I\t", entry->IP);
        if(length < 10) printf("\t");
        printf("  %M\t%s\t  %u", entry->MAC, entry->dynamic?"dynamic":"static", entry->seconds);
    }
}

void arp_initTable(arpTable_t* table)
{
    table->table = list_Create();
    table->lastCheck = timer_getSeconds();

    // Create default entries
    // We use only the first 4 bytes of the array as IP, all 6 bytes are used as MAC
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    arp_addTableEntry(table, broadcast, broadcast, false);
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
            if (memcmp(packet->sourceIP, packet->destIP, 4) == 0) // IP requ. and searched is identical
            {
                printf("ARP Gratuitous Request\n");
            }
            else
            {
                printf("ARP Request\n");
            }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            printf("%M", packet->source_mac);
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            printf("%I", packet->sourceIP);
            textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
            printf("%M", packet->dest_mac);
            textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
            printf("%I", packet->destIP);

            // requested IP is our own IP?
            if (memcmp(packet->destIP, adapter->IP, 4) == 0)
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

                for (uint8_t i = 0; i < 4; i++)
                {
                    reply.destIP[i]   = packet->sourceIP[i];
                    reply.sourceIP[i] = adapter->IP[i];
                }

                EthernetSend(adapter, (void*)&reply, sizeof(arpPacket_t), packet->source_mac, 0x0806);
            }
            break;

        case 2: // ARP-Response
            printf("ARP Response\n");

            textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
            printf("%M", packet->source_mac);
            textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
            printf("%I", packet->sourceIP);
            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            printf("%M", packet->dest_mac);
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            printf("%I", packet->destIP);
            break;
        } // switch
        arp_addTableEntry(&adapter->arpTable, packet->source_mac, packet->sourceIP, true); // ARP table entry
        scheduler_unblockEvent(BL_NETPACKET, (void*)BL_NET_ARP);
    } // if
    else
    {
        printf("No Ethernet and IPv4 - Unknown packet sent to ARP\n");
    }
}

bool arp_sendRequest(network_adapter_t* adapter, uint8_t searchedIP[4])
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

    for (uint8_t i = 0; i < 4; i++)
    {
        request.destIP[i]   = searchedIP[4];
        request.sourceIP[i] = adapter->IP[i];
    }

    uint8_t destMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    return EthernetSend(adapter, (void*)&request, sizeof(arpPacket_t), destMAC, 0x0806);
}

bool arp_waitForReply(struct network_adapter* adapter, uint8_t searchedIP[4])
{
    bool b = scheduler_blockCurrentTask(BL_NETPACKET, (void*)BL_NET_ARP, 1000);
    while(b && arp_findEntry(&adapter->arpTable, searchedIP) == 0)
        b = scheduler_blockCurrentTask(BL_NETPACKET, (void*)BL_NET_ARP, 1000);

    return(b);
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
