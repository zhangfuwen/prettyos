#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "os.h"

enum EVENTS {
    NONE = 0,
    // EHCI-Events
    EHCI_INIT, EHCI_PORTCHECK,
    // Video-Events
    VIDEO_SCREENSHOT
};

void events_install();
uint32_t takeEvent();
void addEvent(uint32_t ID);

#endif
