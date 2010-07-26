/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#include "vbe.h"
#include "util.h"
#include "task.h"
#include "video.h"
#include "paging.h"

ModeInfoBlock_t modeInfoBlock;
ModeInfoBlock_t* mib = &modeInfoBlock;

VgaInfoBlock_t vgaInfoBlock;
VgaInfoBlock_t* vgaIB = &vgaInfoBlock;

BitmapHeader_t bitmapHeader;
BitmapHeader_t* bh = &bitmapHeader;

VgaInfoBlock_t*  pVga;
ModeInfoBlock_t* mob;

BitmapHeader_t*  bh_get;
BMPInfo_t* bmpinfo;

RGBQuadPacked_t* ScreenPal = (RGBQuadPacked_t*)0x1600;

uint8_t* SCREEN = (uint8_t*)0xE0000000; // video memory for supervga

int xres,yres;    // Resolution of video mode used
int bytesperline; // Logical CRT scanline length
int curBank;      // Current read/write bank
int bankShift;    // Bank granularity adjust factor
int oldMode;      // Old video mode number
//void    _far (*bankSwitch)(void); // Direct bank switching function

void setVgaInfoBlock(VgaInfoBlock_t* VIB)
{
    memcpy(vgaIB, VIB, sizeof(VgaInfoBlock_t));
}

void setModeInfoBlock(ModeInfoBlock_t* MIB)
{
    memcpy(mib, MIB, sizeof(ModeInfoBlock_t));
}

ModeInfoBlock_t *getModeInfoBlock()
{
    return mib;
}

void switchToVGA()
{
    waitForTask(create_vm86_task(VM86_VGAINFOBLOCK));
}

void switchToVideomode()
{
    waitForTask(create_vm86_task(VM86_SWITCH_TO_VIDEO));
}

void switchToTextmode()
{
    waitForTask(create_vm86_task(VM86_SWITCH_TO_TEXT));
    refreshUserScreen();
}

void printPalette(RGBQuadPacked_t* RGB)
{
    uint32_t xpos = 0;
    uint32_t ypos = 0;

    for(uint32_t j=0; j<256; j++)
    {
        // transfer from bitmap palette to packed RAMDAC palette
        Set_DAC_C (j, bmpinfo->bmicolors[j].red   >> 2,
                      bmpinfo->bmicolors[j].green >> 2,
                      bmpinfo->bmicolors[j].blue  >> 2);
    }

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
    outportb(0x03C6,0xff);
    outportb(0x03C8,PaletteColorNumber);
    outportb(0x03C9,Red);
    outportb(0x03C9,Green);
    outportb(0x03C9,Blue);
}

void Get_DAC_C(uint8_t PaletteColorNumber, uint8_t* Red, uint8_t* Green, uint8_t* Blue)
{
    outportb(0x03c6,0xff);
    outportb(0x03c7,PaletteColorNumber);
    *Red   = inportb(0x03c9);
    *Green = inportb(0x03c9);
    *Blue  = inportb(0x03c9);
}

void Write_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t *Palette)
{
    outportb(0x03C6,0xff);
    outportb(0x03C8,StartColor);    // first color to be input
    for(short i=0; i<NumOfColors*3; i++ )
     {
        outportb(0x03C9,Palette[i]<<2);
     }
}

void Read_DAC_C_Palette(uint8_t StartColor, uint8_t NumOfColors, uint8_t* Palette)
{
    outportb(0x03C6,0xff);
    outportb(0x03C7,StartColor);    // first color to be read
    for(short i=0; i<NumOfColors*3; i++ )
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


void setPixel(uint32_t x, uint32_t y, uint32_t color)
{
    // long addr = (long)y * bytesperline + x;
    // setBank(addr >> 16);
    // *(screenPtr + (addr & 0xFFFF)) = color;

    // unsigned uint8_t* pixel = vram + y*pitch + x*pixelwidth;
    SCREEN[y * mib->XResolution + x * mib->BitsPerPixel/8] = color;

}



void setVideoMemory()
{
     // size_of_video_ram
     SCREEN = (uint8_t*)paging_acquire_pcimem(mib->PhysBasePtr);
     for (uint32_t i=mib->PhysBasePtr; i<(mib->PhysBasePtr+vgaIB->TotalMemory*0x10000);i=i+0x1000)
     {
         printf("\t: %X",paging_acquire_pcimem(i));
     }
       printf("\nSCREEN (phys): %X SCREEN (virt): %X\n",mib->PhysBasePtr, SCREEN);
     printf("\nVideo Ram %u MiB\n",vgaIB->TotalMemory/0x10);

     // add the size of color (palette) to the screen
     if(mib->BitsPerPixel == 8)
     {
        // SCREEN += 256; // only video mode 101h ??
     }
}

void bitmap(uint32_t xpos, uint32_t ypos, void* bitmapMemStart)
{
    uintptr_t bitmap_start = (uintptr_t)bitmapMemStart + sizeof(BMPInfo_t);
     uintptr_t bitmap_end = bitmap_start + ((BitmapHeader_t*)bitmapMemStart)->Width * ((BitmapHeader_t*)bitmapMemStart)->Height;

     if(mib->BitsPerPixel == 8)
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
             SCREEN[ (xpos+x) + (ypos+y) * mib->XResolution * mib->BitsPerPixel/8 ] = *i;
             i -= (mib->BitsPerPixel/8);
         }
     }
}



