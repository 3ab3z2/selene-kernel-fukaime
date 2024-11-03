/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_BARRIER_H
#define ___ASM_SPARC_BARRIER_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/barrier_64.h>
#else
#include <asm-generic/barrier_32.h>
#endif
#endif
