/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util.h"
#include "task.h"
#include "video.h"
#include "paging.h"
#include "font.h"

ModeInfoBlock_t mib;
VgaInfoBlock_t  vgaIB;
BitmapHeader_t  bh;

BitmapHeader_t* bh_get;
BMPInfo_t* bmpinfo;

RGBQuadPacked_t* ScreenPal = (RGBQuadPacked_t*)0x1600;

uint8_t* SCREEN = (uint8_t*)0xE0000000; // video memory for supervga
CursorPosition_t curPos;


// vm86
extern uintptr_t vm86_com_start;
extern uintptr_t vm86_com_end;

// bmp
extern uintptr_t bmp_start;
extern uintptr_t bmp_end;

ModeInfoBlock_t* modeInfoBlock_user;


void setVgaInfoBlock(VgaInfoBlock_t* VIB)
{
    memcpy(&vgaIB, VIB, sizeof(VgaInfoBlock_t));
}

void setModeInfoBlock(ModeInfoBlock_t* MIB)
{
    memcpy(&mib, MIB, sizeof(ModeInfoBlock_t));
}

ModeInfoBlock_t* getModeInfoBlock()
{
    return &mib;
}

void switchToVGA()
{
    waitForTask(create_vm86_task(VM86_VGAINFOBLOCK));
}

void switchToVideomode(uintptr_t* MODE)
{
    waitForTask(create_vm86_task((void*)MODE));
}

void switchToTextmode()
{
    waitForTask(create_vm86_task(VM86_SWITCH_TO_TEXT));
    refreshUserScreen();
}

void setDisplayStart(uint16_t *xpos, uint16_t *ypos)
{
    // memcpy(*(0x1800), xpos, sizeof(xpos));
    // memcpy(*(0x1802), ypos, sizeof(ypos));
    waitForTask(create_vm86_task(VM86_SETDISPLAYSTART));
}

uint32_t getDisplayStart()
{
    waitForTask(create_vm86_task(VM86_GETDISPLAYSTART));
    return (((*(uint16_t*)(0x1300))<<16) | (*(uint16_t*)(0x1302)));
    // [0x1300]; First Displayed Scan Line
    // [0x1302]; First Displayed Pixel in Scan Line
}

void printPalette(RGBQuadPacked_t* RGB)
{
    for(uint32_t j=0; j<256; j++)
    {
        // transfer from bitmap palette to packed RAMDAC palette
        Set_DAC_C (j, bmpinfo->bmicolors[j].red   >> 2,
                      bmpinfo->bmicolors[j].green >> 2,
                      bmpinfo->bmicolors[j].blue  >> 2);
    }

    uint32_t xpos = 0;
    uint32_t ypos = 0;
    for(uint32_t j=0; j<256; j++)
    {
        for(uint32_t x = 0; x < 5; x++)
        {
            for(uint32_t y = 0; y < 5; y++)
            {
                // setPixel((x+xpos), (y+ypos), (ScreenPal[j].red) + (ScreenPal[j].green) + (ScreenPal[j].blue));
                setPixel((x+xpos), (y+ypos), j);
            }
        }
        xpos +=5;
        if(xpos >= 255)
        {
            ypos += 5;
            xpos = 0;
        }
    }
}

void setPalette(RGBQuadPacked_t* RGB)
{
    /*
    Format of VESA VBE palette entry:
    Offset    Size    Description
    00h      BYTE    red
    01h      BYTE    green
    02h      BYTE    blue
    03h      BYTE    alpha or alignment byte
    */
    ScreenPal = RGB;
    waitForTask(create_vm86_task(VM86_SETPALETTE));
}

uint32_t getPalette()
{
    waitForTask(create_vm86_task(VM86_GETPALETTE));

    printf("\nDEBUG: Palette output:\n");
    printf("RED:   %u \n",  *(uint8_t*)0x1400);
    printf("GREEN: %u \n",  *(uint8_t*)0x1401);
    printf("BLUE:  %u \n",  *(uint8_t*)0x1402);

    return 0;
}

