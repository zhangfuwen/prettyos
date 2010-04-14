#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

task_t* initTaskQueue();
task_t* getReadyTask();
void setNextTask(task_t* task1, task_t* task2);
task_t* getLastTask();
void clearTask(task_t* task1);
void log_task_list();

#endif
