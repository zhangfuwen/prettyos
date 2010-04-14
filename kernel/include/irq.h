#ifndef IRQ_H
#define IRQ_H

#include "os.h"

uint32_t fault_handler(uint32_t esp);
uint32_t irq_handler(uint32_t esp);
void irq_install_handler(int32_t irq, void (*handler)(registers_t* r));
void irq_uninstall_handler(int32_t irq);

#endif
