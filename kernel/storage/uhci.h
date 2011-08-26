#ifndef UHCI_H
#define UHCI_H

#include "os.h"
#include "pci.h"

struct uhci_OpRegs
{
    volatile uint16_t UHCI_USBCMD;           // USB Command                     +00h
    volatile uint16_t UHCI_USBSTS;           // USB Status                      +02h
    volatile uint16_t UHCI_USBINTR;          // USB Interrupt Enable            +04h
    volatile uint16_t UHCI_FRNUM;            // Frame Number                    +06h
    volatile uint32_t UHCI_FRBASEADD;        // Frame List Base Address         +08h
    volatile uint8_t  UHCI_SOFMOD;           // Start Of Frame Modify           +0Ch
    volatile uint8_t  UHCI_RESERVED1;        // Reserved                        +0Dh 
    volatile uint8_t  UHCI_RESERVED2;        // Reserved                        +0Eh 
    volatile uint8_t  UHCI_RESERVED3;        // Reserved                        +0Fh 
    volatile uint16_t UHCI_PORTSC1;          // Port 1 Status/Control           +10h
    volatile uint16_t UHCI_PORTSC2;          // Port 2 Status/Control           +12h
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
#define STS_ERROR_INTERRUPT     BIT(1)
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
#define PORT_LOWSPEED           BIT(8)
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



#endif
