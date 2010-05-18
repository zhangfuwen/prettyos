#ifndef USB2_MSD_H
#define USB2_MSD_H

#include "os.h"
#include "ehciQHqTD.h"

#define OUT   0
#define IN    1
#define SETUP 2

struct usb2_CommandBlockWrapper
{
    uint32_t CBWSignature;
    uint32_t CBWTag;
    uint32_t CBWDataTransferLength;
    uint8_t  CBWFlags;
    uint8_t  CBWLUN;           // only bits 3:0
    uint8_t  CBWCBLength;      // only bits 4:0
    uint8_t  commandByte[16];
} __attribute__((packed));

void usbTransferBulkOnlyMassStorageReset(uint32_t device, uint8_t numInterface);
uint8_t usbTransferBulkOnlyGetMaxLUN(uint32_t device, uint8_t numInterface);

void usbSendSCSIcmd(uint32_t device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer);

void testMSD(uint8_t devAddr, uint8_t config);

void usbResetRecoveryMSD(uint32_t device, uint32_t Interface, uint32_t endpointOUT, uint32_t endpointIN);

int32_t showResultsRequestSense();

void analyzeBootSector(void* DataQTDpage0); // for first tests only

#endif
