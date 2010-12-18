/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util.h"
#include "task.h"
#include "paging.h"
#include "font.h"
#include "timer.h"
#include "gui.h"

ModeInfoBlock_t mib;
VgaInfoBlock_t  vgaIB;
BitmapHeader_t  bh;

BitmapHeader_t* bh_get;

uint8_t* SCREEN = (uint8_t*)0xE0000000; // video memory for supervga
position_t curPos = {0, 0};

uint8_t paletteBitsPerColor = 0;

// vm86
extern uintptr_t vidswtch_com_start;
extern uintptr_t vidswtch_com_end;

// bmp
extern BMPInfo_t bmp_start;
extern BMPInfo_t bmp_end;

uint16_t BGRAtoBGR16(BGRA_t bgr)
{
    uint16_t b = bgr.blue >> 3;
    uint16_t g = bgr.green >> 2;
    uint16_t r = bgr.red >> 3;

    return((r<<11) | (g<<5) | (b));
}

uint16_t BGRAtoBGR15(BGRA_t bgr)
{
    uint16_t b = bgr.blue >> 3;
    uint16_t g = bgr.green >> 3;
    uint16_t r = bgr.red >> 3;

    return((r<<10) | (g<<5) | (b));
}

void vbe_readVIB()
{
    *(char*)0x3400 = 'V';
    *(char*)0x3401 = 'B';
    *(char*)0x3402 = 'E';
    *(char*)0x3403 = '2';
    waitForTask(create_vm86_task(VM86_VGAINFOBLOCK));
    memcpy(&vgaIB, (void*)0x3400, sizeof(VgaInfoBlock_t));
}

void vbe_readMIB(uint16_t mode)
{
    *(uint16_t*)0x3600 = mode;
    waitForTask(create_vm86_task(VM86_MODEINFOBLOCK));
    memcpy(&mib, (void*)0x3600, sizeof(ModeInfoBlock_t));
}

ModeInfoBlock_t* getCurrentMIB()
{
    return(&mib);
}

void switchToVideomode(uint16_t mode)
{
    *(uint16_t*)0x3600 = 0xC1FF&(0xC000|mode); // Bits 9-13 may not be set, bits 14-15 should be set always
    waitForTask(create_vm86_task(VM86_SWITCH_TO_VIDEO));
    vbe_readMIB(mode);
    setVideoMemory();
    if(!(mode&BIT(15))) // We clear the Videoscreen manually, because the VGA is not reliable
        memset(SCREEN, 0, mib.XResolution*mib.YResolution*(mib.BitsPerPixel % 8 == 0 ? mib.BitsPerPixel/8 : mib.BitsPerPixel/8 + 1));
    paletteBitsPerColor = 0; // Has to be reinitializated
    videomode = VM_VBE;
}

void switchToTextmode()
{
    waitForTask(create_vm86_task(VM86_SWITCH_TO_TEXT));
    videomode = VM_TEXT;
    refreshUserScreen();
}

//void setDisplayStart(uint16_t *xpos, uint16_t *ypos)
//{
//    // memcpy(*(0x1800), xpos, sizeof(xpos));
//    // memcpy(*(0x1802), ypos, sizeof(ypos));
//    waitForTask(create_vm86_task(VM86_SETDISPLAYSTART));
//}

uint32_t getDisplayStart()
{
    waitForTask(create_vm86_task(VM86_GETDISPLAYSTART));
    return (*(uint32_t*)0x1300);
    // [0x1300]; First Displayed Scan Line
    // [0x1302]; First Displayed Pixel in Scan Line
}

void printPalette()
{
    if(mib.BitsPerPixel != 8) return;

    uint32_t xpos = 0;
    uint32_t ypos = 0;
    for(uint32_t j=0; j<256; j++)
    {
        BGRA_t temp = {0,0,0, j};
        vbe_drawRectFilled(xpos, ypos, xpos+5, ypos+5, temp);
        xpos +=5;
        if(xpos >= 255)
        {
            ypos += 5;
            xpos = 0;
        }
    }
}

