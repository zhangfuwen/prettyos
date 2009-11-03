#include "os.h"

#ifdef KEYMAP_GER
#include "keyboard_GER.h"
#endif

#ifdef KEYMAP_US
#include "keyboard_US.h"
#endif


//TASKSWITCH
#include "task.h"
extern uint32_t read_eip();
extern page_directory_t* current_directory;
extern task_t* current_task;
extern tss_entry_t tss_entry;

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
       *(pODA->pTailKQ) = KEY;
       ++(pODA->KQ_count_write);

       if(pODA->pTailKQ > pODA->KEYQUEUE)
       {
           --pODA->pTailKQ;
       }
       if(pODA->pTailKQ == pODA->KEYQUEUE)
       {
           pODA->pTailKQ = (pODA->KEYQUEUE)+KQSIZE-1;
       }
   }
}

int32_t k_checkKQ_and_print_char()
{
   if(pODA->KQ_count_write > pODA->KQ_count_read)
   {
       uint8_t KEY = *(pODA->pHeadKQ);
       ++(pODA->KQ_count_read);

       if(pODA->pHeadKQ > pODA->KEYQUEUE)
       {
           --pODA->pHeadKQ;
       }
       if(pODA->pHeadKQ == pODA->KEYQUEUE)
       {
           pODA->pHeadKQ = (pODA->KEYQUEUE)+KQSIZE-1;
       }

       //restore_cursor();
       switch(KEY)
       {
           case KINS:
               break;
           case KDEL:
               move_cursor_right();
               putch('\b'); //BACKSPACE
               break;
           case KHOME:
               move_cursor_home();
               break;
           case KEND:
               move_cursor_end();
               break;
           case KPGUP:
               break;
           case KPGDN:
               break;
           case KLEFT:
               move_cursor_left();
               break;
           case KUP:
               break;
           case KDOWN:
               break;
           case KRIGHT:
               move_cursor_right();
               break;
           default:
               printformat("%c",KEY); // the ASCII character
               break;
       }
       save_cursor();
       return 1;
   }
   return 0;
}

uint8_t k_checkKQ_and_return_char()
{
   if(pODA->KQ_count_write > pODA->KQ_count_read)
   {
       uint8_t KEY = *(pODA->pHeadKQ);
       ++(pODA->KQ_count_read);

       if(pODA->pHeadKQ > pODA->KEYQUEUE)
       {
           --pODA->pHeadKQ;
       }
       if(pODA->pHeadKQ == pODA->KEYQUEUE)
       {
           pODA->pHeadKQ = (pODA->KEYQUEUE)+KQSIZE-1;
       }
       /// TEST
	   //  printformat("KEY:%c ", KEY);
	   /// TEST
       return KEY;
   }
   return 0;
}

void keyboard_install()
{
    /* Installs 'keyboard_handler' to IRQ1 */
    irq_install_handler(1, keyboard_handler);
    keyboard_init();
}



