#ifndef TASK_H
#define TASK_H

#include "os.h"
#include "console.h"
#include "paging.h"
#include "descriptor_tables.h"

#define KERNEL_STACK_SIZE 2048        // Use a 2kb kernel stack.

struct task
{
    bool threadFlag;                  // 0: process 1: thread
    console_t* console;               // Console used by this task
	bool ownConsole;
    uint32_t pid;                     // Process ID
    uint32_t esp;                     // Stack pointer
    uint32_t eip;                     // Instruction pointer
    uint32_t ss;                      // stack segment
    page_directory_t* page_directory; // Page directory
    uint8_t privilege;                // access privilege
    uint8_t* heap_top;                // user heap top
    void* kernel_stack;               // Kernel stack location
    uintptr_t FPU_ptr;                // pointer to FPU data
    struct task* next;                // The next task in a linked list
} __attribute__((packed));

typedef struct task task_t;

extern int32_t userTaskCounter;

extern console_t* current_console;

int32_t getUserTaskNumber();
void settaskflag(int32_t i);

void tasking_install();
uint32_t task_switch(uint32_t esp);
int32_t getpid();
task_t* create_task(page_directory_t* directory, void* entry, uint8_t privilege); // Creates task using kernels console
task_t* create_ctask(page_directory_t* directory, void* entry, uint8_t privilege, const char* consoleName); // Creates task with own console
task_t* create_thread(task_t* parentTask, void* entry); // Creates thread using parentTasks console
task_t* create_cthread(task_t* parentTask, void* entry, const char* consoleName); // Creates a thread with own console
void switch_context();
void exit();

void* task_grow_userheap( uint32_t increase );

void task_log(task_t* t);
void TSS_log(tss_entry_t* tss);

#endif
