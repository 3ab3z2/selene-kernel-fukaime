// SPDX-License-Identifier: GPL-2.0
/* sys_sparc32.c: Conversion between 32bit and 64bit native syscalls.
 *
 * Copyright (C) 1997,1998 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 * Copyright (C) 1997, 2007 David S. Miller (davem@davemloft.net)
 *
 * These routines maintain argument size conversion between 32bit and 64bit
 * environment.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/capability.h>
#include <linux/fs.h> 
#include <linux/mm.h> 
#include <linux/file.h> 
#include <linux/signal.h>
#include <linux/resource.h>
#include <linux/times.h>
#include <linux/smp.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/uio.h>
#include <linux/nfs_fs.h>
#include <linux/quota.h>
#include <linux/poll.h>
#include <linux/personality.h>
#include <linux/stat.h>
#include <linux/filter.h>
#include <linux/highmem.h>
#include <linux/highuid.h>
#include <linux/mman.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/icmpv6.h>
#include <linux/syscalls.h>
#include <linux/sysctl.h>
#include <linux/binfmts.h>
#include <linux/dnotify.h>
#include <linux/security.h>
#include <linux/compat.h>
#include <linux/vfs.h>
#include <linux/ptrace.h>
#include <linux/slab.h>

#include <asm-generic/types.h>
#include <linux/uaccess.h>
#include <asm-generic/fpumacro.h>
#include <asm-generic/mmu_context.h>
#include <asm-generic/compat_signal.h>

#include "systbls.h"

asmlinkage long sys32_truncate64(const char __user * path, unsigned long high, unsigned long low)
{
	if ((int)high < 0)
		return -EINVAL;
	else
		return sys_truncate(path, (high << 32) | low);
}

asmlinkage long sys32_ftruncate64(unsigned int fd, unsigned long high, unsigned long low)
{
	if ((int)high < 0)
		return -EINVAL;
	else
		return sys_ftruncate(fd, (high << 32) | low);
}

static int cp_compat_stat64(struct kstat *stat,
			    struct compat_stat64 __user *statbuf)
{
	int err;

	err  = put_user(huge_encode_dev(stat->dev), &statbuf->st_dev);
	err |= put_user(stat->ino, &statbuf->st_ino);
	err |= put_user(stat->mode, &statbuf->st_mode);
	err |= put_user(stat->nlink, &statbuf->st_nlink);
	err |= put_user(from_kuid_munged(current_user_ns(), stat->uid), &statbuf->st_uid);
	err |= put_user(from_kgid_munged(current_user_ns(), stat->gid), &statbuf->st_gid);
	err |= put_user(huge_encode_dev(stat->rdev), &statbuf->st_rdev);
	err |= put_user(0, (unsigned long __user *) &statbuf->__pad3[0]);
	err |= put_user(stat->size, &statbuf->st_size);
	err |= put_user(stat->blksize, &statbuf->st_blksize);
	err |= put_user(0, (unsigned int __user *) &statbuf->__pad4[0]);
	err |= put_user(0, (unsigned int __user *) &statbuf->__pad4[4]);
	err |= put_user(stat->blocks, &statbuf->st_blocks);
	err |= put_user(stat->atime.tv_sec, &statbuf->st_atime);
	err |= put_user(stat->atime.tv_nsec, &statbuf->st_atime_nsec);
	err |= put_user(stat->mtime.tv_sec, &statbuf->st_mtime);
	err |= put_user(stat->mtime.tv_nsec, &statbuf->st_mtime_nsec);
	err |= put_user(stat->ctime.tv_sec, &statbuf->st_ctime);
	err |= put_user(stat->ctime.tv_nsec, &statbuf->st_ctime_nsec);
	err |= put_user(0, &statbuf->__unused4);
	err |= put_user(0, &statbuf->__unused5);

	return err;
}

asmlinkage long compat_sys_stat64(const char __user * filename,
		struct compat_stat64 __user *statbuf)
{
	struct kstat stat;
	int error = vfs_stat(filename, &stat);

	if (!error)
		error = cp_compat_stat64(&stat, statbuf);
	return error;
}

asmlinkage long compat_sys_lstat64(const char __user * filename,
		struct compat_stat64 __user *statbuf)
{
	struct kstat stat;
	int error = vfs_lstat(filename, &stat);

	if (!error)
		error = cp_compat_stat64(&stat, statbuf);
	return error;
}

asmlinkage long compat_sys_fstat64(unsigned int fd,
		struct compat_stat64 __user * statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_compat_stat64(&stat, statbuf);
	return error;
}

asmlinkage long compat_sys_fstatat64(unsigned int dfd,
		const char __user *filename,
		struct compat_stat64 __user * statbuf, int flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_compat_stat64(&stat, statbuf);
}

COMPAT_SYSCALL_DEFINE3(sparc_sigaction, int, sig,
			struct compat_old_sigaction __user *,act,
			struct compat_old_sigaction __user *,oact)
{
	WARN_ON_ONCE(sig >= 0);
	return compat_sys_sigaction(-sig, act, oact);
}

COMPAT_SYSCALL_DEFINE5(rt_sigaction, int, sig,
			struct compat_sigaction __user *,act,
			struct compat_sigaction __user *,oact,
			void __user *,restorer,
			compat_size_t,sigsetsize)
{
        struct k_sigaction new_ka, old_ka;
        int ret;
	compat_sigset_t set32;

        /* XXX: Don't preclude handling different sized sigset_t's.  */
        if (sigsetsize != sizeof(compat_sigset_t))
                return -EINVAL;

        if (act) {
		u32 u_handler, u_restorer;

		new_ka.ka_restorer = restorer;
		ret = get_user(u_handler, &act->sa_handler);
		new_ka.sa.sa_handler =  compat_ptr(u_handler);
		ret |= copy_from_user(&set32, &act->sa_mask, sizeof(compat_sigset_t));
		sigset_from_compat(&new_ka.sa.sa_mask, &set32);
		ret |= get_user(new_ka.sa.sa_flags, &act->sa_flags);
		ret |= get_user(u_restorer, &act->sa_restorer);
		new_ka.sa.sa_restorer = compat_ptr(u_restorer);
                if (ret)
                	return -EFAULT;
	}

	ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		sigset_to_compat(&set32, &old_ka.sa.sa_mask);
		ret = put_user(ptr_to_compat(old_ka.sa.sa_handler), &oact->sa_handler);
		ret |= copy_to_user(&oact->sa_mask, &set32, sizeof(compat_sigset_t));
		ret |= put_user(old_ka.sa.sa_flags, &oact->sa_flags);
		ret |= put_user(ptr_to_compat(old_ka.sa.sa_restorer), &oact->sa_restorer);
		if (ret)
			ret = -EFAULT;
        }

        return ret;
}

