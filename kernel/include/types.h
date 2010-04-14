#ifndef TYPES_H
#define TYPES_H

typedef unsigned int         size_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t;
typedef unsigned char        uint8_t;
typedef signed long long     int64_t;
typedef signed long          int32_t;
typedef signed short         int16_t;
typedef signed char          int8_t;
typedef uint32_t             uintptr_t;

#define NULL (void*)0
#define bool _Bool
#define true   1
#define false  0
#define __bool_true_false_are_defined 1

typedef __builtin_va_list va_list;
#define va_start(ap, X)    __builtin_va_start(ap, X)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dest,src)  __builtin_va_copy(dest,src)


// typedefs for CDI
typedef struct {} FILE;     /// Dummy: Please fix it
typedef unsigned int dev_t; // Defined like in tyndur
typedef unsigned int uid_t; // Defined like in tyndur
typedef unsigned int gid_t; // Defined like in tyndur


typedef struct task task_t;
// This defines the operatings system common data area
typedef struct
{
    // CPU
    uint32_t CPU_Frequency_kHz;  // determined from rdtsc

    // RAM
    uint32_t Memory_Size;        // memory size in byte

    //tasking
    uint8_t  ts_flag;            // 0: taskswitch off  1: taskswitch on
    task_t* curTask;             // Address of currentTask
    task_t* TaskFPU;             // Address of Task using FPU

    // floppy disk               // array index is number of floppy drive (0,1,2,3)
    bool flpy_motor[4];          // 0: motor off  1: motor on
    bool flpy_ReadWriteFlag[4];  // 0: ready      1: busy (blocked)

    // EHCI
    uint32_t pciEHCInumber;      // pci device number
}__attribute__((packed)) oda_t;

// This defines what the stack looks like after an ISR was running
typedef struct
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
}__attribute__((packed)) registers_t;

#endif
