#ifndef SYNCHRONISATION_H
#define SYNCHRONISATION_H

#include "scheduler.h"


typedef struct
{
    uint16_t resCount;  // Number of resources
    uint16_t freeRes;   // Index of the first unused resource in the array mentioned above
} semaphore_t;

semaphore_t* semaphore_create(uint16_t resourceCount);
void         semaphore_lock  (semaphore_t* obj);
void         semaphore_unlock(semaphore_t* obj);
void         semaphore_delete(semaphore_t* obj);


typedef struct
{
    task_t*  blocker; // Task that is blocking the mutex
    uint32_t blocks;  // Indicates whether this mutex is blocked at the moment or not. -> You have to call unlock as often as lock to unblock mutex.
} mutex_t;

mutex_t* mutex_create();
void     mutex_lock(mutex_t* obj);
void     mutex_unlock(mutex_t* obj);
void     mutex_delete(mutex_t* obj);


#endif
