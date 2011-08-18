/*
 * DNS help functions
 * Written by: cooky451 - 8/15/2011
 * Last change: ehenkes - 8/16/2011
 * Todo: getHostByName() which returns all hosts for a name.
 * Comments: -
 */

#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "userlib.h"
#include "dns.h"
#include "dns_help.h"

IP_t getAddrByName(const char* name)
{
    char buf[512];
    uint16_t query_id = 1911; /// TODO
    size_t query_size;
    IP_t host, dns_server;
    host.iIP = dns_server.iIP = 0;

    query_size = dns_createSimpleQuery(buf,
        sizeof(buf), name, query_id);

    dns_getServer(&dns_server);
    event_enable(1);

    if (query_size && dns_server.iIP && udp_send(buf,
        query_size, dns_server, dns_port, dns_port))
    {
        udpReceivedEventHeader_t* udp_header;
        char* data;
        size_t len;
        EVENT_t ev = event_poll(buf, sizeof(buf), EVENT_NONE);
        for (; ev != EVENT_UDP_RECEIVED;
            ev = event_poll(buf, sizeof(buf), EVENT_NONE))
        {
            switch (ev)
            {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_KEY_DOWN:
                if (*(KEY_t*)buf == KEY_ESC)
                    return host;
                break;
                // gcc: event_not_handled
            case EVENT_INVALID_ARGUMENTS:
            case EVENT_OVERFLOW:
            case EVENT_KEY_UP:
            case EVENT_TEXT_ENTERED:
            case EVENT_TCP_CONNECTED:
            case EVENT_TCP_RECEIVED:
            case EVENT_TCP_CLOSED:
            case EVENT_UDP_RECEIVED:
            default:
                break;
            }
        }

        udp_header = (udpReceivedEventHeader_t*)buf;
        data = (char*)(udp_header + 1);
        len = udp_header->length;

        if (len)
        {
            dns_header header;
            dns_question question;
            dns_resource resource;
            const char* p = data;
            p = dns_parseHeader(&header, data, len);
            while (p && header.qdcount--)
            { // discard questions
                p = dns_parseQuestion(&question, data, len, p);
            }
            while (p && header.ancount--)
            { // look for ipv4 addr
                if ((p = dns_parseResource(&resource, data, len, p)))
                {
                    if (resource.type == dns_type_A && //ipv4
                        resource.dns_class == dns_class_IN && // internet
                        resource.rdlength >= 4) // need min 4 bytes
                    {
                        memcpy(&host.iIP, &resource.rdata, 4);
                        break;
                    }
                }
            }
        }
    }
    return host;
}

void printDNSHeader(const dns_header *header)
{
    printf("ID: %i\n""Flags: %u\n""Questions: %u\n"
        "Answers: %i\n""Authoritys: %u\n""Additionals: %u\n",
        header->id, header->flags, header->qdcount,
        header->ancount, header->arcount, header->nscount);
}

void printDNSQuestion(const dns_question *question)
{
    printf("Name: %s\n""Type: %u\n""Class: %u\n",
        question->qname, question->qtype, question->qclass);
}

void printDNSResource(const dns_resource *resource)
{
    printf("Name: %s\n""Type: %u\n""Class: %u\n"
        "TTL: %ul\n""RDLength: %u\n",
        resource->name, resource->type, resource->dns_class,
        resource->ttl, resource->rdlength);
}

void showDNSQuery(const char* name)
{
    char buf[512];
    uint16_t query_id = 1911; /// TODO
    size_t query_size;
    IP_t dns_server;

    printf("Begin DNS query for %s..\n", name);

    query_size = dns_createSimpleQuery(buf,
        sizeof(buf), name, query_id);

    dns_getServer(&dns_server);
    printf("DNS Server: %u.%u.%u.%u\n",
        dns_server.IP[0], dns_server.IP[1],
        dns_server.IP[2], dns_server.IP[3]);

    event_enable(1);

    if (query_size && dns_server.iIP && udp_send(buf,
        query_size, dns_server, dns_port, dns_port))
    {
        udpReceivedEventHeader_t* udp_header;
        char* data;
        size_t len;
        EVENT_t ev = event_poll(buf, sizeof(buf), EVENT_NONE);
        for (; ev != EVENT_UDP_RECEIVED;
            ev = event_poll(buf, sizeof(buf), EVENT_NONE))
        {
            switch (ev)
            {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_KEY_DOWN:
                if (*(KEY_t*)buf == KEY_ESC)
                    return;
                break;
                    // gcc: event_not_handled
            case EVENT_INVALID_ARGUMENTS:
            case EVENT_OVERFLOW:
            case EVENT_KEY_UP:
            case EVENT_TEXT_ENTERED:
            case EVENT_TCP_CONNECTED:
            case EVENT_TCP_RECEIVED:
            case EVENT_TCP_CLOSED:
            case EVENT_UDP_RECEIVED:
            default:
                break;
            }
        }

        udp_header = (udpReceivedEventHeader_t*)buf;
        data = (char*)(udp_header + 1);
        len = udp_header->length;

        if (len)
        {
            dns_header header;
            dns_question question;
            dns_resource resource;
            const char* p;
            if ((p = dns_parseHeader(&header, data, len)))
            {
                printf(" -- header -- \n");
                printDNSHeader(&header);
            }
            while (p && header.qdcount--)
            {
                if ((p = dns_parseQuestion(&question, data, len, p)))
                {
                    printf(" -- question -- \n");
                    printDNSQuestion(&question);
                    getchar();
                }
            }
            while (p && header.ancount--)
            {
                if ((p = dns_parseResource(&resource, data, len, p)))
                {
                    printf(" -- answer -- \n");
                    printDNSResource(&resource);
                    if (resource.type == dns_type_A &&
                        resource.dns_class == dns_class_IN &&
                        resource.rdlength >= 4)
                    {
                        printf("Found IP: %u.%u.%u.%u\n",
                            (unsigned char)resource.rdata[0],
                            (unsigned char)resource.rdata[1],
                            (unsigned char)resource.rdata[2],
                            (unsigned char)resource.rdata[3]);
                    }
                    else if (resource.type == dns_type_CNAME &&
                        resource.dns_class == dns_class_IN &&
                        resource.rdlength < 256)
                    {
                        char alias[256];
                        if (dns_parseName(alias, data, resource.rdlength,
                            resource.rdata))
                        {
                            printf("Found CNAME: %s\n", alias);
                        }
                    }
                    getchar();
                }
            }
        }
    }
    printf(" -- END -- \n");
    getchar();
}