// http://wiki.osdev.org/VGA_Hardware#VGA_Registers
void Set_DAC_C(uint8_t PaletteColorNumber, uint8_t Red, uint8_t Green, uint8_t Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, PaletteColorNumber);
    outportb(0x03C9, Red);
    outportb(0x03C9, Green);
    outportb(0x03C9, Blue);
}

void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, PaletteColorNumber);
    *Red   = inportb(0x03C9);
    *Green = inportb(0x03C9);
    *Blue  = inportb(0x03C9);
}

void Write_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t *Palette)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C8, StartColor); // first color to be input
    for(uint32_t i=0; i<NumOfColors*3; i++)
     {
        outportb(0x03C9, Palette[i]<<2);
     }
}

void Read_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t* Palette)
{
    outportb(0x03C6, 0xFF);
    outportb(0x03C7, StartColor); // first color to be read
    for(uint32_t i=0; i<NumOfColors*3; i++)
    {
        Palette[i]=inportb(0x03C9);
    }
}

void setDACPalette(RGBQuadPacked_t* RGB)
{
    waitForTask(create_vm86_task(VM86_SETDACPALETTE));
}

uint32_t getDACPalette()
{
    waitForTask(create_vm86_task(VM86_GETDACPALETTE));
    return 0;
}

inline void setPixel(uint32_t x, uint32_t y, uint32_t color)
{
    // unsigned uint8_t* pixel = vram + y*pitch + x*pixelwidth;
    SCREEN[y * mib.XResolution + x * mib.BitsPerPixel/8] = color;
}

uint32_t getPixel(uint32_t x, uint32_t y)
{
    uint32_t color;
    color = SCREEN[y * mib.XResolution + x * mib.BitsPerPixel/8];
    return color;
}

void setVideoMemory()
{
     uint32_t numberOfPages = vgaIB.TotalMemory * 0x10000 / PAGESIZE;
     SCREEN = (uint8_t*)paging_acquire_pcimem(mib.PhysBasePtr, numberOfPages);

     printf("\nSCREEN (phys): %X SCREEN (virt): %X\n",mib.PhysBasePtr, SCREEN);
     printf("\nVideo Ram %u MiB\n",vgaIB.TotalMemory/0x10);
}

void bitmap(uint32_t xpos, uint32_t ypos, void* bitmapMemStart)
{
    uintptr_t bitmap_start = (uintptr_t)bitmapMemStart + sizeof(BMPInfo_t);
    uintptr_t bitmap_end = bitmap_start + ((BitmapHeader_t*)bitmapMemStart)->Width * ((BitmapHeader_t*)bitmapMemStart)->Height;

    if(mib.BitsPerPixel == 8)
    {
        bmpinfo = (BMPInfo_t*)bitmapMemStart;
        for(uint32_t j=0; j<256; j++)
        {
            // transfer from bitmap palette to packed RAMDAC palette
            Set_DAC_C (j, bmpinfo->bmicolors[j].red   >> 2,
                          bmpinfo->bmicolors[j].green >> 2,
                          bmpinfo->bmicolors[j].blue  >> 2);
        }
    }

    uint8_t* i = (uint8_t*)bitmap_end;
     for(uint32_t y=0; y<((BitmapHeader_t*)bitmapMemStart)->Height; y++)
     {
         for(uint32_t x=((BitmapHeader_t*)bitmapMemStart)->Width; x>0; x--)
         {
             SCREEN[ (xpos+x) + (ypos+y) * mib.XResolution * mib.BitsPerPixel/8 ] = *i;
             i -= (mib.BitsPerPixel/8);
         }
     }
}

