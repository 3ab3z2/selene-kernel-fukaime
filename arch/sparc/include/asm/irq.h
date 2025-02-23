/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_IRQ_H
#define ___ASM_SPARC_IRQ_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/irq_64.h>
#else
#include <asm-generic/irq_32.h>
#endif
#endif
