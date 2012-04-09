#include "apic.h"
#include "cpu.h"
#include "video/console.h"
#include "util/util.h"


static volatile uint32_t* apic_base = 0;


// Some APIC registers
enum {
    APIC_TASKPRIORITY = 0x20,
    APIC_SPURIOUSINTERRUPT = 0x3C,
    APIC_TIMER = 0xC8,
    APIC_THERMALSENSOR = 0xCC,
    APIC_PERFORMANCECOUNTER = 0xD0,
    APIC_LINT0 = 0xD4,
    APIC_LINT1 = 0xD8,
    APIC_ERROR = 0xDC
};


bool apic_available()
{
    return(cpu_supports(CF_MSR) && cpu_supports(CF_APIC)); // We need MSR (to initialize APIC) and (obviously) APIC.;
}

bool apic_install()
{
    apic_base = (uint32_t*)(uintptr_t)(cpu_MSRread(0x1B) & (~0xFFF)); // APIC base address can be read from MSR 0x1B
    printf("\nAPIC base: %X", apic_base);

    apic_base[APIC_SPURIOUSINTERRUPT] = 0x10F; // Enable APIC. Spurious Vector is 15
    apic_base[APIC_TASKPRIORITY] = 0x20; // Inhibit software interrupt delivery
    apic_base[APIC_TIMER] = 0x10000; // Disable timer interrupts
    apic_base[APIC_THERMALSENSOR] = 0x10000; // Disable thermal sensor interrupts
    apic_base[APIC_PERFORMANCECOUNTER] = 0x10000; // Disable performance counter interrupts
    apic_base[APIC_LINT0] = 0x8700; // Enable normal external Interrupts
    apic_base[APIC_LINT1] = 0x400; // Enable NMI Processing
    apic_base[APIC_ERROR] = 0x10000; // Disable Error Interrupts

    return(true); // Successful
}
