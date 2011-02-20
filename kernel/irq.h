#ifndef IRQ_H
#define IRQ_H

#include "os.h"

typedef enum
{
    ISR_invalidOpcode = 6,
    ISR_NM            = 7,
    ISR_GPF           = 13,
    ISR_PF            = 14,
} ISR_NUM_t;

typedef enum
{
    IRQ_TIMER    = 0,
    IRQ_KEYBOARD = 1,
    IRQ_FLOPPY   = 6,
    IRQ_MOUSE    = 12,
    IRQ_SYSCALL  = 95
} IRQ_NUM_t;

void isr_install();
uint32_t irq_handler(uintptr_t esp);
void irq_installHandler(IRQ_NUM_t irq, void (*handler)(registers_t*));
void irq_uninstallHandler(IRQ_NUM_t irq);

void irq_resetCounter(IRQ_NUM_t number);
bool waitForIRQ(IRQ_NUM_t number, uint32_t timeout); // Call irq_resetCounter before to reset counter. Returns false in case of timeout.
bool irq_unlockTask(void* data);

#endif
