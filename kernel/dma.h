#ifndef CPU_H
#define CPU_H

#include "util.h"


enum DMA_CHANNEL
{
    DMA_CHANNEL_0 = 0,
    DMA_CHANNEL_1 = 1,
    DMA_CHANNEL_2 = 2,
    DMA_CHANNEL_3 = 3
};

typedef enum
{
    DMA_WRITE     = BIT(2), // Attention: Hardware -> RAM
    DMA_READ      = BIT(3), // Attention: RAM -> Hardware
    DMA_AUTOINIT  = BIT(4),
    DMA_INCREMENT = 0,
    DMA_DECREMENT = BIT(5),
    DMA_DEMAND    = 0,
    DMA_SINGLE    = BIT(6),
    DMA_BLOCK     = BIT(7)
} DMA_TRANSFERMODE_t;


typedef struct
{
    uint8_t  portNum;
    uint16_t pagePort;
    uint16_t addressPort;
    uint16_t counterPort;
} dma_channel_t;

extern dma_channel_t dma_channel[4]; // dma.c

void dma_read (void* dest,   uint16_t length, dma_channel_t* channel, DMA_TRANSFERMODE_t mode);
void dma_write(void* source, uint16_t length, dma_channel_t* channel, DMA_TRANSFERMODE_t mode);


#endif
