#ifndef EHCI_H
#define EHCI_H

#include "os.h"

// structs, ...

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

struct ehci_CapRegs* pCapRegs; // = &CapRegs;
struct ehci_OpRegs*  pOpRegs;  // = &OpRegs;

bool     EHCIflag;
bool     initEHCIFlag;
uint8_t  numPorts;
uint32_t ubar;
uint32_t eecp;
uint8_t* inBuffer;
void*    InQTD;
void*    SetupQTD;
uint32_t InQTDpage0;
uint32_t SetupQTDpage0;




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

struct qtd_token
{
    uint8_t status;

    uint8_t pid:          2;
    uint8_t errorCounter: 2;
    uint8_t currPage:     3;
    uint8_t interrupt:    1;
    uint16_t bytes:      15;
    uint16_t dataToggle:  1;

} __attribute__((packed));


struct ehci_qtd
{
    uint32_t next;
    uint32_t nextAlt;
    struct qtd_token token;
    uint32_t buffer0;
    uint32_t buffer1;
    uint32_t buffer2;
    uint32_t buffer3;
    uint32_t buffer4;
} __attribute__((packed));



struct ehci_qhd
{
    uint32_t horizontalPointer;
    uint32_t deviceAddress:       7;
    uint32_t inactive:            1;
    uint32_t endpoint:            4;
    uint32_t endpointSpeed:       2;
    uint32_t dataToggleControl:   1;
    uint32_t H:                   1;
     uint32_t maxPacketLength:    11;
    uint32_t controlEndpointFlag: 1;
    uint32_t nakCountReload:      4;
    uint8_t interruptScheduleMask;
    uint8_t splitCompletionMask;
    uint16_t hubAddr:             7;
    uint16_t portNumber:          7;
    uint16_t mult:                2;
    uint32_t current;
    struct ehci_qtd qtd;
} __attribute__((packed));

struct ehci_request
{
    uint8_t type;
    uint8_t request;
    uint8_t valueLo;
    uint8_t valueHi;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));


// functions, ...

void ehci_handler(registers_t* r);

void analyzeEHCI(uint32_t bar);

void resetHostController();
void startHostController(uint32_t num);
int32_t initEHCIHostController();
void DeactivateLegacySupport(uint32_t num);
void enablePorts();

void showUSBSTS();
void showPORTSC();

void checkPortLineStatus();
void resetPort(uint8_t j);

void createQH(void* address, uint32_t horizPtr, void* firstQTD, uint8_t H, uint32_t device);
void* createQTD(uint32_t next, uint8_t pid, bool toggle, uint32_t tokenBytes);
void showStatusbyteQTD(void* addressQTD);
void showPacket(uint32_t virtAddrBuf0, uint32_t size);

//void showMEM_();
//void showMEM(void* address, uint8_t n, const char* str);
#endif
