#ifndef USB_H
#define USB_H

#include "os.h"
#include "devicemanager.h"

#define OUT   0
#define IN    1
#define SETUP 2


typedef struct
{
    uint8_t   type;
    uint8_t   request;
    uint8_t   valueLo;
    uint8_t   valueHi;
    uint16_t  index;
    uint16_t  length;
} __attribute__((packed)) usb_request_t;

typedef struct
{
    uint16_t mps;
    bool     toggle;
} usb_endpoint_t;

typedef struct
{
    disk_t*  disk;

    uint16_t  usbSpec;
    uint8_t   usbClass;
    uint8_t   usbSubclass;
    uint8_t   usbProtocol;
    uint16_t  vendor;
    uint16_t  product;
    uint16_t  releaseNumber;
    uint8_t   manufacturerStringID;
    uint8_t   productStringID;
    uint8_t   serNumberStringID;
    uint8_t   numConfigurations;
    uint8_t   maxLUN;
    uint8_t   num;

    usb_endpoint_t* endpoints;

    // MSD specific
    char     productName[16];
    char     serialNumber[13];
    uint8_t  numInterfaceMSD;
    uint8_t  InterfaceClass;
    uint8_t  InterfaceSubclass;
    uint8_t  numEndpointInMSD;
    uint8_t  numEndpointOutMSD;
} usb_device_t;

struct usb_deviceDescriptor
{
   uint8_t  length;            // 18
   uint8_t  descriptorType;    // 1
   uint16_t bcdUSB;            // e.g. 0x0210 means 2.10
   uint8_t  deviceClass;
   uint8_t  deviceSubclass;
   uint8_t  deviceProtocol;
   uint8_t  maxPacketSize;     // MPS0, must be 8,16,32,64
   uint16_t idVendor;
   uint16_t idProduct;
   uint16_t bcdDevice;         // release of the device
   uint8_t  manufacturer;
   uint8_t  product;
   uint8_t  serialNumber;
   uint8_t  numConfigurations; // number of possible configurations
} __attribute__((packed));

struct usb_configurationDescriptor
{
   uint8_t  length;            // 9
   uint8_t  descriptorType;    // 2
   uint16_t totalLength;
   uint8_t  numInterfaces;
   uint8_t  configurationValue;
   uint8_t  configuration;
   uint8_t  attributes;
   uint8_t  maxPower;
} __attribute__((packed));

struct usb_interfaceDescriptor
{
   uint8_t  length;            // 9
   uint8_t  descriptorType;    // 4
   uint8_t  interfaceNumber;
   uint8_t  alternateSetting;
   uint8_t  numEndpoints;
   uint8_t  interfaceClass;
   uint8_t  interfaceSubclass;
   uint8_t  interfaceProtocol;
   uint8_t  interface;
} __attribute__((packed));

struct usb_endpointDescriptor
{
   uint8_t  length;            // 7
   uint8_t  descriptorType;    // 5
   uint8_t  endpointAddress;
   uint8_t  attributes;
   uint16_t maxPacketSize;
   uint8_t  interval;
} __attribute__((packed));

struct usb_stringDescriptor
{
   uint8_t  length;            // ?
   uint8_t  descriptorType;    // 3
   uint16_t languageID[10];    // n = 10 test-wise
} __attribute__((packed));

struct usb_stringDescriptorUnicode
{
   uint8_t  length;            // 2 + 2 * numUnicodeCharacters
   uint8_t  descriptorType;    // 3
   uint8_t  widechar[30];      // n = 30 test-wise
} __attribute__((packed));


uint8_t usb_setDeviceAddress(port_t* port, uint8_t num);
bool usb_getDeviceDescriptor(usb_device_t* device);
bool usb_getConfigDescriptor(usb_device_t* device);
void usb_getStringDescriptor(usb_device_t* device);
void usb_getUnicodeStringDescriptor(usb_device_t* device, uint32_t stringIndex);
void usb_setConfiguration(usb_device_t* device, uint32_t configuration);
uint8_t usb_getConfiguration(usb_device_t* device);
uint16_t usb_getStatus(usb_device_t* device, uint32_t endpoint);

void usb_setFeatureHALT(usb_device_t* device, uint32_t endpoint);
void usb_clearFeatureHALT(usb_device_t* device, uint32_t endpoint);


#endif
