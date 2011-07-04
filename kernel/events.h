#ifndef EVENTS_H
#define EVENTS_H

#include "list.h"
#include "synchronisation.h"


#define MAX_EVENTS 2000 // One queue stores at maximum xx events. If a queue is full, an EVENT_OVERFLOW event is appended and no further events are accepted


typedef enum
{
    EVENT_NONE, EVENT_INVALID_ARGUMENTS, EVENT_OVERFLOW,
    EVENT_KEY_DOWN, EVENT_KEY_UP, EVENT_TEXT_ENTERED, // -> c.f. keyboard.h/c
    EVENT_TCP_CONNECTED, EVENT_TCP_RECEIVED // -> c.f. tcp.h/c
} EVENT_t;

typedef struct
{
    EVENT_t type;
    size_t  length;
    void*   data;
} event_t;

typedef struct
{
    list_t*  list;
    size_t   num;
    mutex_t* mutex;
} event_queue_t;


event_queue_t* event_createQueue();
void           event_deleteQueue(event_queue_t* queue);
void           event_issue(event_queue_t* destination, EVENT_t type, void* data, size_t length); // Sends an event to an event queue
EVENT_t        event_poll(void* destination, size_t maxLength, EVENT_t filter); // Takes an event from the event queue of the current task
void           event_enable(bool b); // Enables/Disables event handling for the current task


#endif
