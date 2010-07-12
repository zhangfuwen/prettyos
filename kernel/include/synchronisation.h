#ifndef SYNCHRONISATION_H
#define SYNCHRONISATION_H

#include "task.h"

typedef struct semaphore
{
    task_t** resources;
    uint16_t resCount;
    uint32_t freeRes;
} semaphore_t;

semaphore_t* semaphore_create(uint16_t resourceCount);
void         semaphore_lock  (semaphore_t* obj);
void         semaphore_unlock(semaphore_t* obj);
void         semaphore_delete(semaphore_t* obj);

#endif
