#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "list.h"


typedef struct todoList
{
    listHead_t* queue;
} todoList_t;


todoList_t* todoList_create();                           // Allocates memory for a todoList_t and initializes it
void todoList_add(todoList_t* list, void (*function)()); // Takes a functionpointer
void todoList_clear(todoList_t* list);                   // Clears the queue without executing its content
void todoList_execute(todoList_t* list);                 // Executes the content of the queue and clears the queue
void todoList_wait(todoList_t* list);                    // Waits (using scheduler) until there is something to do
bool todoList_unlockTask(void* task);                    // Used for scheduler. Returns true if there are exercises in the list that blocks the task
void todoList_delete(todoList_t* list);                  // Frees memory of a todoList_t


#endif
