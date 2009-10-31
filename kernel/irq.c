#include "os.h"
#include "task.h"

/* Own ISR pointing to individual IRQ handler instead of the regular 'fault_handler' function */
extern void irq0();  extern void irq1();  extern void irq2();  extern void irq3();
extern void irq4();  extern void irq5();  extern void irq6();  extern void irq7();
extern void irq8();  extern void irq9();  extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

/* Array of function pointers handling custom IRQ handlers for a given IRQ */
//void* irq_routines[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
void* irq_routines[128];

/* Implement a custom IRQ handler for the given IRQ */
void irq_install_handler(int32_t irq, void (*handler)(struct regs* r)) {irq_routines[irq] = handler;}

/* Clear the custom IRQ handler */
void irq_uninstall_handler(int32_t irq) {irq_routines[irq] = 0;}

/* Remap: IRQ0 to IRQ15 have to be remapped to IDT entries 32 to 47 */
void irq_remap()
{
    // starts the initialization sequence
    outportb(0x20, 0x11); outportb(0xA0, 0x11);

    // define the PIC vectors
    outportb(0x21, 0x20); // Set offset of Master PIC to 0x20 (32): Entry 32-39
    outportb(0xA1, 0x28); // Set offset of Slave  PIC to 0x28 (40): Entry 40-47

    // continue initialization sequence
    outportb(0x21, 0x04); outportb(0xA1, 0x02);
    outportb(0x21, 0x01); outportb(0xA1, 0x01);
    outportb(0x21, 0x00); outportb(0xA1, 0x00);
}

/* After remap of the interrupt controllers the appropriate ISRs are connected to the correct entries in the IDT. */
void irq_install()
{
    irq_remap();
    idt_set_gate(32, (uint32_t) irq0,  0x08, 0x8E);   idt_set_gate(33, (uint32_t) irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t) irq2,  0x08, 0x8E);   idt_set_gate(35, (uint32_t) irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t) irq4,  0x08, 0x8E);   idt_set_gate(37, (uint32_t) irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t) irq6,  0x08, 0x8E);   idt_set_gate(39, (uint32_t) irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t) irq8,  0x08, 0x8E);   idt_set_gate(41, (uint32_t) irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t) irq10, 0x08, 0x8E);   idt_set_gate(43, (uint32_t) irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t) irq12, 0x08, 0x8E);   idt_set_gate(45, (uint32_t) irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t) irq14, 0x08, 0x8E);   idt_set_gate(47, (uint32_t) irq15, 0x08, 0x8E);
}

uint32_t irq_handler(uint32_t esp)
{
    uint32_t retVal;
    struct regs* r = (struct regs*)esp;

    if(!pODA->ts_flag)
        retVal = esp;
    else
        retVal = task_switch(esp); //new task's esp

    void (*handler)(struct regs* r);
    handler = irq_routines[r->int_no - 32];
    if(handler) { handler(r); }

    if(r->int_no >= 40) { outportb(0xA0, 0x20); }
    outportb(0x20, 0x20);

    return retVal;
}
