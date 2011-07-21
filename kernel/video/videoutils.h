#ifndef VIDEOUTILS_H
#define VIDEOUTILS_H

#include "videomanager.h"


// RenderBuffer
typedef struct
{
    videoDevice_t* device;
    void*          buffer;
    void*          buffer2;
} renderBuffer_t;


// BMP
typedef struct
{
    uint16_t Type;            // File type. Set to "BM".
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

typedef struct
{
    BitmapHeader_t header;
    BGRA_t bmicolors[256];
} __attribute__((packed)) BMPInfo_t;


// Pre-defined colors
extern const BGRA_t black;
extern const BGRA_t white;


// Color conversions. Inlined to improve performance
static inline uint16_t BGRAtoBGR16(BGRA_t bgr)
{
    return(((bgr.red & 0xF8)<<8) | ((bgr.green & 0xFC)<<3) | (bgr.blue >> 3));
}

static inline uint16_t BGRAtoBGR15(BGRA_t bgr)
{
    return(((bgr.red & 0xF8)<<7) | ((bgr.green & 0xF8)<<2) | (bgr.blue >> 3));
}

static inline uint16_t BGRAtoBGR8(BGRA_t bgr)
{
    return((bgr.red & 0xE0) | ((bgr.green & 0xE0)>>3) | (bgr.blue >> 6));
}

// Advanced and formatted drawing functionality
void video_drawLine(videoDevice_t* buffer, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, BGRA_t color);                 // Draws a line
void video_drawRect(videoDevice_t* buffer, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color);       // Draws a rectancle
void video_drawRectFilled(videoDevice_t* buffer, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color); // Draw a rectancle filled
void video_drawCartesianCircle(videoDevice_t* buffer, uint32_t xm, uint32_t ym, uint32_t radius, BGRA_t color);               // Draws a circle using cartesian coordinates algorithm
void video_drawChar(videoDevice_t* buffer, char c);                                                                           // Draws a character using font.h
void video_drawString(videoDevice_t* buffer, const char* text, uint32_t xpos, uint32_t ypos);                                 // Draws a string using video_drawChar
void video_drawBitmap(videoDevice_t* buffer, uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap);                                // Draws a bitmap
void video_drawBitmapTransparent(videoDevice_t* buffer, uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap, BGRA_t colorKey);    // Draws a bitmap, colorKey is handled as transparency
void video_drawScaledBitmap(videoDevice_t* buffer, uint32_t newSizeX, uint32_t newSizeY, BMPInfo_t* bitmap);                  // Scales a bitmap and draws it
void video_printPalette(videoDevice_t* device);                                                                               // Draws the palette used by device on device

renderBuffer_t* renderBuffer_create(uint16_t width, uint16_t height, uint8_t bitsPerPixel); // Creates a renderBuffer. It is recommended to use 32 bits per Pixel as color depth to increase performance
void renderBuffer_free(renderBuffer_t* buffer);
void renderBuffer_render(videoDevice_t* destination, renderBuffer_t* buffer, uint16_t x, uint16_t y); // Draw the content of the given renderBuffer to destination at the given position.

void   renderBuffer_setPixel(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color);
void   renderBuffer_fillPixels(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t color, size_t num);
void   renderBuffer_copyPixels(videoDevice_t* device, uint16_t x, uint16_t y, BGRA_t* colors, size_t num);
void   renderBuffer_clear(videoDevice_t* device, BGRA_t color);
BGRA_t renderBuffer_getPixel(videoDevice_t* device, uint16_t x, uint16_t y);
void   renderBuffer_flipScreen(videoDevice_t* device);


#endif
