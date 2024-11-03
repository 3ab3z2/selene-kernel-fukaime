/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_ATOMIC_H
#define ___ASM_SPARC_ATOMIC_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/atomic_64.h>
#else
#include <asm-generic/atomic_32.h>
#endif
#endif
