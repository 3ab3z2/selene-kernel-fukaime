/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_FLOPPY_H
#define ___ASM_SPARC_FLOPPY_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/floppy_64.h>
#else
#include <asm-generic/floppy_32.h>
#endif
#endif
