/* PrettyOS is the result of a stepwise OS development
 * shown at www.henkessoft.de */

// #define _DIAGNOSIS_ // activates prints to the screen about memory use

#ifndef OS_H
#define OS_H

/// //////////////////////////////////////
/// ////  TODO: ANSI TYPEDEFINITIONS  ////
/// //////////////////////////////////////


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

#define bool _Bool
#define true   1
#define false  0
#define __bool_true_false_are_defined 1

#define NULL   0

#define PAGESIZE 4096

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
extern void putch(char c);
extern void puts(char* text);
extern void scroll();
extern void k_printf(char* message, uint32_t line, uint8_t attribute);
extern void set_cursor(uint8_t x, uint8_t y);
extern void update_cursor();
extern void move_cursor_right();
extern void move_cursor_left();
extern void move_cursor_home();
extern void move_cursor_end();
extern void save_cursor();
extern void restore_cursor();
extern void printformat (char *args, ...);

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
extern size_t k_strlen(const char* str);
extern int32_t k_strcmp( const char* s1, const char* s2 );
extern char* k_strcpy(char* dest, const char* src);
extern char* k_strncpy(char* dest, const char* src, size_t n);
extern char* k_strcat(char* dest, const char* src);
extern void reboot();
extern void cli();
extern void sti();
extern void nop();
extern void k_itoa(int32_t value, char* valuestring);
extern void k_i2hex(uint32_t val, char* dest, int32_t len);
extern void float2string(float value, int32_t decimal, char* valuestring);
uint32_t alignUp( uint32_t val, uint32_t alignment );
uint32_t alignDown( uint32_t val, uint32_t alignment );
uint32_t max( uint32_t a, uint32_t b );
uint32_t min( uint32_t a, uint32_t b );

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
extern void analyze_frames_bitset(uint32_t sec);
extern uint32_t show_physical_address(uint32_t virtual_address);
extern void analyze_physical_addresses();

#endif
