/*
 * include/asm-xtensa/ptrace.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001 - 2005 Tensilica Inc.
 */
#ifndef _XTENSA_PTRACE_H
#define _XTENSA_PTRACE_H

#include <uapi/asm-generic/ptrace.h>

/*
 * Kernel stack
 *
 *		+-----------------------+  -------- STACK_SIZE
 *		|     register file     |  |
 *		+-----------------------+  |
 *		|    struct pt_regs     |  |
 *		+-----------------------+  | ------ PT_REGS_OFFSET
 * double	:  16 bytes spill area  :  |  ^
 * excetion	:- - - - - - - - - - - -:  |  |
 * frame	:    struct pt_regs     :  |  |
 *		:- - - - - - - - - - - -:  |  |
 *		|                       |  |  |
 *		|     memory stack      |  |  |
 *		|                       |  |  |
 *		~                       ~  ~  ~
 *		~                       ~  ~  ~
 *		|                       |  |  |
 *		|                       |  |  |
 *		+-----------------------+  |  | --- STACK_BIAS
 *		|  struct task_struct   |  |  |  ^
 *  current --> +-----------------------+  |  |  |
 *		|  struct thread_info   |  |  |  |
 *		+-----------------------+ --------
 */

#define KERNEL_STACK_SIZE (2 * PAGE_SIZE)

/*  Offsets for exception_handlers[] (3 x 64-entries x 4-byte tables). */

#define EXC_TABLE_KSTK		0x004	/* Kernel Stack */
#define EXC_TABLE_DOUBLE_SAVE	0x008	/* Double exception save area for a0 */
#define EXC_TABLE_FIXUP		0x00c	/* Fixup handler */
#define EXC_TABLE_PARAM		0x010	/* For passing a parameter to fixup */
#define EXC_TABLE_SYSCALL_SAVE	0x014	/* For fast syscall handler */
#define EXC_TABLE_FAST_USER	0x100	/* Fast user exception handler */
#define EXC_TABLE_FAST_KERNEL	0x200	/* Fast kernel exception handler */
#define EXC_TABLE_DEFAULT	0x300	/* Default C-Handler */
#define EXC_TABLE_SIZE		0x400

#ifndef __ASSEMBLY__

#include <asm-generic/coprocessor.h>

/*
 * This struct defines the way the registers are stored on the
 * kernel stack during a system call or other kernel entry.
 */
struct pt_regs {
	unsigned long pc;		/*   4 */
	unsigned long ps;		/*   8 */
	unsigned long depc;		/*  12 */
	unsigned long exccause;		/*  16 */
	unsigned long excvaddr;		/*  20 */
	unsigned long debugcause;	/*  24 */
	unsigned long wmask;		/*  28 */
	unsigned long lbeg;		/*  32 */
	unsigned long lend;		/*  36 */
	unsigned long lcount;		/*  40 */
	unsigned long sar;		/*  44 */
	unsigned long windowbase;	/*  48 */
	unsigned long windowstart;	/*  52 */
	unsigned long syscall;		/*  56 */
	unsigned long icountlevel;	/*  60 */
	unsigned long scompare1;	/*  64 */
	unsigned long threadptr;	/*  68 */

	/* Additional configurable registers that are used by the compiler. */
	xtregs_opt_t xtregs_opt;

	/* Make sure the areg field is 16 bytes aligned. */
	int align[0] __attribute__ ((aligned(16)));

	/* current register frame.
	 * Note: The ESF for kernel exceptions ends after 16 registers!
	 */
	unsigned long areg[16];
};

#include <variant/core.h>

# define arch_has_single_step()	(1)
# define task_pt_regs(tsk) ((struct pt_regs*) \
	(task_stack_page(tsk) + KERNEL_STACK_SIZE - (XCHAL_NUM_AREGS-16)*4) - 1)
# define user_mode(regs) (((regs)->ps & 0x00000020)!=0)
# define instruction_pointer(regs) ((regs)->pc)
# define return_pointer(regs) (MAKE_PC_FROM_RA((regs)->areg[0], \
					       (regs)->areg[1]))

# ifndef CONFIG_SMP
#  define profile_pc(regs) instruction_pointer(regs)
# else
#  define profile_pc(regs)						\
	({								\
		in_lock_functions(instruction_pointer(regs)) ?		\
		return_pointer(regs) : instruction_pointer(regs);	\
	})
# endif

#define user_stack_pointer(regs) ((regs)->areg[1])

#else	/* __ASSEMBLY__ */

# include <asm-generic/asm-offsets.h>
#define PT_REGS_OFFSET	  (KERNEL_STACK_SIZE - PT_USER_SIZE)

#endif	/* !__ASSEMBLY__ */

#endif	/* _XTENSA_PTRACE_H */
