/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_MMU_H
#define ___ASM_SPARC_MMU_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/mmu_64.h>
#else
#include <asm-generic/mmu_32.h>
#endif
#endif
