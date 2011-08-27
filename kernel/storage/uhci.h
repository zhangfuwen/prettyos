#ifndef UHCI_H
#define UHCI_H

#include "os.h"
#include "pci.h"

struct uhci_OpRegs
{
    uint16_t UHCI_USBCMD;           // USB Command                     +00h
    uint16_t UHCI_USBSTS;           // USB Status                      +02h
    uint16_t UHCI_USBINTR;          // USB Interrupt Enable            +04h
    uint16_t UHCI_FRNUM;            // Frame Number                    +06h
    uint32_t UHCI_FRBASEADD;        // Frame List Base Address         +08h
    uint8_t  UHCI_SOFMOD;           // Start Of Frame Modify           +0Ch
    uint8_t  reserved1;             // Reserved                        +0Dh
    uint8_t  reserved2;             // Reserved                        +0Eh
    uint8_t  reserved3;             // Reserved                        +0Fh
    uint16_t UHCI_PORTSC1;          // Port 1 Status/Control           +10h
    uint16_t UHCI_PORTSC2;          // Port 2 Status/Control           +12h
} __attribute__((packed));

extern struct uhci_OpRegs*  puhci_OpRegs;  // = &OpRegs;

/* ****** */
/* USBCMD */
/* ****** */

#define MAXP     BIT(7)
#define CF       BIT(6)
#define SWDBG    BIT(5)
#define FGR      BIT(4)
#define EGSM     BIT(3)
#define GRESET   BIT(2)
#define HCRESET  BIT(1)
#define RS       BIT(0)


/* ******* */
/* USBSTS  */
/* ******* */

#define STS_HCHALTED            BIT(5)
#define STS_HC_PROCESS_ERROR    BIT(4)
#define STS_HOST_SYSTEM_ERROR   BIT(3)
#define STS_RESUME_DETECT       BIT(2)
#define STS_USB_ERROR           BIT(1)
#define STS_USBINT              BIT(0)

/* ******* */
/* USBINTR */
/* ******* */

#define SHORT_PACKET_INT_ENABLE BIT(3)
#define IOC_ENABLE              BIT(2)
#define RESUME_INT_ENABLE       BIT(1)
#define TIMEOUT_INT_ENABLE      BIT(0)

/* ******* */
/* PORTSC  */
/* ******* */

#define SUSPEND                 BIT(12)
#define PORT_RESET              BIT(9)
#define PORT_LOWSPEED_DEVICE    BIT(8)
#define PORT_RESUME_DETECT      BIT(6)
#define PORT_ENABLE_CHANGE      BIT(3)
#define PORT_ENABLE             BIT(2)
#define PORT_CS_CHANGE          BIT(1)
#define PORT_CS                 BIT(0)

/* *************** */
/* LEGACY SUPPORT  */
/* *************** */

// Register in PCI (uint16_t)
#define UHCI_PCI_LEGACY_SUPPORT          0xC0

// Interrupt carried out as a PCI interrupt
#define UHCI_PCI_LEGACY_SUPPORT_PIRQ     0x2000

// RO Bits
#define UHCI_PCI_LEGACY_SUPPORT_NO_CHG   0x5040

// Status bits that are cleared by setting to 1
#define UHCI_PCI_LEGACY_SUPPORT_STATUS   0x8F00


// Transfer Descriptors (TD) are always aligned on 16-byte boundaries.
// All transfer descriptors have the same basic, 32-byte structure.
// The last 4 DWORDs are for software use.

typedef
struct uhci_td
{
    // pointer to another TD or QH
    // inclusive control bits  (DWORD 0)
    uint32_t next;

    // TD CONTROL AND STATUS  (DWORD 1)
    uint32_t actualLength      : 11; //  0-10
    uint32_t reserved1         :  5; // 11-15
    uint32_t reserved2         :  1; //    16
    uint32_t bitstuffError     :  1; //    17
    uint32_t crc_timeoutError  :  1; //    18
    uint32_t nakReceived       :  1; //    19
    uint32_t babbleDetected    :  1; //    20
    uint32_t dataBufferError   :  1; //    21
    uint32_t stall             :  1; //    22
    uint32_t active            :  1; //    23
    uint32_t intOnComplete     :  1; //    24
    uint32_t isochrSelect      :  1; //    25
    uint32_t lowSpeedDevice    :  1; //    26
    uint32_t errorCounter      :  2; // 27-28
    uint32_t shortPacketDetect :  1; //    29
    uint32_t reserved3         :  2; // 30-31

    // TD TOKEN  (DWORD 2)
    uint32_t PacketID          :  8; //  0-7
    uint32_t deviceAddress     :  7; //  8-14
    uint32_t endpoint          :  4; // 15-18
    uint32_t dataToggle        :  1; //    19
    uint32_t reserved4         :  1; //    20
    uint32_t maxLength         : 11; // 21-31

    // TD BUFFER POINTER (DWORD 3)
    uint32_t         buffer;

    // RESERVED FOR SOFTWARE (DWORDS 4-7)
    struct uhci_td*  q_next;
    uint32_t         dWord5; // ?
    uint32_t         dWord6; // ?
    uint32_t         dWord7; // ?
  } __attribute__((packed)) uhci_TD_t;



// Queue Heads support the requirements of Control, Bulk, and Interrupt transfers
// and must be aligned on a 16-byte boundary
typedef
struct uhci_qh
{
    // QUEUE HEAD LINK POINTER
    // inclusive control bits (DWORD 0)
    uint32_t   next;

    // QUEUE ELEMENT LINK POINTER
    // inclusive control bits (DWORD 1)
    uint32_t   transfer;

    // TDs
    uhci_TD_t* q_first;
    uhci_TD_t* q_last;
} __attribute__((packed)) uhci_QH_t;


// functions
void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys);
void analyzeUHCI(uintptr_t bar, uintptr_t offset);
void uhci_init(void* data, size_t size);
void startUHCI();
int32_t initUHCIHostController();
void uhci_startHostController(pciDev_t* PCIdev);
void uhci_resetHostController();
void uhci_DeactivateLegacySupport(pciDev_t* PCIdev);
void uhci_handler(registers_t* r, pciDev_t* device);
void uhci_showUSBSTS();


#endif
