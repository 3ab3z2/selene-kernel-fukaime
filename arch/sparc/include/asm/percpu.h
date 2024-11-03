/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_PERCPU_H
#define ___ASM_SPARC_PERCPU_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/percpu_64.h>
#else
#include <asm-generic/percpu_32.h>
#endif
#endif
