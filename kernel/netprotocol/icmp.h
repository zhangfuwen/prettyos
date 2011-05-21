#ifndef ICMP_H
#define ICMP_H

// http://tools.ietf.org/html/rfc1071 <--- internet checksum
// http://de.wikipedia.org/wiki/Internet_Control_Message_Protocol <--- icmp Protocol

#include "types.h"
#include "ethernet.h"
#include "ipv4.h"

#define ICMP_ECHO_REPLY                    0        // Echo Reply
#define ICMP_RESERVED_1                    1        //  1-2 = Reserved
#define ICMP_RESERVED_2                    2        //  1-2 = Reserved
#define ICMP_DESTINATION_UNREACHABLE       3        // Destination Unreachable
#define ICMP_SOURCE_QUENCH                 4        // Source Quench
#define ICMP_REDIRECT                      5        // Redirect
#define ICMP_ECHO_REQUEST                  8        // Echo Request
#define ICMP_ROUTER_ADVERTISEMENT          9        // Router Advertisement
#define ICMP_ROUTER_SOLICITATION           10       // Router Solicitation
#define ICMP_TIME_EXEEDED                  11       // Time Exceeded
#define ICMP_PARAMETER_PROBLEM             12       // Parameter Problem
#define ICMP_TIMESTAMP                     13       // Timestamp
#define ICMP_TIMESTAMP_REPLY               14       // Timestamp Reply
#define ICMP_INFORMATION_REQUEST           15       // Information Request
#define ICMP_INFORMATION_REPLEY            16       // Information Reply
#define ICMP_ADDRESS_MASK_REQUEST          17       // Address Mask Request
#define ICMP_ADDRESS_MASK_REPLY            18       // Address Mask Reply
#define ICMP_RESERVED_19                   19       // Reserved (for Security)
#define ICMP_RESERVED_20_29                20       // Reserved (for Robustness Experiment)
#define ICMP_TRACEROUTE                    30       // Traceroute
#define ICMP_DATAGRAM_CONVERSION_ERROR     31       // Datagram Conversion Error
#define ICMP_MOBILE_HOST_REDIRECT          32       // Mobile Host Redirect
#define ICMP_WHERE_ARE_YOU                 33       // Ursprünglich IPv6 Where-Are-You (ersetzt durch ICMPv6)
#define ICMP_I_AM_HERE                     34       // Ursprünglich IPv6 I-Am-Here (ersetzt durch ICMPv6)
#define ICMP_MOBILE_REGISTRATION_REQUEST   35       // Mobile Registration Request
#define ICMP_MOBILE_REGISTRATION_REPLY     36       // Mobile Registration Reply
#define ICMP_DOMAIN_NAME_REQUEST           37       // Domain Name Request
#define ICMP_DOMAIN_NAME_REPLY             38       // Domain Name Reply
#define ICMP_SKIP                          39       // SKIP
#define ICMP_PHOTURIS                      40       // Photuris
// #define ICMP_SEAMOBY                    41       // ICMP messages utilized by experimental mobility protocols such as Seamoby
// #define ICMP_RESERVED_42_255            42–255   // Reserved


typedef struct icmpheader
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seqnumber;
} __attribute__((packed)) icmpheader_t;

typedef struct icmpPacket // eth:ip:icmp:data
{
    ethernet_t   eth;
    ip_t         ip;
    icmpheader_t icmp;
} __attribute__((packed)) icmppacket_t;


int internetChecksum(void *addr, size_t count);
void ICMPAnswerPing(network_adapter_t* adapter, void* data, uint32_t length);
void icmpDebug(void* data, uint32_t length);


#endif
