/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_KDEBUG_H
#define ___ASM_SPARC_KDEBUG_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/kdebug_64.h>
#else
#include <asm-generic/kdebug_32.h>
#endif
#endif