// http://wiki.osdev.org/VGA_Hardware#VGA_Registers
void Set_DAC_C(uint8_t PaletteColorNumber, uint8_t Red, uint8_t Green, uint8_t Blue)
{
    if(paletteBitsPerColor == 0)
    {
        if(vgaIB.Capabilities[0] & BIT(0)) // VGA can handle palette with 8 bits per color -> Use it
        {
            waitForTask(create_vm86_task(VM86_SET8BITPALETTE));
            paletteBitsPerColor = 8;
        }
        else
        {
            printf("\n8-bit palette not supported");
            paletteBitsPerColor = 6;
        }
    }
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, PaletteColorNumber);
    if(paletteBitsPerColor == 8)
    {
        outportb(0x03C9, Red);
        outportb(0x03C9, Green);
        outportb(0x03C9, Blue);
    }
    else
    {
        outportb(0x03C9, Red   >> 2);
        outportb(0x03C9, Green >> 2);
        outportb(0x03C9, Blue  >> 2);
    }
}

void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, PaletteColorNumber);
    *Red   = inportb(0x03C9);
    *Green = inportb(0x03C9);
    *Blue  = inportb(0x03C9);
}

void setVideoMemory()
{
     uint32_t numberOfPages = vgaIB.TotalMemory * 0x10000 / PAGESIZE;
     SCREEN = paging_acquirePciMemory(mib.PhysBasePtr, numberOfPages);

     printf("\nSCREEN (phys): %X SCREEN (virt): %X\n",mib.PhysBasePtr, SCREEN);
     printf("\nVideo Ram %u MiB\n",vgaIB.TotalMemory/0x10);
}

void vbe_setPixel(uint32_t x, uint32_t y, BGRA_t color)
{
    switch(mib.BitsPerPixel)
    {
        case 15:
            ((uint16_t*)SCREEN)[y * mib.XResolution + x] = BGRAtoBGR15(color);
            break;
        case 16:
            ((uint16_t*)SCREEN)[y * mib.XResolution + x] = BGRAtoBGR16(color);
            break;
        case 24:
            (*(uint16_t*)&SCREEN[(y * mib.XResolution + x) * 3]) = *(uint16_t*)&color; // Performance Hack - copying 16 bits should be faster than copying 8 bits twice
            SCREEN[(y * mib.XResolution + x) * 3 + 2] = color.red;
            break;
        case 32:
            ((uint32_t*)SCREEN)[y * mib.XResolution + x] = *(uint32_t*)&color&0xFFFFFF; // Do not use alpha, we need it for 8-bit compatibility
            break;
        case 8: default:
            ((uint8_t*)SCREEN)[y * mib.XResolution + x] = color.alpha; // Workaround for 24 bit to 8 bit conversion
            break;
    }
}

uint32_t vbe_getPixel(uint32_t x, uint32_t y) // TODO: Fix it. It now ignores mib.BitsPerPixel
{
    return SCREEN[(y * mib.XResolution + x) * mib.BitsPerPixel/8];
}

