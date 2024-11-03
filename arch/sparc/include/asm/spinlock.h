/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_SPINLOCK_H
#define ___ASM_SPARC_SPINLOCK_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/spinlock_64.h>
#else
#include <asm-generic/spinlock_32.h>
#endif
#endif
