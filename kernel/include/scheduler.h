#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

task_t* scheduler_install();
task_t* scheduler_getNextTask();
void scheduler_deleteTask(task_t* task);
void scheduler_log();

task_t* getLastTask();
void setNextTask(task_t* task1, task_t* task2);

#endif
