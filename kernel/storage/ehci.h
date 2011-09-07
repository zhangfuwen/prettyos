#ifndef EHCI_H
#define EHCI_H

#include "os.h"
#include "pci.h"


/* ****** */
/* USBCMD */
/* ****** */

#define CMD_INTERRUPT_THRESHOLD        0x00FF0000 // valid values are:
#define CMD_1_MICROFRAME               0x00010000
#define CMD_2_MICROFRAME               0x00020000
#define CMD_4_MICROFRAME               0x00040000
#define CMD_8_MICROFRAME               0x00080000 // 1ms
#define CMD_16_MICROFRAME              0x00100000
#define CMD_32_MICROFRAME              0x00200000
#define CMD_64_MICROFRAME              0x00400000

#define CMD_PARK_MODE                  0x00000800
#define CMD_PARK_COUNT                 0x00000300

#define CMD_LIGHT_RESET                0x00000080

#define CMD_ASYNCH_INT_DOORBELL        0x00000040
#define CMD_ASYNCH_ENABLE              0x00000020

#define CMD_PERIODIC_ENABLE            0x00000010

#define CMD_FRAMELIST_SIZE             0x0000000C // valid values are:
#define CMD_FRAMELIST_1024             0x00000000
#define CMD_FRAMELIST_512              0x00000004
#define CMD_FRAMELIST_256              0x00000008

#define CMD_HCRESET                    0x00000002 // reset
#define CMD_RUN_STOP                   0x00000001 // run/stop


/* ************** */
/* USBSTS USBINTR */
/* ************** */

// only USBSTS
#define STS_ASYNC_ENABLED              0x00008000
#define STS_PERIODIC_ENABLED           0x00004000
#define STS_RECLAMATION                0x00002000
#define STS_HCHALTED                   0x00001000

// USBSTS (Interrupts)
#define STS_INTMASK                    0x0000003F
#define STS_ASYNC_INT                  0x00000020
#define STS_HOST_SYSTEM_ERROR          0x00000010
#define STS_FRAMELIST_ROLLOVER         0x00000008
#define STS_PORT_CHANGE                0x00000004
#define STS_USBERRINT                  0x00000002
#define STS_USBINT                     0x00000001


/* *********/
/* FRINDEX */
/* *********/

#define FRI_MASK                       0x00001FFF


/* **************** */
/* PERIODICLISTBASE */
/* **************** */

#define PLB_ALIGNMENT                  0x00000FFF  // 4kB


/* ************* */
/* ASYNCLISTADDR */
/* ************* */

#define ALB_ALIGNMENT                  0x0000001F  // 32B


/* ********** */
/* CONFIGFLAG */
/* ********** */

#define CF                             0x00000001


/* *********** */
/* PORTSC[...] */
/* *********** */

#define PSTS_WKOC_E                    0x00400000 // rw
#define PSTS_WKDSCNNT_E                0x00200000 // rw
#define PSTS_WKCNNT_E                  0x00100000 // rw

#define PSTS_PORT_TEST                 0x000F0000 // rw valid:
#define PSTS_J_STATE                   0x00010000
#define PSTS_TEST_K_STATE              0x00020000
#define PSTS_TEST_SE0_NAK              0x00030000
#define PSTS_TEST_PACKET               0x00040000
#define PSTS_TEST_FORCE_ENABLE         0x00050000

#define PSTS_PORT_INDICATOR            0x0000C000 // rw valid:
#define PSTS_PI_OFF                    0x00000000
#define PSTS_PI_AMBER                  0x00004000
#define PSTS_PI_GREEN                  0x00008000

#define PSTS_COMPANION_HC_OWNED        0x00002000 // rw
#define PSTS_POWERON                   0x00001000 // rw valid, if PPC == 1

#define PSTS_LINE_STATUS               0x00000C00 // ro meanings:
#define PSTS_LINE_SE0                  0x00000000
#define PSTS_LINE_J_STATE              0x00000800
#define PSTS_LINE_K_STATE              0x00000400

#define PSTS_PORT_RESET                0x00000100 // rw
#define PSTS_PORT_SUSPEND              0x00000080 // rw
#define PSTS_PORT_RESUME               0x00000040 // rw

