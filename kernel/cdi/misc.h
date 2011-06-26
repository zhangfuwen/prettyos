#ifndef CDI_MISC_H
#define CDI_MISC_H

#include "types.h"
#include <cdi.h>

/* Registiert einen neuen IRQ-Handler.
   irq:    Nummer des zu reservierenden IRQ
   device: Geraet, das dem Handler als Parameter uebergeben werden soll */
void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*), struct cdi_device* device);

/* Setzt den IRQ-Zaehler fuer cdi_wait_irq zurueck.
   irq:    Nummer des IRQ
   return: 0 bei Erfolg, -1 im Fehlerfall */
int cdi_reset_wait_irq(uint8_t irq);

/* Wartet bis der IRQ aufgerufen wurde. Der interne Zähler muss zuerst mit cdi_reset_wait_irq zurückgesetzt werden. Damit auch die IRQs abgefangen
   werden können, die kurz vor dem Aufruf von dieser Funktion aufgerufen werden, sieht der korrekte Ablauf wie folgt aus:
   - cdi_reset_wait_irq
   - Hardware ansprechen und Aktionen ausführen, die schließlich den IRQ auslösen
   - cdi_wait_irq
   Der entsprechende IRQ muss zuvor mit cdi_register_irq registriert worden sein. Der registrierte Handler wird ausgeführt, bevor diese Funktion
   erfolgreich zurückkehrt.
   irq:     Nummer des IRQ auf den gewartet werden soll
   timeout: Anzahl der Millisekunden, die maximal gewartet werden sollen
   return:  0 wenn der irq aufgerufen wurde, -1 sonst. */
int cdi_wait_irq(uint8_t irq, uint32_t timeout);

/* Reserviert IO-Ports
   return: 0 wenn die Ports erfolgreich reserviert wurden, -1 sonst. */
int cdi_ioports_alloc(uint16_t start, uint16_t count);

/* Gibt reservierte IO-Ports frei
   return: 0 wenn die Ports erfolgreich freigegeben wurden, -1 sonst. */
int cdi_ioports_free(uint16_t start, uint16_t count);

// Unterbricht die Ausfuehrung fuer mehrere Millisekunden
void cdi_sleep_ms(uint32_t ms);

#endif
