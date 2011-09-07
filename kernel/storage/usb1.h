#ifndef USB1_H
#define USB1_H

#include "os.h"

#define OUT   0
#define IN    1
#define SETUP 2


typedef struct
{
    uint16_t usbSpec;
    uint8_t  usbClass;
    uint8_t  usbSubclass;
    uint8_t  usbProtocol;
    uint8_t  maxPacketSize;
    uint16_t vendor;
    uint16_t product;
    uint16_t releaseNumber;
    uint8_t  manufacturerStringID;
    uint8_t  productStringID;
    uint8_t  serNumberStringID;
    uint8_t  numConfigurations;
    uint8_t  maxLUN;

    // MSD specific
    char     productName[16];
    char     serialNumber[13];
    uint8_t  numInterfaceMSD;
    uint8_t  InterfaceClass;
    uint8_t  InterfaceSubclass;
    uint8_t  numEndpointInMSD;
    uint8_t  numEndpointOutMSD;
    bool     ToggleEndpointInMSD;
    bool     ToggleEndpointOutMSD;
} usb1_Device_t;

struct usb1_deviceDescriptor
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

struct usb1_configurationDescriptor
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

struct usb1_interfaceDescriptor
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

struct usb1_endpointDescriptor
{
   uint8_t  length;            // 7
   uint8_t  descriptorType;    // 5
   uint8_t  endpointAddress;
   uint8_t  attributes;
   uint16_t maxPacketSize;
   uint8_t  interval;
} __attribute__((packed));

struct usb1_stringDescriptor
{
   uint8_t  length;            // ?
   uint8_t  descriptorType;    // 3
   uint16_t languageID[10];    // n = 10 test-wise
} __attribute__((packed));

struct usb1_stringDescriptorUnicode
{
   uint8_t  length;            // 2 + 2 * numUnicodeCharacters
   uint8_t  descriptorType;    // 3
   uint8_t  widechar[30];      // n = 30 test-wise
   uint8_t  asciichar[30];
} __attribute__((packed));


uint8_t usb1_TransferEnumerate(uint8_t j);
void usb1_TransferDevice(uint32_t device);
void usb1_TransferConfig(uint32_t device);
void usb1_TransferString(uint32_t device);
void usb1_TransferStringUnicode(uint32_t device, uint32_t stringIndex);
void usb1_TransferSetConfiguration(uint32_t device, uint32_t configuration);
uint8_t usb1_TransferGetConfiguration(uint32_t device);
void usb1_SetFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize);
void usb1_ClearFeatureHALT(uint32_t device, uint32_t endpoint, uint32_t packetSize);
uint16_t usb1_GetStatus(uint32_t device, uint32_t endpoint, uint32_t packetSize);
void usb1_addDevice(struct usb1_deviceDescriptor* d, usb1_Device_t* usbDev);
void usb1_showDevice(usb1_Device_t* usbDev);
void usb1_showConfigurationDescriptor(struct usb1_configurationDescriptor* d);
void usb1_showInterfaceDescriptor(struct usb1_interfaceDescriptor* d);
void usb1_showEndpointDescriptor(struct usb1_endpointDescriptor* d);
void usb1_showStringDescriptor(struct usb1_stringDescriptor* d);
void usb1_showStringDescriptorUnicode(struct usb1_stringDescriptorUnicode* d, uint32_t device, uint32_t stringIndex);


#endif
