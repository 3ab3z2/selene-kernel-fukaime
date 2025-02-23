/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/threads.h>
#include <asm-generic/epapr_hcalls.h>
#include <asm-generic/reg.h>
#include <asm-generic/page.h>
#include <asm-generic/cputable.h>
#include <asm-generic/thread_info.h>
#include <asm-generic/ppc_asm.h>
#include <asm-generic/asm-compat.h>
#include <asm-generic/asm-offsets.h>
#include <asm-generic/export.h>

#ifndef CONFIG_PPC64
/* epapr_ev_idle() was derived from e500_idle() */
_GLOBAL(epapr_ev_idle)
	CURRENT_THREAD_INFO(r3, r1)
	PPC_LL	r4, TI_LOCAL_FLAGS(r3)	/* set napping bit */
	ori	r4, r4,_TLF_NAPPING	/* so when we take an exception */
	PPC_STL	r4, TI_LOCAL_FLAGS(r3)	/* it will return to our caller */

	wrteei	1

idle_loop:
	LOAD_REG_IMMEDIATE(r11, EV_HCALL_TOKEN(EV_IDLE))

.global epapr_ev_idle_start
epapr_ev_idle_start:
	li	r3, -1
	nop
	nop
	nop

	/*
	 * Guard against spurious wakeups from a hypervisor --
	 * only interrupt will cause us to return to LR due to
	 * _TLF_NAPPING.
	 */
	b	idle_loop
#endif

/* Hypercall entry point. Will be patched with device tree instructions. */
.global epapr_hypercall_start
epapr_hypercall_start:
	li	r3, -1
	nop
	nop
	nop
	blr
EXPORT_SYMBOL(epapr_hypercall_start)
