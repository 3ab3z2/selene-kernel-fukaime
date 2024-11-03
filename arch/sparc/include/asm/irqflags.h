/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_IRQFLAGS_H
#define ___ASM_SPARC_IRQFLAGS_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/irqflags_64.h>
#else
#include <asm-generic/irqflags_32.h>
#endif
#endif
