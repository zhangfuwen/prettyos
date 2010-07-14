#ifndef TASK_H
#define TASK_H

#include "console.h"
#include "paging.h"
#include "descriptor_tables.h"

#define KERNEL_STACK_SIZE 0x1000      // Use a 4 KB kernel stack

typedef enum {
    BL_NONE, BL_TIME, BL_SEMAPHORE, BL_INTERRUPT
} blockerType_t;

struct task
{
    bool              thread;         // Indicates whether its a thread or a task
    uint32_t          pid;            // Process ID
    uint32_t          esp;            // Stack pointer
    uint32_t          eip;            // Instruction pointer
    uint32_t          ss;             // stack segment
    page_directory_t* page_directory; // Page directory
    uint8_t           privilege;      // access privilege
    uint8_t*          heap_top;       // user heap top
    void*             kernel_stack;   // Kernel stack location
    uintptr_t         FPU_ptr;        // pointer to FPU data

    // Information needed by scheduler
    blockerType_t blockType; // Type of the blocking object
    void*         blockData; // Object storing the blocker (TIME as integer (wakeup-time), SEMAPHORE as semaphore_t*)

    // task specific graphical output settings
    bool       ownConsole; // This task has an own console
    console_t* console;    // Console used by this task
    uint8_t    attrib;     // Color
} __attribute__((packed));

typedef struct task task_t;

extern task_t* FPUTask; // fpu.c

extern bool task_switching;
extern task_t* currentTask;
extern console_t* current_console;

void tasking_install();
uint32_t task_switch(uint32_t esp);
int32_t getpid();
task_t* create_task(page_directory_t* directory, void* entry, uint8_t privilege); // Creates task using kernels console
task_t* create_ctask(page_directory_t* directory, void* entry, uint8_t privilege, const char* consoleName); // Creates task with own console
task_t* create_thread(void(*entry)()); // Creates thread using currentTasks console
task_t* create_cthread(void(*entry)(), const char* consoleName); // Creates a thread with own console
void switch_context();
void exit();

void* task_grow_userheap( uint32_t increase );

void task_log(task_t* t);
void TSS_log(tss_entry_t* tss);

task_t* create_vm86_task(page_directory_t* directory, void* entry);

#endif
