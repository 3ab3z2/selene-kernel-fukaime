/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_PGTABLE_H
#define ___ASM_SPARC_PGTABLE_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/pgtable_64.h>
#else
#include <asm-generic/pgtable_32.h>
#endif
#endif
