#ifndef LIST_H
#define LIST_H

#include "os.h"

typedef struct element element_t;

struct element
{
    void*    data;
    element_t* next;
};

typedef struct
{
    element_t* head;
    element_t* tail;
} listHead_t;


listHead_t* listCreate();                                    // allocates memory for a list, returns a pointer to that list
int8_t      listAppend(listHead_t* hd, void* data);          // Inserts a new (malloc) element at the end of the list
void        listDeleteAll(listHead_t* hd);                   // Deletes everything that has been allocated for this list.
void        listShow(listHead_t* hd);                        // prints the list on the Screen
void*       listGetElement(listHead_t* hd, uint32_t number); // Returns the data at the position "number"

#endif
