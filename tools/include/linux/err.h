/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TOOLS_LINUX_ERR_H
#define __TOOLS_LINUX_ERR_H

#include <linux/compiler.h>
#include <linux/types.h>

#include <asm-generic/errno.h>

/*
 * Original kernel header comment:
 *
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a normal
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 *
 * Userspace note:
 * The same principle works for userspace, because 'error' pointers
 * fall down to the unused hole far from user space, as described
 * in Documentation/x86/x86_64/mm.txt for x86_64 arch:
 *
 * 0000000000000000 - 00007fffffffffff (=47 bits) user space, different per mm hole caused by [48:63] sign extension
 * ffffffffffe00000 - ffffffffffffffff (=2 MB) unused hole
 *
 * It should be the same case for other architectures, because
 * this code is used in generic kernel code.
 */
#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void * __must_check ERR_PTR(long error_)
{
	return (void *) error_;
}

static inline long __must_check PTR_ERR(__force const void *ptr)
{
	return (long) ptr;
}

static inline bool __must_check IS_ERR(__force const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline bool __must_check IS_ERR_OR_NULL(__force const void *ptr)
{
	return unlikely(!ptr) || IS_ERR_VALUE((unsigned long)ptr);
}

#endif /* _LINUX_ERR_H */
