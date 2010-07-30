#ifndef IRQ_H
#define IRQ_H

#include "os.h"

uint32_t irq_handler(uint32_t esp);
void irq_installHandler(int32_t irq, void (*handler)(registers_t* r));
void irq_uninstallHandler(int32_t irq);

#endif
