/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_IO_H_
#define _CDI_IO_H_

/// #include <stdint.h> /// CDI-style
#include "os.h"         /// PrettyOS work-around

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t cdi_inw(uint16_t _port)
{
	return inportw(_port);
}

static inline uint8_t cdi_inb(uint16_t _port)
{
	return inportb(_port);
}

static inline uint32_t cdi_inl(uint16_t _port)
{
	return inportl(_port);
}



static inline void cdi_outw(uint16_t _port, uint16_t _data)
{
	outportw(_port, _data);
}

static inline void cdi_outb(uint16_t _port, uint8_t _data)
{
	outportb(_port, _data);
}

static inline void cdi_outl(uint16_t _port, uint32_t _data)
{
	outportl(_port, _data);
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

