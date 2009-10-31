#ifndef TASK_H
#define TASK_H

#include "os.h"
#include "paging.h"
#include "descriptor_tables.h"

#define KERNEL_STACK_SIZE 2048       // Use a 2kb kernel stack.

uint32_t initial_esp;

typedef struct task
{
    int32_t id;                           // Process ID.
    uint32_t esp, ebp;                   // Stack and base pointers.
    uint32_t eip;                        // Instruction pointer.
    uint32_t ss;
    page_directory_t* page_directory; // Page directory.
    uint32_t kernel_stack;               // Kernel stack location.
    struct task* next;                // The next task in a linked list.
} task_t;

void tasking_install();
uint32_t task_switch(uint32_t esp);
int32_t fork();
int32_t getpid();
task_t* create_task (void* entry, uint8_t privilege);
void switch_context();

void task_log(task_t* t);
void TSS_log(tss_entry_t* tss);
void log_task_list();

#endif
