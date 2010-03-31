/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
*/

#ifndef OS_H
#define OS_H


/// #define _DIAGNOSIS_ // activates prints to the screen about some details and memory use


// typedefs for char, short, int, long, ...
typedef unsigned int         size_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t;
typedef unsigned char        uint8_t;
typedef signed long long     int64_t;
typedef signed long          int32_t;
typedef signed short         int16_t;
typedef signed char          int8_t;
typedef uint32_t             uintptr_t;

// typedefs for CDI
typedef struct {} FILE;     /// Dummy: Please fix it
typedef unsigned int dev_t; // Defined like in tyndur
typedef unsigned int uid_t; // Defined like in tyndur
typedef unsigned int gid_t; // Defined like in tyndur


#define NULL (void*)0
#define bool _Bool
#define true   1
#define false  0
#define __bool_true_false_are_defined 1

// Macros
#define FORM_SHORT(a,b) (((b)<<8)|a)
#define FORM_LONG(a,b,c,d) (((d)<<24)|((c)<<16)|((b)<<8)|(a))
#define BYTE1(a) ( (a)       & 0xFF)
#define BYTE2(a) (((a)>> 8 ) & 0xFF)
#define BYTE3(a) (((a)>>16 ) & 0xFF)
#define BYTE4(a) (((a)>>24 ) & 0xFF)


//// Memory configuration ////
// BE AWARE, THE MEMORY MANAGEMENT CODE STILL HAS SOME HARDCODED
//       ADDRESSES, SO DO NOT CHANGE THESE ONES HERE!

// The page size must not be changed
#define PAGESIZE 0x1000  // 4096 Byte = 4KByte

// Where the kernel's private data is stored (virtual addresses)
#define KERNEL_DATA_START (0xC0000000)    //3 GB
#define KERNEL_DATA_END   (0x100000000)   //4 GB

// PCI/EHCI memory location for MM IO
#define PCI_MEM_START ((char*)0xFFF00000)
#define PCI_MEM_END   ((char*)0x100000000)

// Virtual adress area for the kernel heap
#define KERNEL_HEAP_START KERNEL_DATA_START
#define KERNEL_HEAP_END   PCI_MEM_START
#define KERNEL_HEAP_SIZE  (KERNEL_HEAP_END - KERNEL_HEAP_START)

// Placement allocation
#define PLACEMENT_BEGIN ((char*)(16*1024*1024))
#define PLACEMENT_END   ((char*)(20*1024*1024))

// User Heap management
#define USER_HEAP_START ((char*)(20*1024*1024))
#define USER_HEAP_END   ((char*)(KERNEL_DATA_START - 16*1024*1024))

// User Stack
#define USER_STACK 0x1420000


/// keyboard map
// #define KEYMAP_US // US keyboard
#define KEYMAP_GER   // German keyboard

extern uint32_t placement_address;

#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))
void panic_assert(const char* file, uint32_t line, const char* desc);  // why char ?

/* This defines the operatings system common data area */

#define KQSIZE    20 // size of key queue

struct oda
{
    // hardware data
    uint32_t Memory_Size;        // memory size in byte

    //tasking
    uint8_t  ts_flag;            // 0: taskswitch off  1: taskswitch on

    // floppy disk
    bool  flpy_motor[4];         // 0: motor off  1: motor on
                                 // array index is number of floppy drive (0,1,2,3)
    uint32_t CPU_Frequency_kHz;  // determined from rdtsc

}__attribute__((packed));

typedef struct oda oda_t;

// operatings system common data area
extern oda_t* pODA;


/* This defines what the stack looks like after an ISR was running */
struct regs
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed));

typedef struct regs registers_t;

// PrettyOS Version string
extern const char* version;

// video.c
void refreshUserScreen();
void clear_screen();
void kprintf(const char* message, uint32_t line, uint8_t attribute, ...);
uint8_t AsciiToCP437(uint8_t ascii);

// console.c
static const uint8_t COLUMNS = 80;
static const uint8_t USER_LINES = 46;
void clear_console(uint8_t backcolor);
void settextcolor(uint8_t forecolor, uint8_t backcolor);
void putch(char c);
void puts(const char* text);
void printf (const char *args, ...);
void cprintf(const char* message, uint32_t line, uint8_t attribute, ...);
void scroll();
void set_cursor(uint8_t x, uint8_t y);
void update_cursor();

// timer.c
uint16_t systemfrequency; // system frequency
uint32_t getCurrentSeconds();
uint32_t getCurrentMilliseconds();
void timer_handler(struct regs* r);
void timer_wait(uint32_t ticks);
void sleepSeconds(uint32_t seconds);
void sleepMilliSeconds(uint32_t ms);
void systemTimer_setFrequency(uint32_t freq);
uint16_t systemTimer_getFrequency();
void timer_install();
void timer_uninstall();
void delay(uint32_t microsec);

// keyboard.c
void keyboard_install();
uint8_t FetchAndAnalyzeScancode();
uint8_t ScanToASCII();
void keyboard_handler(struct regs* r);
int32_t checkKQ_and_print_char();
uint8_t checkKQ_and_return_char();
bool testch();

// gtd.c, irq.c, interrupts.asm
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_install();
void idt_install();
uint32_t fault_handler(uint32_t esp);
uint32_t irq_handler(uint32_t esp);
void irq_install_handler(int32_t irq, void (*handler)(struct regs* r));
void irq_uninstall_handler(int32_t irq);

// math.c
uint32_t max(uint32_t a, uint32_t b);
uint32_t min(uint32_t a, uint32_t b);
int32_t abs(int32_t i);
int32_t power(int32_t base,int32_t n);

// paging.c
void analyze_frames_bitset(uint32_t sec);
uint32_t show_physical_address(uint32_t virtual_address);
void analyze_physical_addresses();

// elf.c
bool elf_exec( const void* elf_file, uint32_t elf_file_size, const char* programName );

// util.c
uint8_t inportb(uint16_t port);
uint16_t inportw(uint16_t port);
uint32_t inportl(uint16_t port);
void outportb(uint16_t port, uint8_t val);
void outportw(uint16_t port, uint16_t val);
void outportl(uint16_t port, uint32_t val);

uint32_t fetchESP();
uint32_t fetchEBP();
uint32_t fetchSS();
uint32_t fetchCS();
uint32_t fetchDS();
uint64_t rdtsc();

void memshow(void* start, size_t count);
void* memset(void* dest, int8_t val, size_t count);
uint16_t* memsetw(uint16_t* dest, uint16_t val, size_t count);
uint32_t* memsetl(uint32_t* dest, uint32_t val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);

void sprintf (char *buffer, const char *args, ...);
size_t strlen(const char* str);
int32_t strcmp( const char* s1, const char* s2 );
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);

void reboot();

void cli();
void sti();
void nop();

int8_t ctoi(char c);
void itoa(int32_t value, char* valuestring);
void i2hex(uint32_t val, char* dest, int32_t len);
void float2string(float value, int32_t decimal, char* valuestring);

uint8_t PackedBCD2Decimal(uint8_t PackedBCDVal);

uint32_t alignUp( uint32_t val, uint32_t alignment );
uint32_t alignDown( uint32_t val, uint32_t alignment );

// network
void rtl8139_handler(struct regs* r);
void install_RTL8139(uint32_t number);

// USB 2.0 EHCI
void analyzeEHCI(uint32_t bar);

#endif

/*
* Copyright (c) 2009 The PrettyOS Project. All rights reserved.
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
