#ifndef LIST_H
#define LIST_H

#include "os.h"

typedef struct element element_t;

struct element
{
    void*    data;
    element_t* next;
};

typedef struct listHead
{
    element_t* head;
    element_t* tail;
} listHead_t;


listHead_t* list_Create();                                    // allocates memory for a list, returns a pointer to that list
bool        list_Append(listHead_t* hd, void* data);          // Inserts a new element at the end of the list
void        list_DeleteAll(listHead_t* hd);                   // Deletes everything that has been allocated for this list.
void        list_Show(listHead_t* hd);                        // prints the list on the Screen
void        list_Delete(listHead_t* list, void* data);        // Deletes all elements with data from the list
element_t*  list_GetElement(listHead_t* hd, uint32_t number); // Returns the data at the position "number"

element_t* ring_Create();                                 // allocates memory for a ring, returns a pointer to it
void       ring_Insert(element_t* ring, void* data);      // Inserts an element in the ring at the current position
bool       ring_isEmpty(element_t* ring);                 // returns true, if the ring is empty (data == 0)
bool       ring_DeleteFirst(element_t* ring, void* data); // Deletes the first element equal to data, returns true if an element has been deleted

#endif
