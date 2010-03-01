#ifndef EHCI_H
#define EHCI_H

#include "os.h"

// structs, ...

struct ehci_CapRegs
{
    uint8_t  CAPLENGTH;        // Core Capability Register Length
    uint16_t HCIVERSION;       // Core Interface Version Number
    uint32_t HCSPARAMS;        // Core Structural Parameters
    uint32_t HCCPARAMS;        // Core Capability Parameters
    uint32_t HCSPPORTROUTE_Hi; // Core Companion Port Route Description
    uint32_t HCSPPORTROUTE_Lo; // Core Companion Port Route Description
};

struct ehci_OpRegs
{
    uint32_t USBCMD;           // USB Command                     Core  // +00h
    uint32_t USBSTS;           // USB Status                      Core  // +04h
    uint32_t USBINTR;          // USB Interrupt Enable            Core  // +08h
    uint32_t FRINDEX;          // USB Frame Index                 Core  // +0Ch
    uint32_t CTRLDSSEGMENT;    // 4G Segment Selector             Core  // +10h
    uint32_t PERIODICLISTBASE; // Frame List Base Address         Core  // +14h
    uint32_t ASYNCLISTADDR;    // Next Asynchronous List Address  Core  // +18h
    uint32_t CONFIGFLAG;       // Configured Flag Register        Aux   // +40h
    uint32_t PORTSC[16];       // Port Status/Control             Aux   // +44h, +48h, ...
};

bool EHCIflag;

// functions, ...

void analyzeEHCI(uint32_t bar);
void initEHCIHostController();
void showUSBSTS();
void showPORTSC();

#endif
