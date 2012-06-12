#ifndef AC97_H
#define AC97_H

#include "os.h"
#include "pci.h"

/*
// comes from SB16:
typedef struct
{
    uint8_t masterleft, masterright;
    uint8_t vocleft, vocright;
    uint8_t midileft, midiright;
    uint8_t cdleft ,cdright;
    uint8_t lineleft, lineright;
    uint8_t micvolume;
    uint8_t pcspeaker;
    uint8_t outputswitches;
    uint8_t inputswitchesleft;
    uint8_t inputswitchesright;
    uint8_t inputgainleft, inputgainright;
    uint8_t outputgainleft, outputgainright;
    uint8_t agc;
    uint8_t trebleleft, trebleright;
    uint8_t bassleft, bassright;
} __attribute__ ((packed)) AC97_mixer_t;
*/


void install_AC97(pciDev_t* device);


#endif
