#ifndef LIST_H
#define LIST_H

#include "os.h"

struct element;
typedef struct element element_t;

struct element
{
    void*    data;
    element_t* next;
};

typedef
struct listHead
{
    element_t* head;
    element_t* tail;
}listHead_t;

listHead_t* listCreate();
int8_t      listAppend(listHead_t* hd, void* data);
void        listDeleteAll(listHead_t* hd);
int8_t      listInput(listHead_t* hd);
void        listShow(listHead_t* hd);
void*       listShowElement(listHead_t* hd, uint32_t number);

#endif