asmlinkage compat_ssize_t sys32_pread64(unsigned int fd,
					char __user *ubuf,
					compat_size_t count,
					unsigned long poshi,
					unsigned long poslo)
{
	return sys_pread64(fd, ubuf, count, (poshi << 32) | poslo);
}

asmlinkage compat_ssize_t sys32_pwrite64(unsigned int fd,
					 char __user *ubuf,
					 compat_size_t count,
					 unsigned long poshi,
					 unsigned long poslo)
{
	return sys_pwrite64(fd, ubuf, count, (poshi << 32) | poslo);
}

asmlinkage long compat_sys_readahead(int fd,
				     unsigned long offhi,
				     unsigned long offlo,
				     compat_size_t count)
{
	return sys_readahead(fd, (offhi << 32) | offlo, count);
}

long compat_sys_fadvise64(int fd,
			  unsigned long offhi,
			  unsigned long offlo,
			  compat_size_t len, int advice)
{
	return sys_fadvise64_64(fd, (offhi << 32) | offlo, len, advice);
}

long compat_sys_fadvise64_64(int fd,
			     unsigned long offhi, unsigned long offlo,
			     unsigned long lenhi, unsigned long lenlo,
			     int advice)
{
	return sys_fadvise64_64(fd,
				(offhi << 32) | offlo,
				(lenhi << 32) | lenlo,
				advice);
}

long sys32_sync_file_range(unsigned int fd, unsigned long off_high, unsigned long off_low, unsigned long nb_high, unsigned long nb_low, unsigned int flags)
{
	return sys_sync_file_range(fd,
				   (off_high << 32) | off_low,
				   (nb_high << 32) | nb_low,
				   flags);
}

asmlinkage long compat_sys_fallocate(int fd, int mode, u32 offhi, u32 offlo,
				     u32 lenhi, u32 lenlo)
{
	return sys_fallocate(fd, mode, ((loff_t)offhi << 32) | offlo,
			     ((loff_t)lenhi << 32) | lenlo);
}
