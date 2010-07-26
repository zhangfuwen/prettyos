#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "list.h"

typedef struct {
    void (*function)();
} event_handler_t;


extern event_handler_t EHCI_INIT, EHCI_PORTCHECK, VIDEO_SCREENSHOT;

void events_install();
void handleEvents();
void addEvent(event_handler_t* event);

#endif
