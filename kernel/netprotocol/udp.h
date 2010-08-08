#ifndef UDP_H
#define UDP_H

#include "ipTcpStack.h"

#define MAX_UDP_PORTS 65536

typedef struct udp
{
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t udp_length;
	uint16_t udp_checksum;
} __attribute__((packed)) udp_t;

void UDPSend();
void UDPRecv();

#endif