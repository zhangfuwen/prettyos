#ifndef AC97_H
#define AC97_H

#include "os.h"
#include "pci.h"

#define PORT_NAM_RESET          0x0000
#define PORT_NAM_MASTER_VOLUME  0x0002
#define PORT_NAM_MONO_VOLUME    0x0006
#define PORT_NAM_PC_BEEP        0x000A
#define PORT_NAM_PCM_VOLUME     0x0018
#define PORT_NAM_EXT_AUDIO_ID   0x0028
#define PORT_NAM_EXT_AUDIO_STC  0x002A
#define PORT_NAM_FRONT_SPLRATE  0x002C
#define PORT_NAM_LR_SPLRATE     0x0032
#define PORT_NABM_POBDBAR       0x0010
#define PORT_NABM_POLVI         0x0015
#define PORT_NABM_POCONTROL     0x001B
#define PORT_NABM_GLB_CTRL_STAT 0x0060

struct buf_desc
{
    void*    buf;
    uint16_t len;
    uint16_t reserved : 14;
    uint16_t bup :       1;
    uint16_t ioc :       1;
} __attribute__((packed));

void install_AC97(pciDev_t* device);


#endif
