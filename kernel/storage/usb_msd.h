#ifndef USB_MSD_H
#define USB_MSD_H

#include "os.h"
#include "devicemanager.h"
#include "usb.h"


struct usb_CommandBlockWrapper
{
    uint32_t  CBWSignature;
    uint32_t  CBWTag;
    uint32_t  CBWDataTransferLength;
    uint8_t   CBWFlags;
    uint8_t   CBWLUN;           // only bits 3:0
    uint8_t   CBWCBLength;      // only bits 4:0
    uint8_t   commandByte[16];
} __attribute__((packed));


usb_device_t* usb_createDevice(disk_t* disk);
void          usb_destroyDevice(usb_device_t* device);
void          usb_setupDevice(usb_device_t* device, uint8_t address);

void          usb_bulkReset(usb_device_t* device, uint8_t numInterface);
uint8_t       usb_getMaxLUN(usb_device_t* device, uint8_t numInterface);
void          usb_resetRecoveryMSD(usb_device_t* device, uint32_t Interface);

void          usb_sendSCSICommand    (usb_device_t* device,
                                      uint32_t      interface,
                                      uint32_t      endpointOut,
                                      uint32_t      endpointIn,
                                      uint8_t       SCSIcommand,
                                      uint32_t      LBA,
                                      uint16_t      TransferLength,
                                      void*         dataBuffer,
                                      void*         statusBuffer);

void          usb_sendSCSICommand_out(usb_device_t* device,
                                      uint32_t      interface,
                                      uint32_t      endpointOut,
                                      uint32_t      endpointIn,
                                      uint8_t       SCSIcommand,
                                      uint32_t      LBA,
                                      uint16_t      TransferLength,
                                      void*         dataBuffer,
                                      void*         statusBuffer);

FS_ERROR      usb_read (uint32_t sector, void* buffer, disk_t* device);
FS_ERROR      usb_write(uint32_t sector, void* buffer, disk_t* device);

void          testMSD(usb_device_t* device);


#endif
