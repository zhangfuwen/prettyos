#ifndef USB2_MSD_H
#define USB2_MSD_H

#include "os.h"
#include "devicemanager.h"
#include "usb2.h"


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

typedef struct
{
    uint8_t  SCSIopcode;
    bool     successfulCommand;
    bool     successfulDataOUT;
    bool     successfulDataIN;
    bool     successfulCSW;
    uint32_t DataBytesToTransferOUT;
    uint32_t DataBytesToTransferIN;
} usbBulkTransfer_t;


usb2_Device_t* usb2_createDevice(disk_t* disk);
void usb2_destroyDevice(usb2_Device_t* device);
void usb_setupDevice(usb2_Device_t* device, uint8_t address);

void usbTransferBulkOnlyMassStorageReset(usb2_Device_t* device, uint8_t numInterface);
uint8_t usbTransferBulkOnlyGetMaxLUN(usb2_Device_t* device, uint8_t numInterface);

void usbSendSCSIcmd(usb2_Device_t* device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer, void* dataBuffer, void* statusBuffer);
void usbSendSCSIcmdOUT(usb2_Device_t* device, uint32_t interface, uint32_t endpointOut, uint32_t endpointIn, uint8_t SCSIcommand, uint32_t LBA, uint16_t TransferLength, usbBulkTransfer_t* bulkTransfer, void* dataBuffer, void* statusBuffer);

void testMSD(usb2_Device_t* device);
FS_ERROR usbRead (uint32_t sector, void* buffer, void* device);
FS_ERROR usbWrite(uint32_t sector, void* buffer, void* device);

void usbResetRecoveryMSD(usb2_Device_t* device, uint32_t Interface);

int32_t showResultsRequestSense(void* addr);


#endif
