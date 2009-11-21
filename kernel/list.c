#include "list.h"
#include "paging.h" // k_malloc
#include "kheap.h"    // kfree

//listHead_t* myList = 0;

listHead_t* listCreate()
{
    listHead_t* hd = (listHead_t*)k_malloc(sizeof(listHead_t),0);
    if(hd)
    {
        hd->head = hd->tail = 0;
        return hd;
    }
    return 0;
}

int8_t listAppend(listHead_t* hd, void* data)
{
    element_t* ap = (element_t*)k_malloc(sizeof(element_t),0);
    if(ap)
    {
        ap->data = data;
        ap->next = 0;

        if(!hd->head) // there exist no list element
        {
            hd->head = hd->tail = ap;
            return 1;
        }
        else   // there exist at least one list element
        {
            hd->tail->next = ap;
            hd->tail = ap;
            return 1;
        }
    }
    return 0;
}

void listDeleteAll(struct listHead* hd)
{
    element_t* cur = hd->head;
    element_t* nex;

    while(cur)
    {
        nex=cur->next;
        k_free(cur->data);
        k_free(cur);
        cur=nex;
    }
    k_free(hd);
}

int8_t listInput(listHead_t* hd)
{
    void* dat = (void*)k_malloc(sizeof(void*),0);
    if(dat)
    {
        if(listAppend(hd,dat))
        {
            return 1;
        }
        else
        {
            k_free(dat);
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

void listShow(listHead_t* hd)
{
    printformat("List elements:\n");
    element_t* cur = hd->head;
    if(!cur)
    {
        printformat("The list is empty.");
    }
    else
    {
        while(cur)
        {
            printformat("%X\t",cur);
            cur = cur->next;
        }
    }
}

void* listShowElement(listHead_t* hd, uint32_t number)
{
    element_t* cur = hd->head;
    void* dat=0;
    if(!cur)
    {
        /* printformat(""); */
    }
    else
    {
        uint32_t index=0;
        while(cur)
        {
            ++index;
            if(index==number)
            {
                dat = cur->data;
            }
            cur = cur->next;
        }
    }
    return dat;
}

