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
        if(entry->IP[0] == IP[0] && entry->IP[1] == IP[1] && entry->IP[2] == IP[2] && entry->IP[3] == IP[3])
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

    printf("\nIP\t\t\tMAC\t\t\t\tType\tTime(sec)");
    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        if(entry->IP[0]==255 && entry->IP[1]==255 && entry->IP[2]==255 && entry->IP[3]==255)
        {
            printf("\n%u.%u.%u.%u \t%y-%y-%y-%y-%y-%y\t\t%s\t%u",
            entry->IP[0], entry->IP[1], entry->IP[2], entry->IP[3],
            entry->MAC[0], entry->MAC[1], entry->MAC[2], entry->MAC[3], entry->MAC[4], entry->MAC[5],
            entry->dynamic?"dynamic":"static", entry->seconds);
        }
        else
        {
            printf("\n%u.%u.%u.%u \t\t%y-%y-%y-%y-%y-%y\t\t%s\t%u",
            entry->IP[0], entry->IP[1], entry->IP[2], entry->IP[3],
            entry->MAC[0], entry->MAC[1], entry->MAC[2], entry->MAC[3], entry->MAC[4], entry->MAC[5],
            entry->dynamic?"dynamic":"static", entry->seconds);
        }

    }
}

void arp_initTable(arpTable_t* table)
{
    table->table = list_Create();
    table->lastCheck = timer_getSeconds();

    // Create default entries
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
            if ((packet->sourceIP[0] == packet->destIP[0]) &&
                (packet->sourceIP[1] == packet->destIP[1]) &&
                (packet->sourceIP[2] == packet->destIP[2]) &&
                (packet->sourceIP[3] == packet->destIP[3])) // IP requ. and searched is identical
            {
                printf("Operation: Gratuitous Request\n");
            }
            else
            {
                printf("Operation: Request\n");
            }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->source_mac[i]); }
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->sourceIP[i]); if (i<3) printf("."); }
            textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->dest_mac[i]);   }
            textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->destIP[i]);   if (i<3) printf("."); }

            // requested IP is our own IP?
            if (packet->destIP[0] == adapter->IP_address[0] && packet->destIP[1] == adapter->IP_address[1] &&
                packet->destIP[2] == adapter->IP_address[2] && packet->destIP[3] == adapter->IP_address[3])
            {
                printf("\n Tx prepared:");
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
                    reply.source_mac[i] = adapter->MAC_address[i];
                }

                for (uint8_t i = 0; i < 4; i++)
                {
                    reply.destIP[i]   = packet->sourceIP[i];
                    reply.sourceIP[i] = adapter->IP_address[i];
                }

                EthernetSend(adapter, (void*)&reply, sizeof(arpPacket_t), packet->source_mac, 0x0806);
            }
            break;

        case 2: // ARP-Reply
            printf("Operation: Response\n");

            textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->source_mac[i]); }
            textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->sourceIP[i]); if (i<3) printf("."); }
            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->dest_mac[i]);   }
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->destIP[i]);   if (i<3) printf("."); }
            break;
        } // switch
        arp_addTableEntry(&adapter->arpTable, packet->source_mac, packet->sourceIP, true); // ARP table entry
    } // if
    else
    {
        printf("No Ethernet and IPv4 - Unknown packet sent to ARP\n");
    }
}

void arp_sendGratitiousRequest(network_adapter_t* adapter)
{
    printf("\n Tx prepared:");
    arpPacket_t gratRequest;

    gratRequest.hardware_addresstype[0] = 0;    // Ethernet
    gratRequest.hardware_addresstype[1] = 1;
    gratRequest.protocol_addresstype[0] = 0x08; // IP
    gratRequest.protocol_addresstype[1] = 0x00;

    gratRequest.operation[0] = 0;
    gratRequest.operation[1] = 1; // Request

    gratRequest.hardware_addresssize = 6;
    gratRequest.protocol_addresssize = 4;

    for (uint8_t i = 0; i < 6; i++)
    {
        gratRequest.dest_mac[i]   = 0xFF; // Broadcast
        gratRequest.source_mac[i] = adapter->MAC_address[i];
    }

    for (uint8_t i = 0; i < 4; i++)
    {
        gratRequest.destIP[i]   = adapter->IP_address[i];
        gratRequest.sourceIP[i] = adapter->IP_address[i];
    }

    uint8_t destMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    EthernetSend(adapter, (void*)&gratRequest, sizeof(arpPacket_t), destMAC, 0x0806);
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
