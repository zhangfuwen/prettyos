#include "os.h"

#ifdef KEYMAP_GER
#include "keyboard_GER.h"
#endif

#ifdef KEYMAP_US
#include "keyboard_US.h"
#endif




 // Key Queue
uint8_t  KEYQUEUE[KQSIZE];   // circular queue buffer
uint8_t* pHeadKQ;            // pointer to the head of valid data
uint8_t* pTailKQ;            // pointer to the tail of valid data
uint32_t KQ_count_read;      // number of data read from queue buffer
uint32_t KQ_count_write;     // number of data put into queue buffer




uint8_t ShiftKeyDown = 0; // variable for Shift Key Down
uint8_t AltGrKeyDown = 0; // variable for AltGr Key Down
uint8_t KeyPressed   = 0; // variable for Key Pressed
uint8_t curScan      = 0; // current scan code from Keyboard
uint8_t prevScan     = 0; // previous scan code

/* wait until buffer is empty */
void keyboard_init()
{
    while( inportb(0x64)&1 )
        inportb(0x60);
};

uint8_t FetchAndAnalyzeScancode()
{
	if( inportb(0x64)&1 )
	    curScan = inportb(0x60);   // 0x60: get scan code from the keyboard

    // ACK: toggle bit 7 at port 0x61
    uint8_t port_value = inportb(0x61);
    outportb(0x61,port_value |  0x80); // 0->1
    outportb(0x61,port_value &~ 0x80); // 1->0

	if( curScan & 0x80 ) // Key released? Check bit 7 (10000000b = 0x80) of scan code for this
	{
        KeyPressed = 0;
        curScan &= 0x7F; // Key was released, compare only low seven bits: 01111111b = 0x7F
        if( curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT ) // A key was released, shift key up?
        {
            ShiftKeyDown = 0;	// yes, it is up --> NonShift
        }
        if( (curScan == 0x38) && (prevScan == 0x60) )
		{
            AltGrKeyDown = 0;
		}
	}
	else // Key was pressed
	{
	    KeyPressed = 1;
		if( curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT )
		{
		    ShiftKeyDown = 1; // It is down, use asciiShift characters
		}
		if( (curScan == 0x38) && (prevScan == 0x60) )
		{
            AltGrKeyDown = 1;
		}
	}
	prevScan = curScan;
	return curScan;
}

uint8_t ScanToASCII()
{
	uint8_t retchar = 0;                  // The character that returns the scan code to ASCII code
	curScan = FetchAndAnalyzeScancode();  // Grab scancode, and get the position of the shift key

    /// TEST
    //  printformat(" scan:%d ",scan);
    /// TEST

    if( ShiftKeyDown )
	{
	    if( AltGrKeyDown)
        {
            /* no reaction */
        }
        else
        {
            retchar = asciiShift[curScan];
        }
	}
	else
	{
	    if( AltGrKeyDown)
        {
            #ifdef KEYMAP_GER
            retchar = asciiAltGr[curScan];
            #endif

            #ifdef KEYMAP_US
            /// not yet implemented
            #endif
        }
        else
        {
		    retchar = asciiNonShift[curScan]; // (Lower) Non-Shift Codes
        }
	}

    //filter Shift Key and Key Release
	if( ( !(curScan == KRLEFT_SHIFT || curScan == KRRIGHT_SHIFT) ) && ( KeyPressed == 1 ) )
	{
	    /// TEST
	    //  printformat("ascii:%x ", retchar);
	    /// TEST

	    return retchar; // ASCII version
	}
	else
	{
	    return 0;
	}
}

void keyboard_handler(struct regs* r)
{
   uint8_t KEY = ScanToASCII();
   if(KEY)
   {
       *(pTailKQ) = KEY;
       ++(KQ_count_write);

       if(pTailKQ > KEYQUEUE)
       {
           --pTailKQ;
       }
       if(pTailKQ == KEYQUEUE)
       {
           pTailKQ = (KEYQUEUE)+KQSIZE-1;
       }
   }
}

uint8_t checkKQ_and_return_char() // get a character <--- TODO: make it POSIX like
{
   /// TODO: should only return character, if keystroke was entered

   cli();
   if(KQ_count_write > KQ_count_read)
   {
       uint8_t KEY = *(pHeadKQ);
       ++(KQ_count_read);

       if(pHeadKQ > KEYQUEUE)
       {
           --pHeadKQ;
       }
       if(pHeadKQ == KEYQUEUE)
       {
           pHeadKQ = (KEYQUEUE)+KQSIZE-1;
       }
       return KEY;
   }
   sti();
   return 0;
}

void keyboard_install()
{
    // Setup the queue
    for ( int i=0; i<KQSIZE; ++i )
    {
       KEYQUEUE[i]=0;
    }
    pHeadKQ = KEYQUEUE;
    pTailKQ = KEYQUEUE;
    KQ_count_read  = 0;
    KQ_count_write = 0;

    // Installs 'keyboard_handler' to IRQ1
    irq_install_handler(32+1, keyboard_handler);
    keyboard_init();
}
