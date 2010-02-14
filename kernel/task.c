#include "task.h"
#include "paging.h"
#include "kheap.h"

// The currently running task.
volatile task_t* current_task;

// The start of the task linked list.
volatile task_t* ready_queue;

// Some externs are needed
extern tss_entry_t tss;
extern void irq_tail();
extern uint32_t read_eip();

uint32_t next_pid = 1; // The next available process ID.
int32_t getpid() { return current_task->id; }

static page_directory_t* const KERNEL_DIRECTORY = NULL;

void settaskflag(int32_t i)
{
    pODA->ts_flag = i;
}

int32_t getUserTaskNumber()
{
    return userTaskCounter;
}

void tasking_install()
{
    cli();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("1st_task: ");
    settextcolor(15,0);
    #endif
    ///

    current_task = ready_queue = (task_t*)malloc(sizeof(task_t),0); // first task (kernel task)
    current_task->id = next_pid++;
    current_task->esp = current_task->ebp = 0;
    current_task->eip = 0;
    current_task->page_directory = KERNEL_DIRECTORY;
    current_task->next = 0;

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("1st_ks: ");
    settextcolor(15,0);
    #endif
    ///

    current_task->kernel_stack = (uint32_t)malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    sti();

    userTaskCounter = 0;
}

task_t* create_task( page_directory_t* directory, void* entry, uint8_t privilege)
{
    cli();

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("cr_task: ");
    settextcolor(15,0);
    #endif
    ///

    task_t* new_task = (task_t*)malloc(sizeof(task_t),0);
    new_task->id  = next_pid++;
    new_task->page_directory = directory;

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("cr_task_ks: ");
    settextcolor(15,0);
    #endif
    ///

    new_task->kernel_stack = (uint32_t) malloc(KERNEL_STACK_SIZE,PAGESIZE)+KERNEL_STACK_SIZE;
    new_task->next = 0;

    task_t* tmp_task = (task_t*)ready_queue;
    while (tmp_task->next)
    {
        tmp_task = tmp_task->next;
    }
    tmp_task->next = new_task;

    uint32_t* kernel_stack = (uint32_t*) new_task->kernel_stack;

    uint32_t code_segment=0x08, data_segment=0x10;

///TEST///
    *(--kernel_stack) = 0x0;  // return address dummy
///TEST///

    if(privilege == 3)
    {
        // general information: Intel 3A Chapter 5.12
        *(--kernel_stack) = new_task->ss = 0x23;    // ss
        *(--kernel_stack) = new_task->kernel_stack; // esp
        code_segment = 0x1B; // 0x18|0x3=0x1B
    }

    *(--kernel_stack) = 0x0202; // eflags = interrupts activated and iopl = 0
    *(--kernel_stack) = code_segment; // cs
    *(--kernel_stack) = (uint32_t)entry; // eip
    *(--kernel_stack) = 0; // error code

    *(--kernel_stack) = 0; // interrupt nummer

    // general purpose registers w/o esp
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;

    if(privilege == 3) data_segment = 0x23; // 0x20|0x3=0x23

    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;
    *(--kernel_stack) = data_segment;



    //setup TSS
    tss.ss0   = 0x10;
    tss.esp0  = new_task->kernel_stack;
    tss.ss    = data_segment;


    //setup task_t
    new_task->ebp = 0xd00fc0de; // test value
    new_task->esp = (uint32_t)kernel_stack;
    new_task->eip = (uint32_t)irq_tail;
    new_task->ss  = data_segment;

    sti();
    return new_task;
}

