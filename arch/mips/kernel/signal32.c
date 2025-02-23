/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1991, 1992  Linus Torvalds
 * Copyright (C) 1994 - 2000, 2006  Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2016, Imagination Technologies Ltd.
 */
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/syscalls.h>

#include <asm-generic/compat.h>
#include <asm-generic/compat-signal.h>
#include <linux/uaccess.h>
#include <asm-generic/unistd.h>

#include "signal-common.h"

/* 32-bit compatibility types */

typedef unsigned int __sighandler32_t;
typedef void (*vfptr_t)(void);

/*
 * Atomically swap in the new signal mask, and wait for a signal.
 */

asmlinkage int sys32_sigsuspend(compat_sigset_t __user *uset)
{
	return compat_sys_rt_sigsuspend(uset, sizeof(compat_sigset_t));
}

SYSCALL_DEFINE3(32_sigaction, long, sig, const struct compat_sigaction __user *, act,
	struct compat_sigaction __user *, oact)
{
	struct k_sigaction new_ka, old_ka;
	int ret;
	int err = 0;

	if (act) {
		old_sigset_t mask;
		s32 handler;

		if (!access_ok(VERIFY_READ, act, sizeof(*act)))
			return -EFAULT;
		err |= __get_user(handler, &act->sa_handler);
		new_ka.sa.sa_handler = (void __user *)(s64)handler;
		err |= __get_user(new_ka.sa.sa_flags, &act->sa_flags);
		err |= __get_user(mask, &act->sa_mask.sig[0]);
		if (err)
			return -EFAULT;

		siginitset(&new_ka.sa.sa_mask, mask);
	}

	ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		if (!access_ok(VERIFY_WRITE, oact, sizeof(*oact)))
			return -EFAULT;
		err |= __put_user(old_ka.sa.sa_flags, &oact->sa_flags);
		err |= __put_user((u32)(u64)old_ka.sa.sa_handler,
				  &oact->sa_handler);
		err |= __put_user(old_ka.sa.sa_mask.sig[0], oact->sa_mask.sig);
		err |= __put_user(0, &oact->sa_mask.sig[1]);
		err |= __put_user(0, &oact->sa_mask.sig[2]);
		err |= __put_user(0, &oact->sa_mask.sig[3]);
		if (err)
			return -EFAULT;
	}

	return ret;
}

int copy_siginfo_to_user32(compat_siginfo_t __user *to, const siginfo_t *from)
{
	int err;

	if (!access_ok (VERIFY_WRITE, to, sizeof(compat_siginfo_t)))
		return -EFAULT;

	/* If you change siginfo_t structure, please be sure
	   this code is fixed accordingly.
	   It should never copy any pad contained in the structure
	   to avoid security leaks, but must copy the generic
	   3 ints plus the relevant union member.
	   This routine must convert siginfo from 64bit to 32bit as well
	   at the same time.  */
	err = __put_user(from->si_signo, &to->si_signo);
	err |= __put_user(from->si_errno, &to->si_errno);
	err |= __put_user(from->si_code, &to->si_code);
	if (from->si_code < 0)
		err |= __copy_to_user(&to->_sifields._pad, &from->_sifields._pad, SI_PAD_SIZE);
	else {
		switch (siginfo_layout(from->si_signo, from->si_code)) {
		case SIL_TIMER:
			err |= __put_user(from->si_tid, &to->si_tid);
			err |= __put_user(from->si_overrun, &to->si_overrun);
			err |= __put_user(from->si_int, &to->si_int);
			break;
		case SIL_CHLD:
			err |= __put_user(from->si_utime, &to->si_utime);
			err |= __put_user(from->si_stime, &to->si_stime);
			err |= __put_user(from->si_status, &to->si_status);
		case SIL_KILL:
			err |= __put_user(from->si_pid, &to->si_pid);
			err |= __put_user(from->si_uid, &to->si_uid);
			break;
		case SIL_FAULT:
			err |= __put_user((unsigned long)from->si_addr, &to->si_addr);
			break;
		case SIL_POLL:
			err |= __put_user(from->si_band, &to->si_band);
			err |= __put_user(from->si_fd, &to->si_fd);
			break;
		case SIL_RT:
			err |= __put_user(from->si_pid, &to->si_pid);
			err |= __put_user(from->si_uid, &to->si_uid);
			err |= __put_user(from->si_int, &to->si_int);
			break;
		case SIL_SYS:
			err |= __copy_to_user(&to->si_call_addr, &from->si_call_addr,
					      sizeof(compat_uptr_t));
			err |= __put_user(from->si_syscall, &to->si_syscall);
			err |= __put_user(from->si_arch, &to->si_arch);
			break;
		}
	}
	return err;
}

int copy_siginfo_from_user32(siginfo_t *to, compat_siginfo_t __user *from)
{
	if (copy_from_user(to, from, 3*sizeof(int)) ||
	    copy_from_user(to->_sifields._pad,
			   from->_sifields._pad, SI_PAD_SIZE32))
		return -EFAULT;

	return 0;
}
