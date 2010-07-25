#ifndef VBE_H
#define VBE_H

#include "os.h"

// http://www.petesqbsite.com/sections/tutorials/tuts/vbe3.pdf
// http://poli.cs.vsb.cz/misc/rbint/text/1005.html

#define VM86_SWITCH_TO_VIDEO ((void*)0x100)
#define VM86_VGAINFOBLOCK    ((void*)0x10B)
#define VM86_MODEINFOBLOCK   ((void*)0x119)
#define VM86_SETDACPALETTE   ((void*)0x134)
#define VM86_GETDACPALETTE   ((void*)0x141)
#define VM86_SETPALETTE      ((void*)0x14B)
#define VM86_GETPALETTE      ((void*)0x163)
#define VM86_SWITCH_TO_TEXT  ((void*)0x176)

// #define DIRECT_BANKING

// Transfer segment and offset to a linear 32-bit pointer
#define MAKE_LINEAR_POINTER(segment, offset)  ((uintptr_t)(((uint32_t) (segment) << 16) | (uint16_t) (offset)))

// #define video_memory 0xE0000000// 0xF2000000 // qemu


// SuperVGA information block
typedef struct
{
    uint8_t   VESASignature[4]     ;
    uint16_t  VESAVersion          __attribute__ ((packed));
    uintptr_t OEMStringPtr         __attribute__ ((packed));
    uint8_t   Capabilities[4]      ;
    uintptr_t VideoModePtr         __attribute__ ((packed));
    uint16_t  TotalMemory          __attribute__ ((packed));
    uint16_t  OemSoftwareRev       __attribute__ ((packed));
    uintptr_t OemVendorNamePtr     __attribute__ ((packed));
    uintptr_t OemProductNamePtr    __attribute__ ((packed));
    uintptr_t OemProductRevPtr     __attribute__ ((packed));
    uint8_t   Reserved[222]        ;
    uint8_t   OemData[256]         ;
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
    uintptr_t   PhysBasePtr;            // 32-bit physical memory address
    uint8_t    res2[212];              // Pad to 256 byte block size
} __attribute__((packed)) ModeInfoBlock_t;

typedef enum
{
    memPL   = 3,  // Planar memory model
    memPK   = 4,  // Packed pixel memory model
    memRGB  = 6,  // Direct color RGB memory model
    memYUV  = 7,  // Direct color YUV memory model
} memModels;

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB_t;

// bitmap the structure
typedef struct
{
  uint16_t width;
  uint16_t height;
  uint8_t  palette[256*3];
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
    uint8_t rgbreserved;
} __attribute__((packed)) RGBQuad_t; 

typedef struct
{
    // cf. vbe3.pdf, page 55
    uint8_t blue        : 6;
    uint8_t green       : 6;
    uint8_t red         : 6;
    uint8_t pad         : 6;
    uint8_t alignDword     ; // ???
} __attribute__((packed)) RGBQuadPacked_t;


typedef struct
{
    BitmapHeader_t *bmiheader;
    RGBQuad_t bmicolors[256];
} __attribute__((packed)) BMPInfo_t;

void setVgaInfoBlock(VgaInfoBlock_t* VIB);
void setModeInfoBlock(ModeInfoBlock_t* MIB);
ModeInfoBlock_t *getModeInfoBlock();

void switchToVGA();
void switchToVideomode();
void switchToTextmode();

void vgaDebug();
void printPalette(RGBQuadPacked_t* RGB);

void setPalette(RGBQuadPacked_t* RGB);
uint32_t getPalette();

void SetDAC(uint8_t DAC, uint8_t R, uint8_t G, uint8_t B);

void Write_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t *Palette);
void Set_DAC_C(uint8_t Color, uint8_t Red, uint8_t Green, uint8_t Blue);
void Read_DAC_C_Palette(uint8_t StartColor, uint8_t NumberOfColors, uint8_t* Palette);
// void Get_DAC_C(uint8_t Color, uint8_t &Red, uint8_t &Green, uint8_t &Blue);

void setDACPalette(RGBQuadPacked_t* RGB);

uint32_t getDACPalette();

uint32_t getVBEMode(void);
void setVBEMode(uint32_t mode);
void setBank(uint32_t bank);
void setVideoMemory();

void availableModes(void);

void setPixel(uint32_t x, uint32_t y, uint32_t color);

void line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color);
void drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color);
void bitmap(uint32_t xpos, uint32_t ypos);
char ISValidBitmap(char *fname);
void showbitmap(char *infname,int xs,int ys);
void bitmapDebug();

void draw_char(unsigned char* screen, unsigned char* where, unsigned char font_char);
void draw_string(unsigned char* screen, unsigned char* where, char* input);

#endif
