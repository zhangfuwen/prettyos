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
} list_t;

list_t*    list_create();                                          // Allocates memory for a list, returns a pointer to that list
bool       list_insert(list_t* list, element_t* next, void* data); // Inserts a new element before an element of the list (0 = append)
bool       list_append(list_t* list, void* data);                  // Inserts a new element at the end of the list
void       list_free(list_t* hd);                                  // Deletes everything that has been allocated for this list.
void       list_delete(list_t* list, void* data);                  // Deletes all elements with data from the list
element_t* list_getElement(list_t* list, uint32_t number);         // Returns the data at the position "number"


typedef struct
{
    element_t* begin;
    element_t* current;
} ring_t;

ring_t* ring_create();                                       // Allocates memory for a ring, returns a pointer to it
bool    ring_insert(ring_t* ring, void* data, bool single);  // Inserts an element in the ring at the current position. If single==true then it will be inserted only if its not already in the ring
bool    ring_isEmpty(ring_t* ring);                          // Returns true, if the ring is empty (data == 0)
bool    ring_deleteFirst(ring_t* ring, void* data);          // Deletes the first element equal to data, returns true if an element has been deleted
void    ring_move(ring_t* dest, ring_t* source, void* data); // Looks for an element with given data in source, takes it out from this ring and inserts it to dest. Instead of doing insert and delete this operations saves memory allocations


#endif
