#ifndef EHCI_QH_QTD_H
#define EHCI_QH_QTD_H

#include "os.h"
#include "ehci.h"

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
    uint32_t extend0;
    uint32_t extend1;
    uint32_t extend2;
    uint32_t extend3;
    uint32_t extend4;
} __attribute__((packed));
typedef struct ehci_qtd ehci_qtd_t;

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
typedef struct ehci_qhd ehci_qhd_t;

struct ehci_request
{
    uint8_t type;
    uint8_t request;
    uint8_t valueLo;
    uint8_t valueHi;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));

ehci_qtd_t* allocQTD(uintptr_t next);
uintptr_t allocQTDbuffer(ehci_qtd_t* td);

void createQH(void* address, uint32_t horizPtr, void* firstQTD, uint8_t H, uint32_t device, uint32_t endpoint, uint32_t packetSize);
void* createQTD_SETUP(uintptr_t next, bool toggle, uint32_t tokenBytes, uint32_t type, uint32_t req, uint32_t hiVal, uint32_t loVal, uint32_t index, uint32_t length);
void* createQTD_IO(uintptr_t next, uint8_t direction, bool toggle, uint32_t tokenBytes);
void* createQTD_MSDStatus(uintptr_t next, bool toggle);
void* createQTD_Handshake(uint8_t direction);

void showStatusbyteQTD(void* addressQTD);
void showPacket(uint32_t virtAddrBuf0, uint32_t size);
void showPacketAlphaNumeric(uint32_t virtAddrBuf0, uint32_t size);

void analyzeAsyncList();

#endif
