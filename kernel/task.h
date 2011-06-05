#ifndef TASK_H
#define TASK_H

#include "video/console.h"
#include "paging.h"
#include "scheduler.h"

typedef enum
{
    TASK, THREAD, VM86
} taskType_t;

struct task
{
    taskType_t        type;           // Indicates whether it is a thread or a task
    uint32_t          pid;            // Process ID
    uint32_t          esp;            // Stack pointer
    uint32_t          eip;            // Instruction pointer
    uint32_t          ss;             // Stack segment
    pageDirectory_t*  pageDirectory;  // Page directory
    uint8_t           privilege;      // Access privilege
    uint8_t*          heap_top;       // User heap top
    void*             kernelStack;    // Kernel stack location
    void*             FPUptr;         // Pointer to FPU data
    void              (*entry)();     // Entry point, used to restart the task
    listHead_t*       threads;        // All threads owned by this tasks - deleted if this task is exited
    task_t*           parent;         // Task that created this thread (only used for threads)

    // User task specific program data
    void*             userProgAddr;
    uint32_t          userProgSize;

    // Information needed by scheduler
    uint16_t          priority; // Indicates how often this task get the CPU
    blocker_t         blocker;  // Object indicating reason and duration of blockade

    // Task specific graphical output settings
    bool              ownConsole; // This task has an own console
    console_t*        console;    // Console used by this task
    uint8_t           attrib;     // Color
};

extern volatile task_t* FPUTask; // fpu.c
extern task_t           kernelTask;
extern bool             task_switching;
extern volatile task_t* currentTask;

void     tasking_install();
task_t*  create_task (pageDirectory_t* directory, void(*entry)(), uint8_t privilege); // Creates task using kernels console
task_t*  create_ctask(pageDirectory_t* directory, void(*entry)(), uint8_t privilege, const char* consoleName); // Creates task with own console
task_t*  create_thread (void(*entry)()); // Creates thread using currentTasks console
task_t*  create_cthread(void(*entry)(), const char* consoleName); // Creates a thread with own console
task_t*  create_vm86_task(void(*entry)());
void     switch_context();
void     task_saveState(uint32_t esp);
uint32_t task_switch(task_t* task);
void     exit();
void     task_kill(uint32_t pid);
void     task_restart(uint32_t pid);
bool     waitForTask(task_t* blockingTask, uint32_t timeout); // Returns false in case of timeout. TODO: Can this function cause deadlocks?
int32_t  getpid();
void*    task_grow_userheap(uint32_t increase);
void     task_log(task_t* t);

#endif
