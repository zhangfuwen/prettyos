/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "arp.h"
#include "network/network.h"
#include "video/console.h"
#include "timer.h"
#include "util.h"
#include "kheap.h"


static void arp_deleteTableEntry(arpTable_t* table, arpTableEntry_t* entry)
{
    list_Delete(table->table, entry);
}

static void arp_checkTable(arpTable_t* table)
{
    if(table->lastCheck + 2*60 > timer_getSeconds()) // Check only every 2 minutes
    {
        table->lastCheck = timer_getSeconds();
        for(element_t* e = table->table->head; e != 0; e = e->next)
        {
            if(((arpTableEntry_t*)e->data)->dynamic && ((arpTableEntry_t*)e->data)->seconds + 5*60 > timer_getSeconds()) // Entry is older than 5 minutes -> obsolete entry. Delete it.
                arp_deleteTableEntry(table, (arpTableEntry_t*)e->data);
        }
    }
}

void arp_addTableEntry(arpTable_t* table, uint8_t MAC[6], uint8_t IP[4], bool dynamic)
{
    arp_checkTable(table); // We check the table for obsolete entries.

    arpTableEntry_t* entry = arp_findEntry(table, IP); // Check if there is already an entry with the same IP.
    if(entry == 0) // No entry found. Create new one.
    {
        entry = malloc(sizeof(arpTableEntry_t), 0, "arp entry");
        list_Append(table->table, entry);
    }

    memcpy(entry->IP, IP, 4);
    memcpy(entry->MAC, MAC, 4);
    entry->dynamic = dynamic;
    entry->seconds = timer_getSeconds();

}

arpTableEntry_t* arp_findEntry(arpTable_t* table, uint8_t IP[4])
{
    arp_checkTable(table); // We check the table for obsolete entries.

    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        if(strncmp((char*)((arpTableEntry_t*)e->data)->IP, (char*)IP, 4) == 0)
            return(e->data);
    }
    return(0);
}

void arp_showTable(arpTable_t* table)
{
    printf("\nIP\t\t\tMAC\t\t\tType");
    for(element_t* e = table->table->head; e != 0; e = e->next)
    {
        arpTableEntry_t* entry = e->data;
        printf("\n%u.%u.%u.%u\t\tMAC: %y-%y-%y-%y-%y-%y\t\t%s",
            entry->IP[0], entry->IP[1], entry->IP[2], entry->IP[3], 
            entry->MAC[0], entry->MAC[1], entry->MAC[2], entry->MAC[3], entry->MAC[4], entry->MAC[5], 
            entry->dynamic?"dynamic":"static");
    }
}

void arp_initTable(arpTable_t* table)
{
    table->table = list_Create();
    table->lastCheck = timer_getSeconds();
}

void arp_deleteTable(arpTable_t* table)
{
    list_DeleteAll(table->table);
}


void arp_received(network_adapter_t* adapter, arpPacket_t* packet)
{
    if ((((packet->arp.hardware_addresstype[0] << 8) | packet->arp.hardware_addresstype[1]) ==    1) &&
        (((packet->arp.protocol_addresstype[0] << 8) | packet->arp.protocol_addresstype[1]) == 2048) &&
        (packet->arp.hardware_addresssize == 6) &&
        (packet->arp.protocol_addresssize == 4))
    {
        // extract the operation
        switch ((packet->arp.operation[0] << 8) | packet->arp.operation[1])
        {
        case 1: // ARP-Request
            if ((packet->arp.sourceIP[0] == packet->arp.destIP[0]) &&
                (packet->arp.sourceIP[1] == packet->arp.destIP[1]) &&
                (packet->arp.sourceIP[2] == packet->arp.destIP[2]) &&
                (packet->arp.sourceIP[3] == packet->arp.destIP[3])) // IP requ. and searched is identical
            {
                printf("Operation: Gratuitous Request\n");
            }
            else
            {
                printf("Operation: Request\n");
            }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.source_mac[i]); }
            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.sourceIP[i]); if (i<3) printf("."); }
            textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.dest_mac[i]);   }
            textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.destIP[i]);   if (i<3) printf("."); }

            // requested IP is our own IP?
            if (packet->arp.destIP[0] == adapter->IP_address[0] && packet->arp.destIP[1] == adapter->IP_address[1] &&
                packet->arp.destIP[2] == adapter->IP_address[2] && packet->arp.destIP[3] == adapter->IP_address[3])
            {
                printf("\n Tx prepared:");
                arpPacket_t reply;
                for (uint8_t i = 0; i < 6; i++)
                {
                    reply.eth.recv_mac[i] = packet->arp.source_mac[i];
                    reply.eth.send_mac[i] = adapter->MAC_address[i];
                }
                reply.eth.type_len[0] = 0x08; reply.eth.type_len[1] = 0x06;

                for (uint8_t i = 0; i < 2; i++)
                {
                    reply.arp.hardware_addresstype[i] = packet->arp.hardware_addresstype[i];
                    reply.arp.protocol_addresstype[i] = packet->arp.protocol_addresstype[i];
                }
                reply.arp.operation[0] = 0;
                reply.arp.operation[1] = 2; // reply

                reply.arp.hardware_addresssize = packet->arp.hardware_addresssize;
                reply.arp.protocol_addresssize = packet->arp.protocol_addresssize;

                for (uint8_t i = 0; i < 6; i++)
                {
                    reply.arp.dest_mac[i]   = packet->arp.source_mac[i];
                    reply.arp.source_mac[i] = adapter->MAC_address[i];
                }

                for (uint8_t i = 0; i < 4; i++)
                {
                    reply.arp.destIP[i]   = packet->arp.sourceIP[i];
                    reply.arp.sourceIP[i] = adapter->IP_address[i];
                }

                EthernetSend(adapter, (void*)&reply, sizeof(arpPacket_t));
            }
            break;
        case 2: // ARP-Reply
            printf("Operation: Response\n");

            textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.source_mac[i]); }

            textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.sourceIP[i]); if (i<3) printf("."); }

            textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 6; i++) { printf("%y ", packet->arp.dest_mac[i]);   }

            textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
            for (uint8_t i = 0; i < 4; i++) { printf("%u", packet->arp.destIP[i]);   if (i<3) printf("."); }
            break;
        } // switch
    } // if
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
