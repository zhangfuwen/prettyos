#ifndef EHCI_H
#define EHCI_H

#include "os.h"

struct ehci_CapRegs
{
    volatile uint8_t  CAPLENGTH;        // Core Capability Register Length
    volatile uint8_t  reserved;
    volatile uint16_t HCIVERSION;       // Core Interface Version Number
    volatile uint32_t HCSPARAMS;        // Core Structural Parameters
    volatile uint32_t HCCPARAMS;        // Core Capability Parameters
    volatile uint32_t HCSPPORTROUTE_Hi; // Core Companion Port Route Description
    volatile uint32_t HCSPPORTROUTE_Lo; // Core Companion Port Route Description
} __attribute__((packed));


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
    volatile uint32_t CONFIGFLAG;       // Configured Flag Register        Aux   // +40h
    volatile uint32_t PORTSC[16];       // Port Status/Control             Aux   // +44h
} __attribute__((packed));

extern struct ehci_CapRegs* pCapRegs; // = &CapRegs;
extern struct ehci_OpRegs*  pOpRegs;  // = &OpRegs;

extern bool      EHCIflag;
extern bool      USBINTflag;

extern void*     DataQTD;
extern void*     SetupQTD;
extern uintptr_t DataQTDpage0;
extern uintptr_t MSDStatusQTDpage0;
extern uintptr_t SetupQTDpage0;




/* ****** */
/* USBCMD */
/* ****** */

#define CMD_INTERRUPT_THRESHOLD        0x00FF0000 /* valid values are: */
#define CMD_1_MICROFRAME               0x00010000
#define CMD_2_MICROFRAME               0x00020000
#define CMD_4_MICROFRAME               0x00040000
#define CMD_8_MICROFRAME               0x00080000 /* 1ms */
#define CMD_16_MICROFRAME              0x00100000
#define CMD_32_MICROFRAME              0x00200000
#define CMD_64_MICROFRAME              0x00400000

#define CMD_PARK_MODE                  0x00000800
#define CMD_PARK_COUNT                 0x00000300

#define CMD_LIGHT_RESET                0x00000080

#define CMD_ASYNCH_INT_DOORBELL        0x00000040
#define CMD_ASYNCH_ENABLE              0x00000020

#define CMD_PERIODIC_ENABLE            0x00000010

#define CMD_FRAMELIST_SIZE             0x0000000C /* valid values are: */
#define CMD_FRAMELIST_1024             0x00000000
#define CMD_FRAMELIST_512              0x00000004
#define CMD_FRAMELIST_256              0x00000008

#define CMD_HCRESET                    0x00000002 /* reset */
#define CMD_RUN_STOP                   0x00000001 /* run/stop */


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

#define PLB_ALIGNMENT                  0x00000FFF  /* 4kB */


/* ************* */
/* ASYNCLISTADDR */
/* ************* */

#define ALB_ALIGNMENT                  0x0000001F  /* 32B */


/* ********** */
/* CONFIGFLAG */
/* ********** */

#define CF                             0x00000001


/* *********** */
/* PORTSC[...] */
/* *********** */

#define PSTS_WKOC_E                    0x00400000 /* rw */
#define PSTS_WKDSCNNT_E                0x00200000 /* rw */
#define PSTS_WKCNNT_E                  0x00100000 /* rw */

#define PSTS_PORT_TEST                 0x000F0000 /* rw valid: */
#define PSTS_J_STATE                   0x00010000
#define PSTS_TEST_K_STATE              0x00020000
#define PSTS_TEST_SE0_NAK              0x00030000
#define PSTS_TEST_PACKET               0x00040000
#define PSTS_TEST_FORCE_ENABLE         0x00050000

#define PSTS_PORT_INDICATOR            0x0000C000 /* rw valid: */
#define PSTS_PI_OFF                    0x00000000
#define PSTS_PI_AMBER                  0x00004000
#define PSTS_PI_GREEN                  0x00008000

#define PSTS_COMPANION_HC_OWNED        0x00002000 /* rw */
#define PSTS_POWERON                   0x00001000 /* rw valid, if PPC == 1 */

#define PSTS_LINE_STATUS               0x00000C00 /* ro meanings: */
#define PSTS_LINE_SE0                  0x00000000
#define PSTS_LINE_J_STATE              0x00000800
#define PSTS_LINE_K_STATE              0x00000400

#define PSTS_PORT_RESET                0x00000100 /* rw */
#define PSTS_PORT_SUSPEND              0x00000080 /* rw */
#define PSTS_PORT_RESUME               0x00000040 /* rw */

#define PSTS_OVERCURRENT_CHANGE        0x00000020 /* rwc */
#define PSTS_OVERCURRENT               0x00000010 /* ro */

#define PSTS_ENABLED_CHANGE            0x00000008 /* rwc */
#define PSTS_ENABLED                   0x00000004 /* rw */

#define PSTS_CONNECTED_CHANGE          0x00000002 /* rwc */
#define PSTS_CONNECTED                 0x00000001 /* ro */

/***************************************************************/



void ehci_install(uint32_t num, uint32_t i);
void analyzeEHCI(uintptr_t bar, uintptr_t offset);
void ehci_init(); // for thread with own console
void startEHCI();
int32_t initEHCIHostController();
void startHostController(uint32_t num);
void resetHostController();
void DeactivateLegacySupport(uint32_t num);
void enablePorts();
void resetPort(uint8_t j);

void ehci_handler(registers_t* r);
void ehci_portcheck(); // for thread with own console
void portCheck();
void showPORTSC();
void checkPortLineStatus(uint8_t j);

void setupUSBDevice(uint8_t portNumber);

void showUSBSTS();

#endif
