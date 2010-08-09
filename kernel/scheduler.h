#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "os.h"

typedef struct task task_t;

typedef struct {
    bool eventBased;
    bool (*unlock)(task_t*);
} blockerType_t;

extern blockerType_t BL_TIME, BL_SEMAPHORE, BL_INTERRUPT, BL_TASK;

typedef struct
{
    blockerType_t* type;
    void*          data;
} blocker_t;

void    scheduler_install();
bool    scheduler_shouldSwitchTask();
task_t* scheduler_getNextTask();
void    scheduler_insertTask(task_t* task);
void    scheduler_deleteTask(task_t* task);
void    scheduler_blockCurrentTask(blockerType_t* reason, void* data);
void    scheduler_unblockEvent(blockerType_t* type, void* data);
void    scheduler_log();


#endif
