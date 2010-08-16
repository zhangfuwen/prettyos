#ifndef IPV4_H
#define IPV4_H

#include "os.h"

typedef struct ip
{    
    uint8_t  ipHeaderLength   :4;
    uint8_t  version          :4;
    uint8_t  typeOfService;
    uint16_t length;
    uint16_t identification;
    uint16_t fragmentation;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  sourceIP[4];
    uint8_t  destIP[4];
} __attribute__((packed)) ip_t;

#endif