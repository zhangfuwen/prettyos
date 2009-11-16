#include "os.h"
#include "task.h"
#include "syscall.h"

extern tss_entry_t tss_entry;

/* These are function prototypes for all of the exception handlers:
 * The first 32 entries in the IDT are reserved, and are designed to service exceptions! */
extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

extern void isr127(); //Software interrupt for sys calls

/* Set the first 32 entries in the IDT to the first 32 ISRs. Access flag is set to 0x8E: present, ring 0,
*  lower 5 bits set to the required '14' (hexadecimal '0x0E'). */
void isrs_install()
{
    idt_set_gate( 0, (uint32_t) isr0, 0x08, 0x8E);    idt_set_gate( 1, (uint32_t) isr1, 0x08, 0x8E);
    idt_set_gate( 2, (uint32_t) isr2, 0x08, 0x8E);    idt_set_gate( 3, (uint32_t) isr3, 0x08, 0x8E);
    idt_set_gate( 4, (uint32_t) isr4, 0x08, 0x8E);    idt_set_gate( 5, (uint32_t) isr5, 0x08, 0x8E);
    idt_set_gate( 6, (uint32_t) isr6, 0x08, 0x8E);    idt_set_gate( 7, (uint32_t) isr7, 0x08, 0x8E);
    idt_set_gate( 8, (uint32_t) isr8, 0x08, 0x8E);    idt_set_gate( 9, (uint32_t) isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    idt_set_gate(127,(uint32_t)isr127,0x08, 0x8E); //Software interrupt for sys calls
}

/* Message string corresponding to the exception number 0-31: exception_messages[interrupt_number] */
char* exception_messages[] =
{
    "Division By Zero",        "Debug",                         "Non Maskable Interrupt",    "Breakpoint",
    "Into Detected Overflow",  "Out of Bounds",                 "Invalid Opcode",            "No Coprocessor",
    "Double Fault",            "Coprocessor Segment Overrun",   "Bad TSS",                   "Segment Not Present",
    "Stack Fault",             "General Protection Fault",      "Page Fault",                "Unknown Interrupt",
    "Coprocessor Fault",       "Alignment Check",               "Machine Check",             "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved"
};

uint32_t fault_handler(uint32_t esp)
{
    uint32_t retVal;
    struct regs* r = (struct regs*)esp;

    if(!pODA->ts_flag)
    {
        retVal = esp;
    }
    else
    {
        if(r->int_no == 127) //syscall
        {
            /// TODO: decision about task_switch
            retVal = esp;
            //retVal = task_switch(esp);
            /// TODO: decision about task_switch
            //printformat(" nr: %d esp: %X ", r->eax, esp);
        }
        else
        {
            retVal = task_switch(esp); //new task's esp
        }
    }

    if (r->int_no < 32) //exception
    {
        settextcolor(4,0);

        if (r->int_no == 6 || r->int_no == 1) //Invalid Opcode
        {
            printformat("err_code: %X address(eip): %X\n", r->err_code, r->eip);
            printformat("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
            printformat("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
            printformat("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);
        }

        if (r->int_no == 14) //Page Fault
        {
            uint32_t faulting_address;
            __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address)); // faulting address <== CR2 register

            // The error code gives us details of what happened.
            int32_t present   = !(r->err_code & 0x1); // Page not present
            int32_t rw        =   r->err_code & 0x2;  // Write operation?
            int32_t us        =   r->err_code & 0x4;  // Processor was in user-mode?
            int32_t reserved  =   r->err_code & 0x8;  // Overwritten CPU-reserved bits of page entry?
            int32_t id        =   r->err_code & 0x10; // Caused by an instruction fetch?

            // Output an error message.
                          printformat("\nPage Fault (");
            if (present)  printformat("page not present");
            if (rw)       printformat(" read-only - write operation");
            if (us)       printformat(" user-mode");
            if (reserved) printformat(" overwritten CPU-reserved bits of page entry");
            if (id)       printformat(" caused by an instruction fetch");
                          printformat(") at %X - EIP: %X\n", faulting_address, r->eip);
        }

        /*
        printformat("TSS log:\n");
        TSS_log(&tss_entry);
        printformat("\n\n");
        */

        //log_task_list();
        //printformat("\n\n");

        printformat("err_code: %X address(eip): %X\n", r->err_code, r->eip);
        printformat("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
        printformat("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
        printformat("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);

        printformat("\n");
        printformat("%s >>> Exception. System Halted! <<<", exception_messages[r->int_no]);
        for (;;);
    }//if

    if (r->int_no == 127) //sw-interrupt syscall
    {
        syscall_handler(r);
    }
    return retVal;
}
