#ifndef TASK_H
#define TASK_H

#include "video/console.h"
#include "paging.h"
#include "scheduler.h"
#include "events.h"


typedef enum
{
    PROCESS, THREAD, VM86
} taskType_t;

struct task
{
    taskType_t       type;           // Indicates whether it is a thread or a process
    uint32_t         pid;            // Process ID
    uint32_t         esp;            // Stack pointer
    uint32_t         ss;             // Stack segment
    pageDirectory_t* pageDirectory;  // Page directory
    uint8_t*         heap_top;       // User heap top
    void*            kernelStack;    // Kernel stack location
    void*            FPUptr;         // Pointer to FPU data
    list_t*          threads;        // All threads owned by this tasks - deleted if this task is exited
    task_t*          parent;         // Task that created this thread (only used for threads)
    event_queue_t*   eventQueue;     // 0 if no event handling enabled. Points to queue otherwise.
    uint8_t          privilege;      // Access privilege

    // Information needed by scheduler
    uint16_t         priority; // Indicates how often this task gets the CPU
    blocker_t        blocker;  // Object indicating reason and duration of blockade

    // Task specific graphical output settings
    console_t*       console; // Console used by this task
    uint8_t          attrib;  // Color

    // Information needed for cleanup
    bool    speaker;
    list_t* files;
    list_t* folders;
};


extern task_t* volatile FPUTask; // fpu.c
extern task_t           kernelTask;
extern bool             task_switching;
extern task_t*          currentTask;


void     tasking_install(void);
task_t*  create_task(taskType_t type, pageDirectory_t* directory, void(*entry)(void), uint8_t privilege, console_t* console, size_t argc, char* argv[]); // Creates a basic task
task_t*  create_thread (void(*entry)(void));                                                                                                     // Creates thread using currentTasks console
task_t*  create_cthread(void(*entry)(void), const char* consoleName);                                                                            // Creates a thread with own console
task_t*  create_process (pageDirectory_t* directory, void(*entry)(void), uint8_t privilege, size_t argc, char* argv[]);                          // Creates a process which uses currentTasks console
task_t*  create_cprocess(pageDirectory_t* directory, void(*entry)(void), uint8_t privilege, size_t argc, char* argv[], const char* consoleName); // Creates a process with an own console
task_t*  create_vm86_task(pageDirectory_t* pd, void(*entry)(void));                                                                              // Creates a task running in VM86 mode
void     switch_context(void);
void     task_saveState(uint32_t esp);
uint32_t task_switch(task_t* newTask);
void     exit(void);
void     kill(task_t* task);
void     task_kill(uint32_t pid);
bool     waitForTask(task_t* blockingTask, uint32_t timeout); // Returns false in case of timeout. TODO: Can this function cause deadlocks?
uint32_t getpid(void);
void*    task_grow_userheap(uint32_t increase);
void     task_log(task_t* t);


#endif
