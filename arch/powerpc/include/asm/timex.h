/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_POWERPC_TIMEX_H
#define _ASM_POWERPC_TIMEX_H

#ifdef __KERNEL__

/*
 * PowerPC architecture timex specifications
 */

#include <asm-generic/cputable.h>
#include <asm-generic/reg.h>

#define CLOCK_TICK_RATE	1024000 /* Underlying HZ */

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
#ifdef __powerpc64__
	return mftb();
#else
	cycles_t ret;

	/*
	 * For the "cycle" counter we use the timebase lower half.
	 * Currently only used on SMP.
	 */

	ret = 0;

	__asm__ __volatile__(
#ifdef CONFIG_PPC_8xx
		"97:	mftb %0\n"
#else
		"97:	mfspr %0, %2\n"
#endif
		"99:\n"
		".section __ftr_fixup,\"a\"\n"
		".align 2\n"
		"98:\n"
		"	.long %1\n"
		"	.long 0\n"
		"	.long 97b-98b\n"
		"	.long 99b-98b\n"
		"	.long 0\n"
		"	.long 0\n"
		".previous"
		: "=r" (ret) : "i" (CPU_FTR_601), "i" (SPRN_TBRL));
	return ret;
#endif
}
#define get_cycles get_cycles

#endif	/* __KERNEL__ */
#endif	/* _ASM_POWERPC_TIMEX_H */