// draws a line using Bresenham's line-drawing algorithm, which uses no multiplication or division. (DON`T USE IT, IT CRASH!)
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
    uint32_t top_offset,bottom_offset,temp; //word

    if (top>bottom)
    {
        temp=top;
        top=bottom;
        bottom=temp;
    }
    if (left>right)
    {
        temp=left;
        left=right;
        right=temp;
    }

    top_offset=(top<<8)+(top<<6);
    bottom_offset=(bottom<<8)+(bottom<<6);

    for(uint32_t i=left;i<=right;i++)
    {
        SCREEN[top_offset+i]=color;
        SCREEN[bottom_offset+i]=color;
    }
    for(uint32_t i=top_offset;i<=bottom_offset;i+=mib->XResolution) //SCREEN_WIDTH
    {
        SCREEN[left+i]=color;
        SCREEN[right+i]=color;
    }
}

void drawCircle(uint32_t xm, uint32_t ym, uint32_t radius, uint32_t color)
{
    // http://en.wikipedia.org/wiki/Circle#Cartesian_coordinates
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
    printf("VESA-Signature:         %s\n",     vgaIB->VESASignature);
    printf("VESA-Version:           %u.%u\n", (vgaIB->VESAVersion&0xFF00)>>8,vgaIB->VESAVersion&0xFF); // 01 02 ==> 1.2
    printf("Capabilities:           %X\n",     vgaIB->Capabilities);
    printf("Video Memory (MiB):     %u\n",     vgaIB->TotalMemory/0x10); // number of 64 KiB blocks of memory on the video card
    printf("OEM-String (address):   %X\n",     vgaIB->OEMStringPtr);
    printf("Video Modes Ptr:        %X\n",     vgaIB->VideoModePtr);

    // TEST Video Ram Paging
    printf("\nSCREEN (phys): %X SCREEN (virt): %X\n",mib->PhysBasePtr, SCREEN);

    // TEST
    // printf("OemVendorNamePtr:       %X\n",     vgaIB->OemVendorNamePtr);
    // printf("OemProductNamePtr:      %X\n",     vgaIB->OemProductNamePtr);
    // printf("OemProductRevPtr:       %X\n",     vgaIB->OemProductRevPtr);

    textColor(0x0E);
    printf("\nVideo Modes:\n\n");
    for (uint8_t i=0; i<16; i++)
    {
        printf("%x ", *((uint16_t*)(vgaIB->VideoModePtr)+i));
        switch(*((uint16_t*)(vgaIB->VideoModePtr)+i))
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
    printf("WinAAttributes:        %u\n", mib->WinAAttributes);
    printf("WinBAttributes:        %u\n", mib->WinBAttributes);
    printf("WinGranularity:        %u\n", mib->WinGranularity);
    printf("WinSize:               %u\n", mib->WinSize);
    printf("WinASegment:           %u\n", mib->WinASegment);
    printf("WinBSegment:           %u\n", mib->WinBSegment);
    printf("WinFuncPtr:            %X\n", mib->WinFuncPtr);
    printf("BytesPerScanLine:      %u\n", mib->BytesPerScanLine);
    printf("XResolution:           %u\n", mib->XResolution);
    printf("YResolution:           %u\n", mib->YResolution);
    printf("XCharSize:             %u\n", mib->XCharSize);
    printf("YCharSize:             %u\n", mib->YCharSize);
    printf("NumberOfPlanes:        %u\n", mib->NumberOfPlanes);
    printf("BitsPerPixel:          %u\n", mib->BitsPerPixel);
    printf("NumberOfBanks:         %u\n", mib->NumberOfBanks);
    printf("MemoryModel:           %u\n", mib->MemoryModel);
    printf("BankSize:              %u\n", mib->BankSize);
    printf("NumberOfImagePages:    %u\n", mib->NumberOfImagePages);
    printf("RedMaskSize:           %u\n", mib->RedMaskSize);
    printf("RedFieldPosition:      %u\n", mib->RedFieldPosition);
    printf("GreenMaskSize:         %u\n", mib->GreenMaskSize);
    printf("GreenFieldPosition:    %u\n", mib->GreenFieldPosition);
    printf("BlueMaskSize:          %u\n", mib->BlueMaskSize);
    printf("BlueFieldPosition:     %u\n", mib->BlueFieldPosition);
    printf("RsvdMaskSize:          %u\n", mib->RsvdMaskSize);
    printf("RsvdFieldPosition:     %u\n", mib->RsvdFieldPosition);
    printf("OffScreenMemOffset:    %u\n", mib->OffScreenMemOffset);
    printf("OffScreenMemSize:      %u\n", mib->OffScreenMemSize);
    printf("DirectColorModeInfo:   %u\n", mib->DirectColorModeInfo);
    printf("Physical Memory Base:  %X\n", mib->PhysBasePtr);
}

char ISValidBitmap(char *fname)
{
/*
    BMPINFO bmpinfo;
    FILE *fp;
    if((fp = fopen(fname,"rb+"))==NULL)
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

void showbitmap(char *infname,int xs,int ys)
{
/*
    BMPINFO bmpinfo;
    RGB pal[256];
    FILE *fpt;
    int i,j,w,h,c,bank;
    unsigned char byte[1056];
    long addr;
    unsigned int k;
    if((fpt=fopen(infname,"rb+"))==NULL)
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

void bitmapDebug()
{
    memcpy((void*)bh, (void*)bh_get, sizeof(BitmapHeader_t));

    printf("\nDEBUG print: BitmapHeader, size: %x\n\n", sizeof(BitmapHeader_t));
    printf("Type:                  %u\n", bh->Type);
    printf("Reserved:              %u\n", bh->Reserved);
    printf("Offset:                %u\n", bh->Offset);
    printf("Header Size:           %u\n", bh->headerSize);
    printf("Width:                 %u\n", bh->Width);
    printf("Height:                %u\n", bh->Height);
    printf("Planes:                %u\n", bh->Planes);
    printf("Bits Per Pixel:        %u\n", bh->BitsPerPixel);
    printf("Compression:           %u\n", bh->Compression);
    printf("Image Size:            %u\n", bh->SizeImage);
    printf("X-Pixels Per Meter:    %u\n", bh->XPixelsPerMeter);
    printf("Y-Pixels Per Meter:    %u\n", bh->YPixelsPerMeter);
    printf("Colors Used:           %u\n", bh->ColorsUsed);
    printf("Colors Important:      %u\n", bh->ColorsImportant);
}


void draw_char(unsigned char* screen, unsigned char* where, unsigned char font_char)
{
}

void draw_string(unsigned char* screen, unsigned char* where, char* input)
{
/*
; ------ Fill screen buffer with a color.
        mov    eax, 0xFFFFFFFF ; white
        mov    edi, buffer
        mov    ecx, 640*480/2
        rep    stosd

; ------ Print message using the font.
        mov    edi, buffer
        xor    ecx, ecx
    .m:    mov    al, [msg1+ecx]
        call    next_glyph
        inc    ecx
        cmp    ecx, len1
        jl    .m

; ------ Copy screen buffer into video memory.
        mov    esi, buffer
        mov    edi, 0xE0000000
        mov    ecx, 640*480/2
        rep    movsd

; ------ Hang computer.
;        jmp    $

; -------------------------------------------------------------------------
; SUBROUTINES
; -------------------------------------------------------------------------

next_2_pixels:
; Converts low 2 bits in EAX to 2 pixels on screen
; PASS eax=bits from font, edi=pointer into screen buffer
        mov    ebx, eax
        and    ebx, 0x03 ; leave only bits 0..1 (value 0..3)
        mov    ebx, [colors+ebx*4]
        shr    eax, 2
        mov    [edi], ebx
        lea    edi, [edi+4]
        ret

next_scanline:
; Converts low 8 bits in EAX to 8 pixels on screen
; PASS eax=bits from font, edi=pointer into screen buffer
        call    next_2_pixels
        call    next_2_pixels
        call    next_2_pixels
        call    next_2_pixels
        add    edi, 640*2-16
        ret

next_4_scanlines:
; Converts 32 bits in EAX to 4 rows of 8 pixels on screen
; PASS esi=pointer into font, edi=pointer into screen buffer
        mov    eax, [esi]   ; grab 4 bytes from font
        call    next_scanline
        call    next_scanline
        call    next_scanline
        call    next_scanline
        lea    esi, [esi+4] ; move to next 4 bytes in font
        ret

next_glyph:
; Prints character whose ASCII value is passed in AL (EAX)
; PASS eax=character, edi=pointer into screen buffer
        push    edi
        and    eax, 0xFF
        shl    eax, 4    ; multiply by 16 (# bytes in font per char)
        add    eax, font ; create offset into font
        mov    esi, eax
        call    next_4_scanlines
        call    next_4_scanlines
        call    next_4_scanlines
        call    next_4_scanlines
        pop    edi
        add    edi, 16   ; width in bytes of one character
        ret

; -------------------------------------------------------------------------
black equ 0x0000
white equ 0xFFFF

; We take pixels two at a time and print both at once. Bit 0 becomes the
; first pixel printed; bit 1, the second. If the two bits are 01 (bit 0 = 1
; and bit 1 = 0), we want bit 0 done first, so we need to lay color 1 first,
; thus offset 01 leads to the "white, black" entry.

colors:
        dw    black, black ; bits 00
        dw    white, black ; bits 10 -> 01
        dw    black, white ; bits 01 -> 10
        dw    white, white ; bits 11

msg1:        db    "This is the system font that I will be using in Karig."
len1:        equ    $-msg1

*/
}

/*------------------------------------------------------------------------------
* The following commented-out routines are for Planar modes
* outpw() is for word output, outp() is for byte output */

/* Initialize Planar (Write mode 2)
* Should be Called from initGraphics */
/*
void initPlanner()
{
    outpw(0x3C4, 0x0F02);
    outpw(0x3CE, 0x0003);
    outpw(0x3CE, 0x0205);
}
*/


/* Reset to Write Mode 0
* for BiOS default draw text */
/*
void setWriteMode0()
{
    outpw(0x3CE, 0xFF08);
    outpw(0x3CE, 0x0005);
}
*/
/* Plot a pixel in Planer mode */

/*

void putPixelP(int x, int y, int color)
{
    char dummy_read;

    long addr = (long)y * bytesperline +(x/8));
    setBank((int)(addr >> 16));
    outp(0x3CE, 8);
    outp(0x3CF, 0x80 >> (x & 7));
    dummy_read = *(screenPtr +^(addr & 0xFFFF));
    *(screenPtr + (addr & 0xFFFF)) = (char)color;
}
*/
//------------------------------------------------------------------------------

/* Set new read/write bank. We must set both Window A and Window B, as
 * many VBE's have these set as separately available read and write
 * windows. We also use a simple (but very effective) optimisation of
 * checking if the requested bank is currently active.
 */
 /*
void setBank(int bank)
{
    union REGS  regs;
    if (bank == curBank) return;    // Bank is already active
    curBank = bank;                 // Save current bank number
    bank <<= bankShift;             // Adjust to window granularity
#ifdef  DIRECT_BANKING
asm {   mov bx,0;
        mov dx,bank; }
    bankSwitch();
asm {   mov bx,1;
        mov dx,bank; }
    bankSwitch();
#else
    regs.x.ax = 0x4F05; regs.x.bx = 0;  regs.x.dx = bank;
    int86(0x10, &regs, &regs);
    regs.x.ax = 0x4F05; regs.x.bx = 1;  regs.x.dx = bank;
    int86(0x10, &regs, &regs);
#endif
}
*/


/* Below is the Assembly Language module required for the direct bank switching. In
* Borland C or other C compilers, this can be converted to in-lin assembly code.
*/

/*
public _setbxdx
.MODEL SMALL
.CODE
set_struc    struc
    dw    ?    ;old bp
    dd    ?    ; return addr ( always far call)
    p_bx    dw    ?    ; reg bx value
    p_dx    dw    ?    ; reg dx value
    set_struc    ends

    _setbxdx    proc far    ; must be far
        push    bp
        mov    bp, sp
        mov    bx,[bp]+p_bx
        mov    dx,[bp]+p_dx
        pop    bp
        ret
    _setbxdx    endp
    END
*/


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
