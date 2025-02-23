/* SPDX-License-Identifier: GPL-2.0 */
#ifndef ___ASM_SPARC_PGALLOC_H
#define ___ASM_SPARC_PGALLOC_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-generic/pgalloc_64.h>
#else
#include <asm-generic/pgalloc_32.h>
#endif
#endif
