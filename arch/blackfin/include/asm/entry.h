/*
 * Copyright 2004-2009 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __BFIN_ENTRY_H
#define __BFIN_ENTRY_H

#include <asm-generic/setup.h>
#include <asm-generic/page.h>

#ifdef __ASSEMBLY__

#define	LFLUSH_I_AND_D	0x00000808
#define	LSIGTRAP	5

/*
 * NOTE!  The single-stepping code assumes that all interrupt handlers
 * start by saving SYSCFG on the stack with their first instruction.
 */

/* This one is used for exceptions, emulation, and NMI.  It doesn't push
   RETI and doesn't do cli.  */
#define SAVE_ALL_SYS		save_context_no_interrupts
/* This is used for all normal interrupts.  It saves a minimum of registers
   to the stack, loads the IRQ number, and jumps to common code.  */
#ifdef CONFIG_IPIPE
# define LOAD_IPIPE_IPEND \
	P0.l = lo(IPEND); \
	P0.h = hi(IPEND); \
	R1 = [P0];
#else
# define LOAD_IPIPE_IPEND
#endif

/*
 * Workaround for anomalies 05000283 and 05000315
 */
#if ANOMALY_05000283 || ANOMALY_05000315
# define ANOMALY_283_315_WORKAROUND(preg, dreg)		\
	cc = dreg == dreg;				\
	preg.h = HI(CHIPID);				\
	preg.l = LO(CHIPID);				\
	if cc jump 1f;					\
	dreg.l = W[preg];				\
1:
#else
# define ANOMALY_283_315_WORKAROUND(preg, dreg)
#endif /* ANOMALY_05000283 || ANOMALY_05000315 */

#ifndef CONFIG_EXACT_HWERR
/* As a debugging aid - we save IPEND when DEBUG_KERNEL is on,
 * otherwise it is a waste of cycles.
 */
# ifndef CONFIG_DEBUG_KERNEL
#define INTERRUPT_ENTRY(N)						\
    [--sp] = SYSCFG;							\
    [--sp] = P0;	/*orig_p0*/					\
    [--sp] = R0;	/*orig_r0*/					\
    [--sp] = (R7:0,P5:0);						\
    R0 = (N);								\
    LOAD_IPIPE_IPEND							\
    jump __common_int_entry;
# else /* CONFIG_DEBUG_KERNEL */
#define INTERRUPT_ENTRY(N)						\
    [--sp] = SYSCFG;							\
    [--sp] = P0;	/*orig_p0*/					\
    [--sp] = R0;	/*orig_r0*/					\
    [--sp] = (R7:0,P5:0);						\
    p0.l = lo(IPEND);							\
    p0.h = hi(IPEND);							\
    r1 = [p0];								\
    R0 = (N);								\
    LOAD_IPIPE_IPEND							\
    jump __common_int_entry;
# endif /* CONFIG_DEBUG_KERNEL */

/* For timer interrupts, we need to save IPEND, since the user_mode
 *macro accesses it to determine where to account time.
 */
#define TIMER_INTERRUPT_ENTRY(N)					\
    [--sp] = SYSCFG;							\
    [--sp] = P0;	/*orig_p0*/					\
    [--sp] = R0;	/*orig_r0*/					\
    [--sp] = (R7:0,P5:0);						\
    p0.l = lo(IPEND);							\
    p0.h = hi(IPEND);							\
    r1 = [p0];								\
    R0 = (N);								\
    jump __common_int_entry;
#else /* CONFIG_EXACT_HWERR is defined */

/* if we want hardware error to be exact, we need to do a SSYNC (which forces
 * read/writes to complete to the memory controllers), and check to see that
 * caused a pending HW error condition. If so, we assume it was caused by user
 * space, by setting the same interrupt that we are in (so it goes off again)
 * and context restore, and a RTI (without servicing anything). This should
 * cause the pending HWERR to fire, and when that is done, this interrupt will
 * be re-serviced properly.
 * As you can see by the code - we actually need to do two SSYNCS - one to
 * make sure the read/writes complete, and another to make sure the hardware
 * error is recognized by the core.
 *
 * The extra nop before the SSYNC is to make sure we work around 05000244,
 * since the 283/315 workaround includes a branch to the end
 */
#define INTERRUPT_ENTRY(N)						\
    [--sp] = SYSCFG;							\
    [--sp] = P0;	/*orig_p0*/					\
    [--sp] = R0;	/*orig_r0*/					\
    [--sp] = (R7:0,P5:0);						\
    R1 = ASTAT;								\
    ANOMALY_283_315_WORKAROUND(p0, r0)					\
    P0.L = LO(ILAT);							\
    P0.H = HI(ILAT);							\
    NOP;								\
    SSYNC;								\
    SSYNC;								\
    R0 = [P0];								\
    CC = BITTST(R0, EVT_IVHW_P);					\
    IF CC JUMP 1f;							\
    ASTAT = R1;								\
    p0.l = lo(IPEND);							\
    p0.h = hi(IPEND);							\
    r1 = [p0];								\
    R0 = (N);								\
    LOAD_IPIPE_IPEND							\
    jump __common_int_entry;						\
1:  ASTAT = R1;								\
    RAISE N;								\
    (R7:0, P5:0) = [SP++];						\
    SP += 0x8;								\
    SYSCFG = [SP++];							\
    CSYNC;								\
    RTI;

#define TIMER_INTERRUPT_ENTRY(N)					\
    [--sp] = SYSCFG;							\
    [--sp] = P0;	/*orig_p0*/					\
    [--sp] = R0;	/*orig_r0*/					\
    [--sp] = (R7:0,P5:0);						\
    R1 = ASTAT;								\
    ANOMALY_283_315_WORKAROUND(p0, r0)					\
    P0.L = LO(ILAT);							\
    P0.H = HI(ILAT);							\
    NOP;								\
    SSYNC;								\
    SSYNC;								\
    R0 = [P0];								\
    CC = BITTST(R0, EVT_IVHW_P);					\
    IF CC JUMP 1f;							\
    ASTAT = R1;								\
    p0.l = lo(IPEND);							\
    p0.h = hi(IPEND);							\
    r1 = [p0];								\
    R0 = (N);								\
    jump __common_int_entry;						\
1:  ASTAT = R1;								\
    RAISE N;								\
    (R7:0, P5:0) = [SP++];						\
    SP += 0x8;								\
    SYSCFG = [SP++];							\
    CSYNC;								\
    RTI;
#endif	/* CONFIG_EXACT_HWERR */

/* This one pushes RETI without using CLI.  Interrupts are enabled.  */
#define SAVE_CONTEXT_SYSCALL	save_context_syscall
#define SAVE_CONTEXT		save_context_with_interrupts
#define SAVE_CONTEXT_CPLB	save_context_cplb

#define RESTORE_ALL_SYS		restore_context_no_interrupts
#define RESTORE_CONTEXT		restore_context_with_interrupts
#define RESTORE_CONTEXT_CPLB	restore_context_cplb

#endif				/* __ASSEMBLY__ */
#endif				/* __BFIN_ENTRY_H */