uint32_t task_switch(uint32_t esp)
{
    if(!current_task) return esp;
    current_task->esp = esp;   // save esp

    // task switch
    current_task = current_task->next; // take the next task
    if(!current_task)
    {
        current_task = ready_queue;    // start at the beginning of the queue
    }

    // new_task
	paging_switch( current_task->page_directory );
	//tss.cr3 = ... TODO: Really unnecessary?
    tss.esp  = current_task->esp;
    tss.esp0 = (current_task->kernel_stack)+KERNEL_STACK_SIZE;
    tss.ebp  = current_task->ebp;
    tss.ss   = current_task->ss;

    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printformat("%d ",getpid());
    settextcolor(15,0);
    #endif

    return current_task->esp;  // return new task's esp
}

void switch_context() // switch to next task
{
    __asm__ volatile("int $0x7E");
}

void exit()
{
    cli();
    #ifdef _DIAGNOSIS_
    log_task_list();
    #endif

    // finish current task and free occupied heap
    void* pkernelstack = (void*) ( (uint32_t) current_task->kernel_stack - KERNEL_STACK_SIZE );
    void* ptask        = (void*) current_task;


    // delete task in linked list
    task_t* tmp_task = (task_t*)ready_queue;
    do
    {
        if(tmp_task->next == current_task)
        {
            tmp_task->next = current_task->next;
        }
        if(tmp_task->next)
        {
            tmp_task = tmp_task->next;
        }
    }
    while (tmp_task->next);
    // printformat("after delete task in linked list.\n");

    // free memory at heap
    free(pkernelstack);
    free(ptask);
    // printformat("pid: %d t: %x ks: %x <---heap freed.\n",getpid(),ptask,pkernelstack);

    #ifdef _DIAGNOSIS_
    log_task_list();
    printformat("exit finished.\n");
    #endif

    userTaskCounter--; // a user-program has been deleted
    sti();
    switch_context(); // switch to next task
}

void log_task_list()
{
    task_t* tmp_task = (task_t*)ready_queue;
    do
    {
        task_log(tmp_task);
        tmp_task = tmp_task->next;
    }
    while (tmp_task->next);
    task_log(tmp_task);
}

void task_log(task_t* t)
{
    settextcolor(5,0);
    printformat("\nid: %d ", t->id);               // Process ID
    printformat("ebp: %x ",t->ebp);              // Base pointer
    printformat("esp: %x ",t->esp);              // Stack pointer
    printformat("eip: %x ",t->eip);              // Instruction pointer
    printformat("PD: %x ", t->page_directory);   // Page directory.
    printformat("k_stack: %x ",t->kernel_stack); // Kernel stack location.
    printformat("next: %x\n",  t->next);         // The next task in a linked list.
    settextcolor(15,0);
}

void TSS_log(tss_entry_t* tss)
{
    settextcolor(6,0);
    //printformat("\nprev_tss: %x ", tss->prev_tss);
    printformat("esp0: %x ", tss->esp0);
    printformat("ss0: %x ", tss->ss0);
    //printformat("esp1: %x ", tss->esp1);
    //printformat("ss1: %x ", tss->ss1);
    //printformat("esp2: %x ", tss->esp2);
    //printformat("ss2: %x ", tss->ss2);
    printformat("cr3: %x ", tss->cr3);
    printformat("eip: %x ", tss->eip);
    printformat("eflags: %x ", tss->eflags);
    printformat("eax: %x ", tss->eax);
    printformat("ecx: %x ", tss->ecx);
    printformat("edx: %x ", tss->edx);
    printformat("ebx: %x ", tss->ebx);
    printformat("esp: %x ", tss->esp);
    printformat("ebp: %x ", tss->ebp);
    printformat("esi: %x ", tss->esi);
    printformat("edi: %x ", tss->edi);
    printformat("es: %x ", tss->es);
    printformat("cs: %x ", tss->cs);
    printformat("ss: %x ", tss->ss);
    printformat("ds: %x ", tss->ds);
    printformat("fs: %x ", tss->fs);
    printformat("gs: %x ", tss->gs);
    //printformat("ldt: %x ", tss->ldt);
    //printformat("trap: %x ", tss->trap); //only 0 or 1
    //printformat("iomap_base: %x ", tss->iomap_base);
}
