/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_BITOPS_H
#define ___ASM_SPARC_BITOPS_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/bitops_64.h>
#else
#include <asm-generic/bitops_32.h>
#endif
#endif
