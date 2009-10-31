/* PrettyOS is the result of a stepwise OS development
 * shown at www.henkessoft.de */

/// #define _DIAGNOSIS_ // activates prints to the screen about memory use

#ifndef OS_H
#define OS_H

/// //////////////////////////////////////
/// ////  TODO: ANSI TYPEDEFINITIONS  ////
/// //////////////////////////////////////

// old

#define TRUE   1
#define FALSE  0

/// //////////////////////////////////////

// new

typedef unsigned int         size_t;

typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t;
typedef unsigned char        uint8_t;

typedef signed long long     int64_t;
typedef signed long          int32_t;
typedef signed short         int16_t;
typedef signed char          int8_t;

typedef enum {false, true} bool;

/// //////////////////////////////////////


/// keyboard map
// #define KEYMAP_US // US keyboard
#define KEYMAP_GER // German keyboard // currently only TEST VERSION, does not run correctly

extern uint32_t placement_address;

#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))
extern void panic_assert(char* file, uint32_t line, char* desc);  // why char ?

/* This defines the operatings system common data area */

#define KQSIZE    20 // size of key queue

typedef struct oda
{
    // Hardware Data
    uint32_t COM1, COM2, COM3, COM4; // address
    uint32_t LPT1, LPT2, LPT3, LPT4; // address
    uint32_t Memory_Size;            // Memory size in Byte

    // Key Queue
    uint8_t  KEYQUEUE[KQSIZE];   // circular queue buffer
    uint8_t* pHeadKQ;            // pointer to the head of valid data
    uint8_t* pTailKQ;            // pointer to the tail of valid data
    uint32_t  KQ_count_read;      // number of data read from queue buffer
    uint32_t  KQ_count_write;     // number of data put into queue buffer

    //tasking
    uint8_t  ts_flag;            // 0: taskswitch off  1: taskswitch on
}oda_t;

// operatings system common data area
oda_t ODA;
extern oda_t* pODA;

typedef struct Mem_Chunk_struct
 {
   uint32_t base_lo;
   uint32_t base_hi;
   uint32_t length_lo;
   uint32_t length_hi;
   uint32_t type;
   uint32_t extended;
 }Mem_Chunk_t;


/* This defines what the stack looks like after an ISR was running */
typedef struct regs
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
}registers_t;

// video.c
extern void k_clear_screen();
extern void settextcolor(uint8_t forecolor, uint8_t backcolor);
extern void putch(int8_t c);
extern void puts(int8_t* text);
extern void scroll();
extern void k_printf(int8_t* message, uint32_t line, uint8_t attribute);
extern void set_cursor(uint8_t x, uint8_t y);
extern void update_cursor();
extern void move_cursor_right();
extern void move_cursor_left();
extern void move_cursor_home();
extern void move_cursor_end();
extern void save_cursor();
extern void restore_cursor();
extern void printformat (char *args, ...); // why not int8_t ?

// timer.c
extern void timer_handler(struct regs* r);
extern void timer_wait (uint32_t ticks);
extern void sleepSeconds (uint32_t seconds);
extern void sleepMilliSeconds (uint32_t ms);
extern void systemTimer_setFrequency( uint32_t freq );
extern void timer_install();
extern void timer_uninstall();

// keyboard.c
extern void keyboard_init();
extern uint8_t FetchAndAnalyzeScancode();
extern uint8_t ScanToASCII();
extern void keyboard_handler(struct regs* r);
extern int32_t k_checkKQ_and_print_char();
extern uint8_t k_checkKQ_and_return_char();

// util.c
extern void initODA();
extern void outportb(uint16_t port, uint32_t val);
extern uint32_t inportb(uint16_t port);
extern void outportl(uint16_t port, uint32_t val);
extern uint32_t inportl(uint16_t port);
extern uint32_t fetchESP();
extern uint32_t fetchEBP();
extern uint32_t fetchSS();
extern uint32_t fetchCS();
extern uint32_t fetchDS();
extern void k_memshow(void* start, size_t count);
extern void* k_memset(void* dest, int8_t val, size_t count);
extern uint16_t* k_memsetw(uint16_t* dest, uint16_t val, size_t count);
extern void* k_memcpy(void* dest, const void* src, size_t count);
extern size_t k_strlen(const int8_t* str);
extern int32_t k_strcmp( const int8_t* s1, const int8_t* s2 );
extern int8_t* k_strcpy(int8_t* dest, const int8_t* src);
extern int8_t* k_strncpy(int8_t* dest, const int8_t* src, size_t n);
extern int8_t* k_strcat(int8_t* dest, const int8_t* src);
extern void reboot();
extern void cli();
extern void sti();
extern void nop();
extern void k_itoa(int32_t value, int8_t* valuestring);
extern void k_i2hex(uint32_t val, int8_t* dest, int32_t len);
extern void float2string(float value, int32_t decimal, int8_t* valuestring);

// gtd.c itd.c irq.c isrs.c
extern void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
extern void gdt_install();
extern void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
extern void idt_install();
extern void isrs_install();
extern void irq_install();

extern void timer_install();
extern void keyboard_install();

extern uint32_t fault_handler(uint32_t esp);
extern uint32_t irq_handler(uint32_t esp);
extern void irq_install_handler(int32_t irq, void (*handler)(struct regs* r));
extern void irq_uninstall_handler(int32_t irq);
extern void irq_remap(void);


// math.c
extern int32_t k_abs(int32_t i);
extern int32_t k_power(int32_t base,int32_t n);

// paging.c
extern void paging_install();
extern uint32_t k_malloc(uint32_t size, uint8_t align, uint32_t* phys);
extern void analyze_frames_bitset(uint32_t sec);
extern uint32_t show_physical_address(uint32_t virtual_address);
extern void analyze_physical_addresses();

#endif
