#ifndef SYNCHRONISATION_H
#define SYNCHRONISATION_H

#include "scheduler.h"
#include "os.h"

typedef struct semaphore
{
    task_t** resources; // Array of resources. Each resource can be used by one task.
    uint16_t resCount;  // Number of resources
    uint32_t freeRes;   // Index of the first unused resource in the array mentioned above
} semaphore_t;

semaphore_t* semaphore_create(uint16_t resourceCount);
bool         semaphore_unlockTask(void* task);
bool         semaphore_locked(semaphore_t*obj, task_t* task); // returns if this semaphore is locked or not. If you specify a task, this function only looks for this task as a locker
void         semaphore_lock  (semaphore_t* obj);
void         semaphore_unlock(semaphore_t* obj);
void         semaphore_delete(semaphore_t* obj);

#endif
