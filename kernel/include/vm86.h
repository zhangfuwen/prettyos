#ifndef VM86_H
#define VM86_H

// test from: http://osdev.berlios.de/v86.html

#include "os.h"

#define VALID_FLAGS         0xDFF

#define EFLAG_IF			0x00
#define USER_FLAT_CODE		0x00
#define USER_FLAT_DATA		0x00
#define USER_THREAD_INFO	0x00
#define EFLAG_VM			0x00

/* segment:offset pair */
typedef uintptr_t FARPTR;

/* Make a FARPTR from a segment and an offset */
#define MK_FP(seg, off)    ((FARPTR) (((uint32_t) (seg) << 16) | (uint16_t) (off)))

/* Extract the segment part of a FARPTR */
#define FP_SEG(fp)        (((FARPTR) fp) >> 16)

/* Extract the offset part of a FARPTR */
#define FP_OFF(fp)        (((FARPTR) fp) & 0xFFFF)

/* Convert a segment:offset pair to a linear address */
#define FP_TO_LINEAR(seg, off) ((void*) ((((uint16_t) (seg)) << 4) + ((uint16_t) (off))))

typedef struct context 
{
	uint32_t cs;
	uint32_t eip;
	uint32_t ss;
	uint32_t esp;
	uint32_t eflags;
	uint32_t ds;
	uint32_t es;
	uint32_t gs;
	uint32_t fs;
}context_t;


typedef struct context_v86 
{
	uint32_t cs;
	uint32_t eip;
	uint32_t ss;
	uint32_t esp;
	uint32_t eflags;
	uint32_t ds;
	uint32_t es;
	uint32_t gs;
	uint32_t fs;
}context_v86_t;

typedef struct current 
{
    uint32_t       v86_if;
    uint32_t       v86_in_handler;
    context_v86_t  v86_context;
    uint32_t       kernel_esp;
    uint32_t       user_stack_top;
    uint32_t       v86_handler;
}current_t;

bool i386V86Gpf(context_v86_t* ctx);

#endif