#define PSTS_OVERCURRENT_CHANGE        0x00000020 // rwc
#define PSTS_OVERCURRENT               0x00000010 // ro

#define PSTS_ENABLED_CHANGE            0x00000008 // rwc
#define PSTS_ENABLED                   0x00000004 // rw

#define PSTS_CONNECTED_CHANGE          0x00000002 // rwc
#define PSTS_CONNECTED                 0x00000001 // ro


#define N_PORTS                        0x000F     // number of ports
#define PORT_ROUTING_RULES             BIT(7);    // port routing to EHCI or cHC


struct ehci_CapRegs
{
    volatile uint8_t  CAPLENGTH;        // Core Capability Register Length
    volatile uint8_t  reserved;
    volatile uint16_t HCIVERSION;       // Core Interface Version Number
    volatile uint32_t HCSPARAMS;        // Core Structural Parameters // 
    volatile uint32_t HCCPARAMS;        // Core Capability Parameters
    volatile uint32_t HCSPPORTROUTE_Hi; // Core Companion Port Route Description
    volatile uint32_t HCSPPORTROUTE_Lo; // Core Companion Port Route Description
} __attribute__((packed));

/*
HCSP-PORTROUTE - Companion Port Route Description:

This optional field is valid only if Port Routing Rules field in the HCSPARAMS register is set to a one.

This field is a 15-element nibble array (each 4 bits is one array element). 
Each array location corresponds one-to-one with a physical port provided by the HC 
(e.g. PORTROUTE[0] corresponds to the first PORTSC port, PORTROUTE[1] to the second PORTSC port, etc.). 
The value of each element indicates to which of the cHC this port is routed. 
Only the first N_PORTS elements have valid information. 
A value of zero indicates that the port is routed to the lowest numbered function cHC.
A value of one indicates that the port is routed to the next lowest numbered function cHC, and so on.
*/


struct ehci_OpRegs
{
    volatile uint32_t USBCMD;           // USB Command                     Core  // +00h
    volatile uint32_t USBSTS;           // USB Status                      Core  // +04h
    volatile uint32_t USBINTR;          // USB Interrupt Enable            Core  // +08h
    volatile uint32_t FRINDEX;          // USB Frame Index                 Core  // +0Ch
    volatile uint32_t CTRLDSSEGMENT;    // 4G Segment Selector             Core  // +10h
    volatile uint32_t PERIODICLISTBASE; // Frame List Base Address         Core  // +14h
    volatile uint32_t ASYNCLISTADDR;    // Next Asynchronous List Address  Core  // +18h
    volatile uint32_t reserved1;                                                 // +1Ch
    volatile uint32_t reserved2;                                                 // +20h
    volatile uint32_t reserved3;                                                 // +24h
    volatile uint32_t reserved4;                                                 // +28h
    volatile uint32_t reserved5;                                                 // +2Ch
    volatile uint32_t reserved6;                                                 // +30h
    volatile uint32_t reserved7;                                                 // +34h
    volatile uint32_t reserved8;                                                 // +38h
    volatile uint32_t reserved9;                                                 // +3Ch
    volatile uint32_t CONFIGFLAG;       // Configure Flag Register         Aux   // +40h
    volatile uint32_t PORTSC[16];       // Port Status/Control             Aux   // +44h
} __attribute__((packed));

/*
Configure Flag (CF) - R/W. Default: 0. Host software sets this bit as the last action in
its process of configuring the HC. This bit controls the default port-routing control logic. 
Bit values and side-effects are listed below. 
0: routes each port to an implementation dependent classic HC.
1: routes all ports to the EHCI.
*/


extern struct ehci_OpRegs* OpRegs;  // = &OpRegs;

extern bool      USBINTflag;

extern void*     DataQTD;
extern void*     SetupQTD;
extern uintptr_t DataQTDpage0;
extern uintptr_t MSDStatusQTDpage0;


void ehci_install(pciDev_t* PCIdev, uintptr_t bar_phys);
void ehci_start();
void ehci_initHC();
void ehci_startHC(pciDev_t* PCIdev);
void ehci_resetHC();
void ehci_enablePorts();
void ehci_resetPort(uint8_t port);
void ehci_portCheck();

void setupUSBDevice(uint8_t portNumber);
void showUSBSTS();


#endif
