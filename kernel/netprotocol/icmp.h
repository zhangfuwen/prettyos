#ifndef ICMP_H
#define ICMP_H

// http://tools.ietf.org/html/rfc1071 <--- internet checksum 
// http://de.wikipedia.org/wiki/Internet_Control_Message_Protocol <--- icmp Protocol

#include "types.h"

#define ECHO_REPLY					0					// Echo Reply
#define RESERVED_1					1					//  1-2 = Reserved
#define RESERVED_2					2					//  1-2 = Reserved
#define DESTINATION_UNREACHABLE		3					// Destination Unreachable
#define SOURCE_QUENCH				4					// Source Quench
#define REDIRECT 					5					// Redirect
#define ECHO_REQUEST				8					// Echo Request
#define ROUTER_ADVERTISEMENT		9 					// Router Advertisement
#define ROUTER_SOLICITATION			10 					// Router Solicitation
#define TIME_EXEEDED				11					// Time Exceeded
#define PARAMETER_PROBLEM			12					// Parameter Problem
#define TIMESTAMP					13 					// Timestamp
#define TIMESTAMP_REPLY				14					// Timestamp Reply
#define INFORMATION_REQUEST			15					// Information Request
#define INFORMATION_REPLEY			16					// Information Reply
#define ADDRESS_MASK_REQUEST		17					// Address Mask Request
#define ADDRESS_MASK_REPLY			18					// Address Mask Reply
#define RESERVED_19					19					// Reserved (for Security)
#define RESERVED_20_29				20					// Reserved (for Robustness Experiment)
#define TRACEROUTE					30					// Traceroute
#define DATAGRAM_CONVERSION_ERROR	31					// Datagram Conversion Error
#define MOBILE_HOST_REDIRECT		32					// Mobile Host Redirect
#define WHERE_ARE_YOU				33					// Ursprünglich IPv6 Where-Are-You (ersetzt durch ICMPv6)
#define I_AM_HERE					34					// Ursprünglich IPv6 I-Am-Here (ersetzt durch ICMPv6)
#define MOBILE_REGISTRATION_REQUEST	35					// Mobile Registration Request
#define MOBILE_REGISTRATION_REPLY	36					// Mobile Registration Reply
#define DOMAIN_NAME_REQUEST			37					// Domain Name Request
#define DOMAIN_NAME_REPLY			38					// Domain Name Reply
#define SKIP						39					// SKIP
#define PHOTURIS					40					// Photuris
// #define SEAMOBY						41				// ICMP messages utilized by experimental mobility protocols such as Seamoby
// #define Reserved					42–255				// Reserved

typedef struct icmpheader
{
	uint32_t type;
	uint32_t code;
	uint16_t checksum;
	// uint16_t data;
} __attribute__((packed)) icmpheader_t;

void internetChecksum();

#endif
