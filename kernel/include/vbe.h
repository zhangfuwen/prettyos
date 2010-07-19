#ifndef VBE_H
#define VBE_H

#include "os.h"

// http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf

#define VM86_SWITCH_TO_VIDEO ((void*)0x100)
#define VM86_SWITCH_TO_TEXT  ((void*)0x118)

// SuperVGA information block
typedef struct
{
    uint8_t  VESASignature[4]; // VESA 4 byte signature
    uint16_t VESAVersion;      // VBE version number
             
    char*      OEMStringPtr;   // Pointer to OEM string
    long       Capabilities;   // Capabilities of video card
    
    uint16_t*  VideoModePtr;   // Pointer to supported modes
    uint16_t   TotalMemory;    // Number of 64kb memory blocks
    uint8_t    reserved[236];  // Pad to 256 byte block size
} VgaInfoBlock_t;

// SuperVGA mode information block 
typedef struct 
{
    uint16_t   ModeAttributes;         // Mode attributes
    uint8_t    WinAAttributes;         // Window A attributes
    uint8_t    WinBAttributes;         // Window B attributes
    uint16_t   WinGranularity;         // Window granularity in k
    uint16_t   WinSize;                // Window size in k
    uint16_t   WinASegment;            // Window A segment
    uint16_t   WinBSegment;            // Window B segment

    // void*   WinFuncPtr;             // Pointer to window function
    uint16_t   BytesPerScanLine;       // Bytes per scanline
    uint16_t   XResolution;            // Horizontal resolution
    uint16_t   YResolution;            // Vertical resolution
    uint8_t    XCharSize;              // Character cell width
    uint8_t    YCharSize;              // Character cell height
    uint8_t    NumberOfPlanes;         // Number of memory planes
    uint8_t    BitsPerPixel;           // Bits per pixel
    uint8_t    NumberOfBanks;          // Number of CGA style banks
    uint8_t    MemoryModel;            // Memory model type
    uint8_t    BankSize;               // Size of CGA style banks
    uint8_t    NumberOfImagePages;     // Number of images pages
    uint8_t    res1;                   // Reserved
    uint8_t    RedMaskSize;            // Size of direct color red mask
    uint8_t    RedFieldPosition;       // Bit posn of lsb of red mask
    uint8_t    GreenMaskSize;          // Size of direct color green mask
    uint8_t    GreenFieldPosition;     // Bit posn of lsb of green mask
    uint8_t    BlueMaskSize;           // Size of direct color blue mask
    uint8_t    BlueFieldPosition;      // Bit posn of lsb of blue mask
    uint8_t    RsvdMaskSize;           // Size of direct color res mask
    uint8_t    RsvdFieldPosition;      // Bit posn of lsb of res mask
    uint8_t    DirectColorModeInfo;    // Direct color mode attributes
    uint8_t    res2[216];              // Pad to 256 byte block size
} ModeInfoBlock_t;

typedef enum
{
    memPL   = 3,  // Planar memory model
    memPK   = 4,  // Packed pixel memory model
    memRGB  = 6,  // Direct color RGB memory model
    memYUV  = 7,  // Direct color YUV memory model
} memModels;

// bitmap the structure 
typedef struct               
{
  uint16_t width;
  uint16_t height;
  uint8_t palette[256*3];
  uint8_t *data;
} Bitmap_t;

void switchToVideomode();
void switchToTextmode();

uint32_t getVgaInfo(VgaInfoBlock_t* vgaInfo);
uint32_t getModeInfo(uint32_t mode, ModeInfoBlock_t* modeInfo);
uint32_t getVBEMode(void);
void setVBEMode(uint32_t mode);
void setBank(uint32_t bank);

void availableModes(void);
void initGraphics(uint32_t x, uint32_t y, uint32_t pixelwidth);

void setPixel(uint32_t x, uint32_t y, uint32_t color);

// should be in util.h, maybe later in math.h or something like that
float sgn(float x);
uint32_t abs(uint32_t arg);

void line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color);
void drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color);

// rendering one of the character, given its font_data

//void draw_char(unsigned char* screen, where, font_char*);
//void draw_string(unsigned char* screen, where, char* input);

#endif
