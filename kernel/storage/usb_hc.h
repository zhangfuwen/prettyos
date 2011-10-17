#ifndef USB_HC_H
#define USB_HC_H

#include "devicemanager.h"
#include "pci.h"

#define UHCI    0x00
#define OHCI    0x10
#define EHCI    0x20
#define XHCI    0x30
#define NO_HCI  0x80
#define ANY_HCI 0xFE


typedef enum
{
    USB_BULK, USB_CONTROL, USB_INTERRUPT, USB_ISOCHRONOUS
} usb_tranferType_t;

typedef struct
{
    void*             data; // EHCI: Contains pointer to QH
    usb_tranferType_t type;
    uint32_t          device;
    uint32_t          endpoint;
    uint32_t          packetSize;
    port_t*           HC;
    list_t*           transactions;
    bool              success;
} usb_transfer_t;

typedef enum
{
    USB_TT_SETUP, USB_TT_IN, USB_TT_OUT
} usb_transactionType_t;

typedef struct
{
    void*                 data; // EHCI: Contains pointer to ehci_transaction_t
    usb_transactionType_t type;
} usb_transaction_t;


void usb_hc_install(pciDev_t* PCIdev);

void usb_setupTransfer(port_t* usbPort, usb_transfer_t* transfer, usb_tranferType_t type, uint32_t endpoint, size_t maxLength);
void usb_setupTransaction(usb_transfer_t* transfer, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length);
void usb_inTransaction(usb_transfer_t* transfer, bool toggle, void* buffer, size_t length);
void usb_outTransaction(usb_transfer_t* transfer, bool toggle, void* buffer, size_t length);
void usb_issueTransfer(usb_transfer_t* transfer);


#endif
