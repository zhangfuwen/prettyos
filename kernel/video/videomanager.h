#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include "os.h"
#include "list.h"


typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha; // Used to store 8-bit color equivalent at the moment
} __attribute__((packed)) BGRA_t;

// Device driver interface
struct videoDevice;
struct videoMode;
typedef struct
{
    const char* driverName;

    // Setup functions
    size_t (*detect)(); // Returns number of hardware devices detected and supported by this driver
    void   (*createDevice)(struct videoDevice*); // Sets up the device
    void   (*freeDevice)(struct videoDevice*); // Uninstalls the devices, frees memory
    void   (*createModeList)(struct videoDevice*, list_t*); // Fills in all available video modes
    void   (*freeVideoMode)(struct videoMode*); // Frees memory allocated for this mode
    void   (*enterVideoMode)(struct videoMode*); // Switches to a video mode
    void   (*leaveVideoMode)(struct videoDevice*); // Leaves current video mode

    // Basic drawing functions
    void (*setPixel)(struct videoDevice*, uint16_t, uint16_t, BGRA_t); // Sets a single pixel on the screen
    void (*fillPixels)(struct videoDevice*, uint16_t, uint16_t, BGRA_t, size_t); // Fills an array. Caller may assume that the function continues at next line after line end
    void (*copyPixels)(struct videoDevice*, uint16_t, uint16_t, BGRA_t*, size_t); // Copies an array of pixels. Caller may assume that the function continues at next line after line end
    void (*clear)(struct videoDevice*, BGRA_t);

    // Other
    BGRA_t (*getPixel)(struct videoDevice*, uint16_t, uint16_t);
    void   (*flipScreen)(struct videoDevice*);
} videoDeviceDriver_t;

typedef enum
{
    VMT_NONE, VMT_TEXT, VMT_GRAPHIC
} videoModeType_t;

typedef enum
{
    CM_2COL = 0,
    CM_16COL = 4,
    CM_256COL = 8,
    CM_15BIT = 15,
    CM_16BIT = 16,
    CM_24BIT = 24,
    CM_32BIT = 32
} colorMode_t;

typedef struct videoMode
{
    struct videoDevice* device;
    void*               data; // Internal data of driver specific to this video mode (for example mode number)
    uint16_t            xRes, yRes;
    bool                doubleBuffer;
    videoModeType_t     type;
    colorMode_t         colorMode;
    BGRA_t*             palette;
} videoMode_t;

typedef struct videoDevice
{
    videoDeviceDriver_t* driver;
    videoMode_t          videoMode;
    void*                data; // Internal data of driver specific to this device
} videoDevice_t;

enum
{
    VDD_RENDERBUFFER, VDD_VGA, VDD_VBE, VDD_COUNT
};


extern videoDeviceDriver_t video_drivers[VDD_COUNT];
extern videoMode_t* video_currentMode;


// General functions
void video_install();
void video_test();
void video_setMode(videoMode_t* mode);
void video_createModeList(list_t* list);
void video_freeModeList(list_t* list);
videoDevice_t* video_createDevice(videoDeviceDriver_t* driver);
void video_freeDevice(videoDevice_t* device);

// Basic drawing functionality
void   video_setPixel(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t color);                 // Sets a single pixel on the screen
void   video_fillPixels(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t color, size_t num);   // Sets pixels on the screen
void   video_copyPixels(videoDevice_t* device, uint32_t x, uint32_t y, BGRA_t* pixels, size_t num); // Copies pixels to screen
void   video_clearScreen(videoDevice_t* device, BGRA_t color);                                      // Fills the whole screen with the given color

void   video_flipScreen(videoDevice_t* device);
BGRA_t video_getPixel(videoDevice_t* device, uint32_t x, uint32_t y);                               // Returns the color of a single pixel on the screen


#endif
