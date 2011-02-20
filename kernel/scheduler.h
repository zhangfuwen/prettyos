#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "os.h"

typedef struct task task_t;

typedef struct
{
    bool (*unlock)(void*); // if 0, the blocker is event based
} blockerType_t;

extern blockerType_t BL_SEMAPHORE, BL_INTERRUPT, BL_TASK, BL_TODOLIST;

typedef struct
{
    blockerType_t* type;
    void*          data;    // While the task is blocked, it contains information for the unblock functions. After the block ended, it is set to 0 in case of timeout and 1 otherwise
    uint32_t       timeout; // 0 if no timeout is defined.
} blocker_t;

void    scheduler_install();
bool    scheduler_shouldSwitchTask();
task_t* scheduler_getNextTask();
void    scheduler_insertTask(task_t* task);
void    scheduler_deleteTask(task_t* task);
bool    scheduler_blockCurrentTask(blockerType_t* reason, void* data, uint32_t timeout); // false in case of timeout
void    scheduler_unblockEvent(blockerType_t* type, void* data);
void    scheduler_log();


#endif
