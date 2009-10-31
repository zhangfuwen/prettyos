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

uint8_t ShiftKeyDown = 0; // Variable for Shift Key Down
uint8_t KeyPressed   = 0; // Variable for Key Pressed
uint8_t scan         = 0; // Scan code from Keyboard

/* Wait until buffer is empty */
void keyboard_init()
{
    while( inportb(0x64)&1 )
        inportb(0x60);
};

uint8_t FetchAndAnalyzeScancode()
{
	if( inportb(0x64)&1 )
	    scan = inportb(0x60);   // 0x60: get scan code from the keyboard

    // ACK: toggle bit 7 at port 0x61
    uint8_t port_value = inportb(0x61);
    outportb(0x61,port_value |  0x80); // 0->1
    outportb(0x61,port_value &~ 0x80); // 1->0

	if( scan & 0x80 ) // Key released? Check bit 7 (10000000b = 0x80) of scan code for this
	{
        KeyPressed = 0;
        scan &= 0x7F; // Key was released, compare only low seven bits: 01111111b = 0x7F
        if( scan == KRLEFT_SHIFT || scan == KRRIGHT_SHIFT ) // A key was released, shift key up?
        {
            ShiftKeyDown = 0;	// yes, it is up --> NonShift
        }
	}
	else // Key was pressed
	{
	    KeyPressed = 1;
		if( scan == KRLEFT_SHIFT || scan == KRRIGHT_SHIFT )
		{
		    ShiftKeyDown = 1; // It is down, use asciiShift characters
		}
	}
	return scan;
}

uint8_t ScanToASCII()
{
	uint8_t retchar;	                   // The character that returns the scan code to ASCII code
	scan = FetchAndAnalyzeScancode();  // Grab scancode, and get the position of the shift key

	if( ShiftKeyDown )
	    retchar = asciiShift[scan];	   // (Upper) Shift Codes
	else
		retchar = asciiNonShift[scan]; // (Lower) Non-Shift Codes

	if( ( !(scan == KRLEFT_SHIFT || scan == KRRIGHT_SHIFT) ) && ( KeyPressed == 1 ) ) //filter Shift Key and Key Release
	    return retchar; // ASCII version
	else
	    return 0;
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
   //task_switch();
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