void vbe_drawBitmap(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint16_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C(j, bitmap->bmicolors[j].red, bitmap->bmicolors[j].green, bitmap->bmicolors[j].blue);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;
    for(uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for(uint32_t x=bitmap->header.Width; x>0; x--)
        {
            BGRA_t temp = {bitmap->bmicolors[*pixel].blue, bitmap->bmicolors[*pixel].green, bitmap->bmicolors[*pixel].red, *pixel};
            vbe_setPixel(xpos+x, ypos+y, temp);
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void vbe_drawBitmapTransparent(uint32_t xpos, uint32_t ypos, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint16_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C(j, bitmap->bmicolors[j].red, bitmap->bmicolors[j].green, bitmap->bmicolors[j].blue);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;
    for(uint32_t y=0; y<bitmap->header.Height; y++)
    {
        for(uint32_t x=bitmap->header.Width; x>0; x--)
        {
            if(bitmap->bmicolors[*pixel].red != 255 && bitmap->bmicolors[*pixel].green != 255 && bitmap->bmicolors[*pixel].blue != 255) // 0xF6 == WHITE
            {
                BGRA_t temp = {bitmap->bmicolors[*pixel].blue, bitmap->bmicolors[*pixel].green, bitmap->bmicolors[*pixel].red, *pixel};
                vbe_setPixel(xpos+x, ypos+y, temp);
            }
            pixel -= bitmap->header.BitsPerPixel/8;
        }
    }
}

void vbe_drawScaledBitmap(uint32_t newSizeX, uint32_t newSizeY, BMPInfo_t* bitmap)
{
    if(mib.BitsPerPixel == 8)
    {
        for(uint16_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C(j, bitmap->bmicolors[j].red, bitmap->bmicolors[j].green, bitmap->bmicolors[j].blue);
        }
    }

    uint8_t* pixel = (uint8_t*)bitmap + sizeof(BMPInfo_t) + bitmap->header.Width * bitmap->header.Height;

    float FactorX = (float)newSizeX / (float)bitmap->header.Width;
    float FactorY = (float)newSizeY / (float)bitmap->header.Height;

    float OverflowX = 0, OverflowY = 0;
    for(uint32_t y = 0; y < newSizeY; y += (uint32_t)FactorY)
    {
        for(int32_t x = newSizeX-1; x >= 0; x -= (int32_t)FactorX)
        {
            pixel -= bitmap->header.BitsPerPixel/8;

            uint16_t rowsX = (uint16_t)FactorX + (OverflowX >= 1 ? 1:0);
            uint16_t rowsY = (uint16_t)FactorY + (OverflowY >= 1 ? 1:0);
            for(uint16_t i = 0; i < rowsX; i++)
            {
                for(uint16_t j = 0; j < rowsY; j++)
                {
                    if(x-i > 0)
                    {
                        BGRA_t temp = {bitmap->bmicolors[*pixel].blue, bitmap->bmicolors[*pixel].green, bitmap->bmicolors[*pixel].red, *pixel};
                        vbe_setPixel(x-i, y+j, temp);
                    }
                }
            }

            if(OverflowX >= 1)
            {
                OverflowX--;
                x--;
            }
            OverflowX += FactorX-(int)FactorX;
        }
        if(OverflowY >= 1)
        {
            OverflowY--;
            y++;
        }
        OverflowY += FactorY-(int)FactorY;
    }
}

// draws a line using Bresenham's line-drawing algorithm, which uses no multiplication or division.
void vbe_drawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, BGRA_t color)
{
    uint32_t dx=x2-x1;      // the horizontal distance of the line
    uint32_t dy=y2-y1;      // the vertical distance of the line
    uint32_t dxabs=fabs(dx);
    uint32_t dyabs=fabs(dy);
    uint32_t sdx=sgn(dx);
    uint32_t sdy=sgn(dy);
    uint32_t x=dyabs>>1;
    uint32_t y=dxabs>>1;
    uint32_t px=x1;
    uint32_t py=y1;

    vbe_setPixel(px, py, color);

    if (dxabs>=dyabs) // the line is more horizontal than vertical
    {
        for(uint32_t i=0;i<dxabs;i++)
        {
            y+=dyabs;
            if (y>=dxabs)
            {
                y-=dxabs;
                py+=sdy;
            }
            px+=sdx;
            vbe_setPixel(px,py,color);
        }
    }
    else // the line is more vertical than horizontal
    {
        for(uint32_t i=0;i<dyabs;i++)
        {
            x+=dxabs;
            if (x>=dyabs)
            {
                x-=dyabs;
                px+=sdx;
            }
            py+=sdy;
            vbe_setPixel(px,py,color);
        }
    }
}

// Draws a rectangle by drawing all lines by itself.
void vbe_drawRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color)
{
    // Sort edges
    if (top>bottom)
    {
        uint32_t temp=top;
        top=bottom;
        bottom=temp;
    }
    if (left>right)
    {
        uint32_t temp=left;
        left=right;
        right=temp;
    }

    for(uint32_t i=left; i<=right; i++)
    {
        vbe_setPixel(i, top, color);
        vbe_setPixel(i, bottom, color);
    }
    for(uint32_t i=top; i<=bottom; i++)
    {
        vbe_setPixel(left, i, color);
        vbe_setPixel(right, i, color);
    }
}

// Draws a rectangle by drawing all lines by itself. Filled
void vbe_drawRectFilled(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, BGRA_t color)
{
    for(uint32_t i = left; i < right; i++)
    {
        for(uint32_t j = top; j < bottom; j++)
        {
            vbe_setPixel(i, j, color);
        }
    }
}

// http://en.wikipedia.org/wiki/Circle#Cartesian_coordinates
void vbe_drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, BGRA_t color)
{
    for (uint32_t i=0; i<=2*radius; i++)
    {
        uint32_t x  = xm - radius + i;
        uint32_t y1 = ym + (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        uint32_t y2 = ym - (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        vbe_setPixel(x, y1, color);
        vbe_setPixel(x, y2, color);
    }
}

static void vgaDebug()
{
    textColor(0x0E);
    printf("\nVgaInfoBlock:\n");
    textColor(0x0F);
    printf("VESA-Signature:  %c%c%c%c\n", vgaIB.VESASignature[0], vgaIB.VESASignature[1], vgaIB.VESASignature[2], vgaIB.VESASignature[3]);
    printf("VESA-Version:    %u.%u\n",    vgaIB.VESAVersion>>8, vgaIB.VESAVersion&0xFF); // 01 02 ==> 1.2
    printf("Capabilities:    %y\n",       vgaIB.Capabilities[0]);
    printf("Video Memory:    %u MiB\n",   vgaIB.TotalMemory/0x10); // number of 64 KiB blocks of memory on the video card
    printf("Video Modes Ptr: %X\n",       vgaIB.VideoModes);

    textColor(0x0E);
    printf("\nVideo Modes:");
    textColor(0x0F);
    for (uint16_t i=0; i < 256; i++)
    {
        if(vgaIB.VideoModes[i] == 0xFFFF) break; // End of modelist
        vbe_readMIB(vgaIB.VideoModes[i]);
        if(!(mib.ModeAttributes & BIT(0)) || !(mib.ModeAttributes & BIT(7))) continue; // If bit 0 is not set, the mode is not supported due to the present hardware configuration, if bit 7 is not set, linear frame buffer is not supported
        printf("\n%u (%x) = %ux%ux%u", vgaIB.VideoModes[i], vgaIB.VideoModes[i], mib.XResolution, mib.YResolution, mib.BitsPerPixel);
        if(!(mib.ModeAttributes & BIT(4))) printf(" (textmode)");
    }
    printf("\n");
    textColor(0x0F);
}

static void modeDebug()
{
    textColor(0x0E);
    printf("\nModeInfoBlock:\n");
    textColor(0x0F);
    printf("ModeAttributes:        %x\n", mib.ModeAttributes);
    printf("WinAAttributes:        %u\n", mib.WinAAttributes);
    printf("WinBAttributes:        %u\n", mib.WinBAttributes);
    printf("WinGranularity:        %u\n", mib.WinGranularity);
    printf("WinSize:               %u\n", mib.WinSize);
    printf("WinASegment:           %u\n", mib.WinASegment);
    printf("WinBSegment:           %u\n", mib.WinBSegment);
    printf("WinFuncPtr:            %X\n", mib.WinFuncPtr);
    printf("BytesPerScanLine:      %u\n", mib.BytesPerScanLine);
    printf("XResolution:           %u\n", mib.XResolution);
    printf("YResolution:           %u\n", mib.YResolution);
    printf("XCharSize:             %u\n", mib.XCharSize);
    printf("YCharSize:             %u\n", mib.YCharSize);
    printf("NumberOfPlanes:        %u\n", mib.NumberOfPlanes);
    printf("BitsPerPixel:          %u\n", mib.BitsPerPixel);
    printf("NumberOfBanks:         %u\n", mib.NumberOfBanks);
    printf("MemoryModel:           %u\n", mib.MemoryModel);
    printf("BankSize:              %u\n", mib.BankSize);
    printf("NumberOfImagePages:    %u\n", mib.NumberOfImagePages);
    printf("RedMaskSize:           %u\n", mib.RedMaskSize);
    printf("RedFieldPosition:      %u\n", mib.RedFieldPosition);
    printf("GreenMaskSize:         %u\n", mib.GreenMaskSize);
    printf("GreenFieldPosition:    %u\n", mib.GreenFieldPosition);
    printf("BlueMaskSize:          %u\n", mib.BlueMaskSize);
    printf("BlueFieldPosition:     %u\n", mib.BlueFieldPosition);
    printf("RsvdMaskSize:          %u\n", mib.RsvdMaskSize);
    printf("RsvdFieldPosition:     %u\n", mib.RsvdFieldPosition);
    printf("OffScreenMemOffset:    %u\n", mib.OffScreenMemOffset);
    printf("OffScreenMemSize:      %u\n", mib.OffScreenMemSize);
    printf("DirectColorModeInfo:   %u\n", mib.DirectColorModeInfo);
    printf("Physical Memory Base:  %X\n", mib.PhysBasePtr);
}

/*char ISValidBitmap(char* fname)
{
    BMPINFO bmpinfo;
    FILE *fp;
    if((fp = fopen(fname,"rb+"))==0)
    {
        printf("Unable open the file %s",fname,"!!");
        return 0;
    }

    fread(&bmpinfo,sizeof(bmpinfo),1,fp);
    fclose(fp);
    if(!(bmpinfo.bmiheader.bftype[0]=='B' &&
    bmpinfo.bmiheader.bftype[1]=='M'))
    {
        printf("can't read the file: not a valid BMP file!");
        return 0;
    }

    if(!bmpinfo.bmiheader.bicompression==0)
    {
        printf("can't read the file: should not be a RLR encoded!!");
        return 0;
    }

    if(!bmpinfo.bmiheader.bibitcount==8)
    {
        printf("can't read the file: should be 8-bit per color format!!");
        return 0;
    }

    return 1;
}

void showbitmap(char* infname,int xs,int ys)
{
    BMPINFO bmpinfo;
    RGB pal[256];
    FILE *fpt;
    int i,j,w,h,c,bank;
    unsigned char byte[1056];
    long addr;
    unsigned int k;
    if((fpt=fopen(infname,"rb+"))==0)
    {
        printf("Error opening file ");
        getch();
        return 1;
    }

    fread(&bmpinfo,sizeof(bmpinfo),1,fpt);
    fseek(fpt,bmpinfo.bmiheader.bfoffbits,SEEK_SET);
    w = bmpinfo.bmiheader.biwidth;
    h = bmpinfo.bmiheader.biheight;

    for(i=0;i<=255;i++)
    {
        pal[i].red = bmpinfo.bmicolors[i].red/4;
        pal[i].green = bmpinfo.bmicolors[i].green/4;
        pal[i].blue = bmpinfo.bmicolors[i].blue/4;
    }

    vinitgraph(VGALOW);
    setwidth(1000);
    SetPalette(pal);

    for(i=0;i<h;i++)
    {
        fread(&byte[0],sizeof(unsigned char),w,fpt);

        for(j=0;j<w;j++)
        {
            c= (int ) byte[j];
            addr= (long) (ys+h-i)*bytesperline+xs+j;
            bank = (int ) (addr >>16);
            if(curbank!= bank)
            {
                curbank =bank;
                bank<<=bankshift;
                _BX=0;
                _DX=bank;
                bankswitch();
                _BX=1;
                bankswitch();
            }
            *(screenptr+(addr & 0xFFFF)) = (char ) c;
        }
    }

    fclose(fpt);
    getch();
    vclosegraph();
    return 0;
}

static void bitmapDebug() // TODO: make it bitmap-specific
{
    memcpy(&bh, bh_get, sizeof(BitmapHeader_t));

    textColor(0x0E);
    printf("\nBitmapHeader\n");
    textColor(0x0F);
    printf("Type:                  %u\n", bh.Type);
    printf("Reserved:              %u\n", bh.Reserved);
    printf("Offset:                %u\n", bh.Offset);
    printf("Header Size:           %u\n", bh.headerSize);
    printf("Width:                 %u\n", bh.Width);
    printf("Height:                %u\n", bh.Height);
    printf("Planes:                %u\n", bh.Planes);
    printf("Bits Per Pixel:        %u\n", bh.BitsPerPixel);
    printf("Compression:           %u\n", bh.Compression);
    printf("Image Size:            %u\n", bh.SizeImage);
    printf("X-Pixels Per Meter:    %u\n", bh.XPixelsPerMeter);
    printf("Y-Pixels Per Meter:    %u\n", bh.YPixelsPerMeter);
    printf("Colors Used:           %u\n", bh.ColorsUsed);
    printf("Colors Important:      %u\n", bh.ColorsImportant);
}*/

void vbe_drawChar(char font_char)
{
    uint8_t uc = AsciiToCP437((uint8_t)font_char); // no negative values

    switch (uc)
    {
        case 0x08: // backspace: move the cursor one space backwards and delete
            /*move_cursor_left();
            putch(' ');
            move_cursor_left();*/
            break;
        case 0x09: // tab: increment cursor.x (divisible by 8)
            curPos.x = (curPos.x + fontWidth*8) & ~(fontWidth*8 - 1);
            if(curPos.x+fontWidth >= mib.XResolution) vbe_drawChar('\n');
            break;
        case '\r': // r: cursor back to the margin
            curPos.x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment cursor.y
            curPos.x = 0;
            curPos.y += fontHeight;
            break;
        default:
            if(curPos.x+fontWidth >= mib.XResolution) vbe_drawChar('\n');
            if (uc != 0)
            {
                for(uint32_t y = 0; y < fontHeight; y++)
                {
                    for(uint32_t x = 0; x < fontWidth; x++)
                    {
                        BGRA_t temp = {font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048], font[(x + fontWidth*uc) + (fontHeight-y-1) * 2048]};
                        vbe_setPixel(curPos.x+x, curPos.y+y, temp);
                    }
                }
            }
            curPos.x += 8;
            break;
    }
}

void vbe_drawString(const char* text, uint32_t xpos, uint32_t ypos)
{
    curPos.x = xpos;
    curPos.y = ypos;
    for (; *text; vbe_drawChar(*text), ++text);
}
void vbe_clearScreen()
{
    BGRA_t color = { 0, 0, 0, 0 };
    for(int y = 0; y < mib.YResolution; y++)
    {
        for(int x = 0; x < mib.XResolution; x++)
        {
            vbe_setPixel(x, y, color);
        }
    }
}

void vbe_bootscreen()
{
    textColor(0x09);
    printf("       >>>>>   Press 's' to skip VBE-Test or any key to continue   <<<<<\n\n");
    textColor(0x0F);
    if(getch() == 's')
    {
        return;
    }

    memcpy((void*)0x100, &vidswtch_com_start, (uintptr_t)&vidswtch_com_end - (uintptr_t)&vidswtch_com_start);

    bh_get = (BitmapHeader_t*)&bmp_start;

    vbe_readVIB();
    vgaDebug();

    printf("\n");
    uint16_t modenumber = 0;

    while(modenumber == 0)
    {
        textColor(0x0E);
        printf("Type in the mode number: ");
        char temp[20];
        gets(temp);
        modenumber = atoi(temp);
        bool valid = false;
        for(uint16_t i = 0; i < 256; i++)
        {
            if(vgaIB.VideoModes[i] == modenumber)
            {
                vbe_readMIB(modenumber);
                if((mib.ModeAttributes & BIT(0)) && (mib.ModeAttributes & BIT(4)) && (mib.ModeAttributes & BIT(7))) // supported && videomode && LFB
                    valid = true;
                break;
            }
            if(vgaIB.VideoModes[i] == 0xFFFF) break; // End of modelist
        }
        if(!valid)
            modenumber = 0;
    }

    modeDebug();
    waitForKeyStroke();

    printf("\n");
    uint16_t whatToStart = 0;

    while(whatToStart == 0)
    {
        textColor(0x0E);
        printf("1. Start Graphical Tests\n");
        printf("2. Start GUI\n");
        printf("Type in the number: ");
        char num[3];
        gets(num);
        whatToStart = atoi(num);
    }

    switchToVideomode(modenumber);
    uint32_t displayStart = getDisplayStart();
    printf("\nFirst Displayed Scan Line: %u, First Displayed Pixel in Scan Line: %u", displayStart >> 16, displayStart & 0xFFFF);

    if(whatToStart == 1)
    {
        uint16_t radius = mib.YResolution/2;
        for(uint16_t i = 0; i < radius; i++)
        {
            BGRA_t color = {(i*128/radius)/2, (i*128/radius)*2, 128-(i*128/radius), (i*128/radius)};
            vbe_drawCircle(mib.XResolution/2, mib.YResolution/2, radius-i, color); // FPU
            sleepMilliSeconds(1);
        }

        BGRA_t bright_blue = {255, 75, 75, 0x09};
        vbe_drawLine(0, mib.YResolution/2, mib.XResolution, mib.YResolution/2, bright_blue); // FPU
        vbe_drawLine(0, mib.YResolution/2 + 1, mib.XResolution, mib.YResolution/2 + 1, bright_blue); // FPU
        vbe_drawLine(mib.XResolution/2, 0, mib.XResolution/2, mib.YResolution, bright_blue); // FPU
        vbe_drawLine(mib.XResolution/2+1, 0, mib.XResolution/2+1, mib.YResolution, bright_blue); // FPU
        vbe_drawCircle(mib.XResolution/2, mib.YResolution/2, mib.YResolution/2, bright_blue); // FPU
        vbe_drawCircle(mib.XResolution/2, mib.YResolution/2, mib.YResolution/2-1, bright_blue); // FPU
        waitForKeyStroke();

        vbe_drawBitmap(0, 0, &bmp_start);
        waitForKeyStroke();

        printPalette();

        vbe_drawString("PrettyOS started in March 2009.\nThis hobby OS tries to be a possible access for beginners in this area.", 0, 400);
        waitForKeyStroke();

        vbe_drawScaledBitmap(mib.XResolution, mib.YResolution, &bmp_start);
        waitForKeyStroke();

        vbe_clearScreen();
        waitForKeyStroke();
    }
    else if(whatToStart == 2)
    {
        StartGUI();
    }

    switchToTextmode();
}

/*
* Copyright (c) 2010 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
