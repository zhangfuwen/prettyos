#ifndef NETUTILS_H
#define NETUTILS_H

#include "types.h"

// htonl = Host To Network Long  
#define htons(v) ((((v) >> 8) & 0xFF) | (((v) & 0xFF) << 8))
#define htonl(v) ((((v) >> 24) & 0xFF) | (((v) >> 8) & 0xFF00) | (((v) & 0xFF00) << 8) | (((v) & 0xFF) << 24))
// ntohl = Network To Host Long
#define ntohs(v) htons(v)
#define ntohl(v) htonl(v)


uint16_t internetChecksum(void* addr, size_t count, uint32_t pseudoHeaderChecksum);
uint16_t udptcpCalculateChecksum(void* p, uint16_t length, uint8_t srcIP[4], uint8_t destIP[4], uint16_t protocol);


#endif