void scaleBitmap(uint32_t newSizeX, uint32_t newSizeY, void* bitmapMemStart)
{
    uintptr_t bitmap_start = (uintptr_t)bitmapMemStart + sizeof(BMPInfo_t);
    uintptr_t bitmap_end = bitmap_start + ((BitmapHeader_t*)bitmapMemStart)->Width * ((BitmapHeader_t*)bitmapMemStart)->Height;
    uint8_t*  pixel = (uint8_t*)bitmap_end;

    uint8_t bytesPerPixel = mib.BitsPerPixel/8;

    float FactorX = (float)newSizeX / (float)((BitmapHeader_t*)bitmapMemStart)->Width;
    float FactorY = (float)newSizeY / (float)((BitmapHeader_t*)bitmapMemStart)->Height;

    float OverflowX = 0, OverflowY = 0;
    for(uint32_t y = 0; y < newSizeY; y += (uint32_t)FactorY)
    {
        for(int32_t x = newSizeX-1; x >= 0; x -= (int32_t)FactorX)
        {
            pixel -= bytesPerPixel; // We go through the image backwards

            uint16_t rowsX = (uint16_t)FactorX + (OverflowX >= 1 ? 1:0);
            uint16_t rowsY = (uint16_t)FactorY + (OverflowY >= 1 ? 1:0);
            for(uint16_t i = 0; i < rowsX; i++)
            {
                for(uint16_t j = 0; j < rowsY; j++)
                {
                    if(x-i > 0)
                        setPixel(x-i, y+j, *pixel);
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
void line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
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

    SCREEN[(py<<8)+(py<<6)+px]=color;

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
            setPixel(px,py,color);
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
            setPixel(px,py,color);
        }
    }
}

// Draws a rectangle by drawing all lines by itself.
void rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t color)
{
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

    uint32_t top_offset = (top<<8)+(top<<6);
    uint32_t bottom_offset = (bottom<<8)+(bottom<<6);

    for(uint32_t i=left; i<=right; i++)
    {
        SCREEN[top_offset+i]=color;
        SCREEN[bottom_offset+i]=color;
    }
    for(uint32_t i=top_offset; i<=bottom_offset; i+=mib.XResolution) //SCREEN_WIDTH
    {
        SCREEN[left+i]=color;
        SCREEN[right+i]=color;
    }
}

// http://en.wikipedia.org/wiki/Circle#Cartesian_coordinates
void drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color)
{    
    for (uint32_t i=0; i<=2*radius; i++)
    {
        uint32_t x  = xm - radius + i;
        uint32_t y1 = ym + (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        uint32_t y2 = ym - (uint32_t)sqrt(radius*radius - (x-xm)*(x-xm));
        setPixel(x, y1, 9);
        setPixel(x, y2, 9);
    }
}

void vgaDebug()
{
    printf("\nDEBUG print: VgaInfoBlock, size: %x\n\n", sizeof(VgaInfoBlock_t));
    printf("VESA-Signature:         %s\n",     vgaIB.VESASignature);
    printf("VESA-Version:           %u.%u\n", (vgaIB.VESAVersion&0xFF00)>>8,vgaIB.VESAVersion&0xFF); // 01 02 ==> 1.2
    printf("Capabilities:           %X\n",     vgaIB.Capabilities);
    printf("Video Memory (MiB):     %u\n",     vgaIB.TotalMemory/0x10); // number of 64 KiB blocks of memory on the video card
    printf("OEM-String (address):   %X\n",     vgaIB.OEMStringPtr);
    printf("Video Modes Ptr:        %X\n",     vgaIB.VideoModePtr);

    // TEST Video Ram Paging
    printf("\nSCREEN (phys): %X SCREEN (virt): %X\n", mib.PhysBasePtr, SCREEN);

    // TEST
    //printf("OemVendorNamePtr:       %X\n",     vgaIB.OemVendorNamePtr);
    //printf("OemProductNamePtr:      %X\n",     vgaIB.OemProductNamePtr);
    //printf("OemProductRevPtr:       %X\n",     vgaIB.OemProductRevPtr);

    textColor(0x0E);
    printf("\nVideo Modes:\n\n");
    for (uint8_t i=0; i<16; i++)
    {
        printf("%x ", vgaIB.VideoModePtr[i]);
        switch(vgaIB.VideoModePtr[i])
        {
            case 0x0100:
                printf("= 640x400x256\n");
                break;
            case 0x0101:
                printf("= 640x480x256\n");
                break;
            case 0x0102:
                printf("= 800x600x16\n");
                break;
            case 0x0103:
                printf("= 800x600x256\n");
                break;
            case 0x0104:
                printf("= 1024x768x16\n");
                break;
            case 0x0105:
                printf("= 1024x768x256\n");
                break;
            case 0x0106:
                printf("= 1280x1024x16\n");
                break;
            case 0x0107:
                printf("= 1280x1024x256\n");
                break;
            case 0x0108:
                printf("= 80x60 text\n");
                break;
            case 0x0109:
                printf("= 132x25 text\n");
                break;
            case 0x010A:
                printf("= 132x43 text\n");
                break;
            case 0x010B:
                printf("= 132x50 text\n");
                break;
            case 0x010C:
                printf("= 132x60 text\n");
                break;
            case 0x010D:
                printf("= 320x200x32K\n");
                break;
            case 0x010E:
                printf("= 320x200x64K\n");
                break;
            case 0x010F:
                printf("= 320x200x16M\n");
                break;
            case 0x0110:
                printf("= 640x480x32K\n");
                break;
            case 0x0111:
                printf("= 640x480x64K\n");
                break;
            case 0x0112:
                printf("= 640x480x16M\n");
                break;
            case 0x0113:
                printf("= 800x600x32K\n");
                break;
            case 0x0114:
                printf("= 800x600x64K\n");
                break;
            case 0x0115:
                printf("= 800x600x16M\n");
                break;
            case 0x0116:
                printf("= 1024x768x32K\n");
                break;
            case 0x0117:
                printf("= 1024x768x64K\n");
                break;
            case 0x0118:
                printf("= 1024x768x16M\n");
                break;
            case 0x0119:
                printf("= 1280x1024x32K\n");
                break;
            case 0x011A:
                printf("= 1280x1024x64K\n");
                break;
            case 0x011B:
                printf("= 1280x1024x16M\n");
                break;
            // VBE 2.0 modes
            case 0x0120:
                printf("= 1600x1200x256\n");
                break;
            case 0x0121:
                printf("= 1600x1200x32K\n");
                break;
            case 0x0122:
                printf("= 1600x1200x64K\n");
                break;
            case 0xFFFF:
                printf("= end of modelist\n");
                break;
            default:
                printf("\n");
                break;
        }
    }
    printf("\n");
    textColor(0x0F);

    waitForKeyStroke();

    printf("\nDEBUG print: ModeInfoBlock, size: %x\n\n", sizeof(ModeInfoBlock_t));
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

char ISValidBitmap(char* fname)
{
/*
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
*/
    return 1;
}

void showbitmap(char* infname,int xs,int ys)
{
/*
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
    */
}

void bitmapDebug() // TODO: make it bitmap-specific
{
    memcpy(&bh, bh_get, sizeof(BitmapHeader_t));

    printf("\nDEBUG print: BitmapHeader, size: %x\n\n", sizeof(BitmapHeader_t));
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
}

void drawChar(char font_char)
{
    uint8_t uc = AsciiToCP437((uint8_t)font_char); // no negative values
    uint32_t xFont = 8, yFont = 16; // This info should be achievable from the font.h

    // mib.XResolution;
    // mib.YResolution;

    switch (uc)
    {
        case 0x08: // backspace: move the cursor one space backwards and delete
            /*move_cursor_left();
            putch(' ');
            move_cursor_left();*/
            break;
        case 0x09: // tab: increment cursor.x (divisible by 8)
            curPos.x = (curPos.x + xFont*8) & ~(xFont*8 - 1);
            /*if (currentConsole->cursor.x>=COLUMNS)
            {
                ++currentConsole->cursor.y;
                currentConsole->cursor.x=0;
                scroll();
            }*/
            break;
        case '\r': // r: cursor back to the margin
            curPos.x = 0;
            break;
        case '\n': // newline: like 'cr': cursor to the margin and increment cursor.y
            curPos.x = 0;
            curPos.y += yFont;
            break;
        default:
            if (uc != 0)
            {
                for(uint32_t y=0; y < yFont; y++)
                {
                    for(uint32_t x=0; x<xFont; x++)
                    {
                        //SCREEN[ (xpos+x) + (ypos+y) * mib.XResolution * mib.BitsPerPixel/8 ] = font[(x + xFont*uc) + (yFont-y-1) * 2048];
                        SCREEN[ (curPos.x+x) + (curPos.y+y) * mib.XResolution * mib.BitsPerPixel/8 ] = font[(x + xFont*uc) + (yFont-y-1) * 2048];
                    }
                }
            }
            curPos.x += 8;
            break;
    }
}

void drawString(const char* text, uint32_t xpos, uint32_t ypos)
{
    curPos.x = xpos;
    curPos.y = ypos;
    for (; *text; drawChar(*text), ++text);
}

void VBE_bootscreen()
{
    textColor(0x09);
    printf("       >>>>>   Press 's' to skip VBE-Test or any key to continue   <<<<<\n\n");
    textColor(0x0F);
    if(getch() == 's')
    {
        return;
    }

    memcpy((void*)0x100, &vm86_com_start, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start);

    #ifdef _VM_DIAGNOSIS_
    printf("vm86 binary code at 0x100: ");
    memshow((void*)0x100, (uintptr_t)&vm86_com_end - (uintptr_t)&vm86_com_start);
    printf("\n");
    #endif

    bh_get = (BitmapHeader_t*)&bmp_start;

    switchToVGA(); // change it to a better matching function name...

    setVgaInfoBlock((VgaInfoBlock_t*)0x3400);

    textColor(0x0E);
    printf("Select Resolution (given by its number):\n");
    textColor(0x0F);
    printf("1. 640x480x256\n");
    printf("2. 800x600x256\n");
    printf("3. 1024x768x256 (Default mode WORKING!)\n");
    uint8_t selectMode = getch();
    switch(selectMode)
    {
        case '1':
            waitForTask(create_vm86_task(VM86_MODEINFOBLOCK_640_480_256));
            break;
        case '2':
            waitForTask(create_vm86_task(VM86_MODEINFOBLOCK_800_600_256));
            break;
        case '3': default:
            waitForTask(create_vm86_task(VM86_MODEINFOBLOCK_1024_768_256));
            break;
    }

    setModeInfoBlock((ModeInfoBlock_t*)0x3600);
    modeInfoBlock_user = getModeInfoBlock();

    vgaDebug();

    setVideoMemory();
    waitForKeyStroke();

    switch(selectMode)
    {
        case '1':
            switchToVideomode(VM86_SWITCH_TO_VIDEO_640_480_256);
            break;
        case '2':
            switchToVideomode(VM86_SWITCH_TO_VIDEO_800_600_256);
            break;
        case '3': default:
            switchToVideomode(VM86_SWITCH_TO_VIDEO_1024_768_256);
            break;
    }

    line(0, modeInfoBlock_user->YResolution/2 + 1, modeInfoBlock_user->XResolution, modeInfoBlock_user->YResolution/2 + 1, 0x09); // FPU
    line(modeInfoBlock_user->XResolution/2, 0, modeInfoBlock_user->XResolution/2, modeInfoBlock_user->YResolution, 0x09); // FPU
    waitForKeyStroke();

    drawCircle(modeInfoBlock_user->XResolution/2, modeInfoBlock_user->YResolution/2, modeInfoBlock_user->YResolution/2, 0x01); // FPU
    waitForKeyStroke();

    bitmap(320,0,&bmp_start);
    waitForKeyStroke();

    printPalette(ScreenPal);
    waitForKeyStroke();

    drawString("PrettyOS started in March 2009.\nThis hobby OS tries to be a possible access for beginners in this area.", 0, 400);
    waitForKeyStroke();

    scaleBitmap(modeInfoBlock_user->XResolution, modeInfoBlock_user->YResolution, &bmp_start); // testing
    waitForKeyStroke();

    uint32_t displayStart = getDisplayStart();
    uint32_t color = getPixel(1,1);

    switchToTextmode();

    vgaDebug();
    printf("\nFirst Displayed Scan Line: %u, First Displayed Pixel in Scan Line: %u", (displayStart & 0xFFFF0000)>>16, displayStart & 0xFFFF);
    printf("\ngetPixel = %u\n", color);

    waitForKeyStroke();
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
