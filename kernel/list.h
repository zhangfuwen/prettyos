#ifndef LIST_H
#define LIST_H

#include "os.h"


// double linked list element
typedef struct dlelement dlelement_t;
struct dlelement
{
    void*        data;
    dlelement_t* prev;
    dlelement_t* next;
};

typedef struct
{
    dlelement_t* head;
    dlelement_t* tail;
} list_t;


list_t*      list_create();                                            // Allocates memory for a list, returns a pointer to that list.
void         list_free(list_t* hd);                                    // Deletes everything that has been allocated for this list.
dlelement_t* list_insert(list_t* list, dlelement_t* next, void* data); // Inserts a new element before an element of the list (0 = append). Returns a pointer to the new element.
dlelement_t* list_append(list_t* list, void* data);                    // Inserts a new element at the end of the list and returns a pointer to it.
dlelement_t* list_delete(list_t* list, dlelement_t* elem);             // Deletes the element elem and returns a pointer to the element that was behind it.
dlelement_t* list_getElement(list_t* list, uint32_t number);           // Returns the data at the position "number".
dlelement_t* list_find(list_t* list, void* data);                      // Finds an element with data in the list and returns a pointer to it.


#endif
