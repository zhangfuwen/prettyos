#include "os.h"
#include "descriptor_tables.h"

// ASM functions in flush.asm
extern void gdt_flush(uint32_t);
extern void idt_flush(uint32_t);
extern void tss_flush();

// Internal function prototypes.
void write_tss(int32_t,uint16_t,uint32_t);

tss_entry_t tss;


// Initialise our task state segment structure.
void write_tss(int32_t num, uint16_t ss0, uint32_t esp0)
{
    // Firstly, let's compute the base and limit of our entry into the GDT.
    uint32_t base = (uint32_t) &tss;
    uint32_t limit = sizeof(tss); //http://forum.osdev.org/viewtopic.php?f=1&t=19819&p=155587&hilit=tss_entry#p155587

    // Now, add our TSS descriptor's address to the GDT.
    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    // Ensure the descriptor is initially zero.
    memset(&tss, 0, sizeof(tss));

    tss.ss0  = ss0;  // Set the kernel stack segment.
    tss.esp0 = esp0; // Set the kernel stack pointer.

    tss.cs   = 0x08;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10;
}

void set_kernel_stack(uint32_t stack)
{
    tss.esp0 = stack;
}
