#ifndef TCP_H
#define TCP_H

// http://tools.ietf.org/html/rfc793 <--- Transmission Control Protocol

#include "ethernet.h"
#include "ipv4.h"
#include "types.h"

typedef struct tcpheader
{
    uint16_t sourcePort;
    uint16_t destinationPort;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    int8_t data_offset[4];
    int8_t reserved;
    //Flags
    bool CWR : 1;                           // Congestion Window Reduced 
    bool ECN : 1;                           // ECN-Echo
    bool URG : 1;                           // Urgent 
    bool ACK : 1;                           // Acknowledgment 
    bool PSH : 1;                           // Push 
    bool RST : 1;                           // Reset 
    bool SYN : 1;                           // Synchronize sequence numbers
    bool FIN : 1;                           // No more data from sender
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;
    int8_t options[8];
    uint8_t padding[8];
    // uint32_t data;
} __attribute__((packed)) tcpheader_t;

typedef struct tcppacket
{
    ethernet_t eth;
    ip_t ip;
    tcpheader_t tcp;
} __attribute__((packed)) tcppacket_t;


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