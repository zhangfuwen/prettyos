#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

void    scheduler_install();
task_t* scheduler_getNextTask();
void    scheduler_insertTask(task_t* task);
void    scheduler_deleteTask(task_t* task);
void    scheduler_log();

#endif
