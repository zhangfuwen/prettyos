#ifndef VBE_H
#define VBE_H

#include "os.h"
#include "video.h"

// http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf
// http://poli.cs.vsb.cz/misc/rbint/text/1005.html

// This values are hardcoded adresses from documentation/vidswtch.map
#define VM86_SETDISPLAYSTART   ((void*)0x100)
#define VM86_GETDISPLAYSTART   ((void*)0x113)
#define VM86_SET8BITPALETTE    ((void*)0x128)
#define VM86_SWITCH_TO_TEXT    ((void*)0x133)
#define VM86_SWITCH_TO_VIDEO   ((void*)0x141)
#define VM86_VGAINFOBLOCK      ((void*)0x150)
#define VM86_MODEINFOBLOCK     ((void*)0x15E)


// SuperVGA information block
typedef struct
{
    uint8_t   VESASignature[4];
    uint16_t  VESAVersion;
    uintptr_t OEMStringPtr;
    uint8_t   Capabilities[4];
    uint16_t* VideoModes;
    uint16_t  TotalMemory;
    uint16_t  OemSoftwareRev;
    uintptr_t OemVendorNamePtr;
    uintptr_t OemProductNamePtr;
    uintptr_t OemProductRevPtr;
    uint8_t   Reserved[222];
    uint8_t   OemData[256];
} __attribute__ ((packed)) VgaInfoBlock_t;

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

    void*      WinFuncPtr;             // Pointer to window function
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
    uintptr_t  PhysBasePtr;            // 32-bit physical memory address
    uint32_t   OffScreenMemOffset;     // pointer to start of off screen memory
    uint16_t   OffScreenMemSize;       // amount of off screen memory in 1k units
    uint8_t    res2[206];              // Pad to 256 byte block size //212
} __attribute__((packed)) ModeInfoBlock_t;

/*typedef enum
{
    memPL   = 3,  // Planar memory model
    memPK   = 4,  // Packed pixel memory model
    memRGB  = 6,  // Direct color RGB memory model
    memYUV  = 7,  // Direct color YUV memory model
} memModels;*/

// bitmap the structure
typedef struct
{
  uint16_t width;
  uint16_t height;
  uint8_t  palette[256*4];
  uint8_t* data;
} Bitmap_t;

typedef struct
{
    uint16_t Type;            // File type. Set to "BM."
    uint32_t Size;            // Size in DWORDs of the file
    uint32_t Reserved;        // Reserved. Set to zero.
    uint32_t Offset;          // Offset to the data.
    uint32_t headerSize;      // Size of rest of header. Set to 40.
    uint32_t Width;           // Width of bitmap in pixels.
    uint32_t Height;          // Height of bitmap in pixels.
    uint16_t Planes;          // Number of Planes. Set to 1.
    uint16_t BitsPerPixel;    // Number of bits per pixel.
    uint32_t Compression;     // Compression. Usually set to 0.
    uint32_t SizeImage;       // Size in bytes of the bitmap.
    uint32_t XPixelsPerMeter; // Horizontal pixels per meter.
    uint32_t YPixelsPerMeter; // Vertical pixels per meter.
    uint32_t ColorsUsed;      // Number of colors used.
    uint32_t ColorsImportant; // Number of "important" colors.
} __attribute__((packed)) BitmapHeader_t;

// Correct arrangement of palettes in bmp format: Blue Green Red Unused (mostly 0)
// The palettes are stored in backwards order in each palette entry
// http://en.wikipedia.org/wiki/BMP_file_format#Color_palette
typedef struct
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha; // Used to store 8-bit color equivalent at the moment
} __attribute__((packed)) BGRA_t;

typedef struct
{
    // cf. vbe3.pdf, page 55
    uint32_t blue        : 6;
    uint32_t blue_pad    : 2;
    uint32_t green       : 6;
    uint32_t green_pad   : 2;
    uint32_t red         : 6;
    uint32_t red_pad     : 2;
    uint8_t alignDword;
} __attribute__((packed)) BGRQuadPacked_t;


typedef struct
{
    BitmapHeader_t header;
    BGRA_t bmicolors[256];
} __attribute__((packed)) BMPInfo_t;


// Mode switch functions, will be later moved to a "video-manager" that switches between the modes
void switchToVideomode(uint16_t mode); // Switches to a VBE Mode
void switchToTextmode(); // Switches to the VGA Textmode

void vbe_readVIB();

void vbe_readMIB(uint16_t mode);
ModeInfoBlock_t* getCurrentMIB();

void setDisplayStart(uint16_t *xpos, uint16_t *ypos);
uint32_t getDisplayStart();

void setVideoMemory(); // Allocate the videomemory from the graphiccard

void printPalette();

// Set a Palette (old vga registers, need changed to the VBE Registers in vidswtch.asm)
void Set_DAC_C(uint8_t PaletteColorNumber, uint8_t  Red, uint8_t  Green, uint8_t  Blue);
void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue);

// Basic drawing functionality
void     vbe_setPixel(uint32_t x, uint32_t y, BGRA_t color); // Sets a single pixel on the screen
uint32_t vbe_getPixel(uint32_t x, uint32_t y);               // Returns the color of a single pixel on the screen

// Advanced and formatted drawing functionality
void vbe_drawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, BGRA_t color);             // Draws a line
void vbe_drawRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color);   // Draws a rectancle
void vbe_drawRectFilled(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color); // Draw a rectancle filled
void vbe_drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, BGRA_t color);                    // Draws a circle
void vbe_drawChar(char c);                                                                       // Draws a character using font.h
void vbe_drawString(const char* text, uint32_t xpos, uint32_t ypos);                             // Draws a string using vbe_drawChar
void vbe_drawBitmap(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap);                            // Draws a bitmap, loaded from data.asm via incbin
void vbe_drawBitmapTransparent(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap);                 // Draws a bitmap, WHITE is for transparent
void vbe_drawScaledBitmap(uint32_t newSizeX, uint32_t newSizeY, BMPInfo_t* bitmap);              // Scales a bitmap and draws it

// VBE Testing area
void vbe_bootscreen();

/// Unsuned functions
/*char ISValidBitmap(char *fname);
void showbitmap(char *infname,int xs,int ys);*/

#endif