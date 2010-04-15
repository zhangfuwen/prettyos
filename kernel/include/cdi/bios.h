#ifndef CDI_BIOS_H
#define CDI_BIOS_H

#include "os.h"
#include <cdi/lists.h>

struct cdi_bios_registers {
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t ds;
    uint16_t es;
};

// Structure to access memory areas of a 16-bit Process
struct cdi_bios_memory {
    uintptr_t dest; // Virtuelle Addresse im Speicher des 16bit-Prozesses. Muss unterhalb von 0xC0000 liegen.

    /* Pointer auf reservierten Speicher für die Daten des Speicherbereichs. Wird beim Start des Tasks zum Initialisieren
       des Bereichs benutzt und erhaelt auch wieder die Daten nach dem BIOS-Aufruf. */
    void *src;

    uint16_t size; // Length of memory area
};

/* Ruft den BIOS-Interrupt 0x10 auf.
   registers: Pointer auf eine Register-Struktur. Diese wird beim Aufruf in die Register des Tasks geladen und 
                  bei Beendigung des Tasks wieder mit den Werten des Tasks gefuellt.
   memory:    Speicherbereiche, die in den Bios-Task kopiert und bei Beendigung des Tasks wieder zurueck kopiert
                  werden sollen. Die Liste ist vom Typ struct cdi_bios_memory.
   return:    0, wenn der Aufruf erfolgreich war, -1 bei Fehlern */
int cdi_bios_int10(struct cdi_bios_registers *registers, cdi_list_t memory);

#endif
