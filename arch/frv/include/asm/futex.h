/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_FUTEX_H
#define _ASM_FUTEX_H

#ifdef __KERNEL__

#include <linux/futex.h>
#include <asm-generic/errno.h>
#include <linux/uaccess.h>

extern int arch_futex_atomic_op_inuser(int op, int oparg, int *oval,
		u32 __user *uaddr);

static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	return -ENOSYS;
}

#endif
#endif
