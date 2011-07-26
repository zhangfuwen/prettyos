#ifndef LIST_H
#define LIST_H

#include "os.h"


typedef struct
{
    element_t* head;
    element_t* tail;
} list_t;


list_t*    list_create();                                          // Allocates memory for a list, returns a pointer to that list
bool       list_insert(list_t* list, element_t* next, void* data); // Inserts a new element before an element of the list (0 = append)
bool       list_append(list_t* list, void* data);                  // Inserts a new element at the end of the list
void       list_free(list_t* hd);                                  // Deletes everything that has been allocated for this list.
element_t* list_delete(list_t* list, void* data);                  // Deletes all elements with data from the list. Returns a pointer to the element behind the first deleted element.
element_t* list_getElement(list_t* list, uint32_t number);         // Returns the data at the position "number"


#endif
