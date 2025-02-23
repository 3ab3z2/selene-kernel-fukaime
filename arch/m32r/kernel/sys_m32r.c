// SPDX-License-Identifier: GPL-2.0
/*
 * linux/arch/m32r/kernel/sys_m32r.c
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on the Linux/M32R platform.
 *
 * Taken from i386 version.
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/file.h>
#include <linux/utsname.h>
#include <linux/ipc.h>

#include <linux/uaccess.h>
#include <asm-generic/cachectl.h>
#include <asm-generic/cacheflush.h>
#include <asm-generic/syscall.h>
#include <asm-generic/unistd.h>

/*
 * sys_tas() - test-and-set
 */
asmlinkage int sys_tas(int __user *addr)
{
	int oldval;

	if (!access_ok(VERIFY_WRITE, addr, sizeof (int)))
		return -EFAULT;

	/* atomic operation:
	 *   oldval = *addr; *addr = 1;
	 */
	__asm__ __volatile__ (
		DCACHE_CLEAR("%0", "r4", "%1")
		"	.fillinsn\n"
		"1:\n"
		"	lock	%0, @%1	    ->	unlock	%2, @%1\n"
		"2:\n"
		/* NOTE:
		 *   The m32r processor can accept interrupts only
		 *   at the 32-bit instruction boundary.
		 *   So, in the above code, the "unlock" instruction
		 *   can be executed continuously after the "lock"
		 *   instruction execution without any interruptions.
		 */
		".section .fixup,\"ax\"\n"
		"	.balign 4\n"
		"3:	ldi	%0, #%3\n"
		"	seth	r14, #high(2b)\n"
		"	or3	r14, r14, #low(2b)\n"
		"	jmp	r14\n"
		".previous\n"
		".section __ex_table,\"a\"\n"
		"	.balign 4\n"
		"	.long 1b,3b\n"
		".previous\n"
		: "=&r" (oldval)
		: "r" (addr), "r" (1), "i"(-EFAULT)
		: "r14", "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		  , "r4"
#endif /* CONFIG_CHIP_M32700_TS1 */
	);

	return oldval;
}

asmlinkage int sys_cacheflush(void *addr, int bytes, int cache)
{
	/* This should flush more selectively ...  */
	_flush_cache_all();
	return 0;
}

asmlinkage int sys_cachectl(char *addr, int nbytes, int op)
{
	/* Not implemented yet. */
	return -ENOSYS;
}
