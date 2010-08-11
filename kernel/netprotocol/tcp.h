#ifndef TCP_H
#define TCP_H

// http://tools.ietf.org/html/rfc793 <--- Transmission Control Protocol

#include "ipTcpStack.h"
#include "types.h"

typedef struct tcpheader
{
	uint16_t source_port;
	uint16_t destination_port;
	uint32_t sequence_number;
	uint32_t acknowledgment_number;
	int8_t data_offset[4];
	int8_t reserved[8];
	bool URG;                           // Urgent Pointer field significant
	bool ACK;                           // Acknowledgment field significant
	bool PSH;                           // Push Function
	bool RST;                           // Reset the connection
	bool SYN;                           // Synchronize sequence numbers
	bool FIN;                           // No more data from sender
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_pointer;
	int8_t options[24];
	uint8_t padding[8];
	// uint32_t data;
} __attribute__((packed)) tcpheader_t;

typedef struct tcppacket
{
	ethernet_t eth;
	ip_t ip;
	tcpheader_t tcp;
} __attribute__((packed)) tcphpacket_t;


// Binds the connection to a local portnumber and IP address.
void tcpBind();

// Set the state of the connection to be LISTEN.
void tcpListen();

// Connects to another host.
void tcpConnect();

void tcpRecv();
void tcpSend();

void tcpDebug(tcpheader_t *tcp);


#endif