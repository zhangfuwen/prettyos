#ifndef UDP_H
#define UDP_H

#include "ipTcpStack.h"

#define MAX_UDP_PORTS 65536

typedef struct udpheader
{
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t udp_length;
	uint16_t udp_checksum;
} __attribute__((packed)) udpheader_t;

typedef struct udpPacket // ???
{
	ip_t ip;
	udpheader_t updheader;
} __attribute__((packed)) udppacket_t;

void UDPConnect();
void UDPBind();
void UDPSend(void* data);
void UDPRecv(void* data);

#endif
