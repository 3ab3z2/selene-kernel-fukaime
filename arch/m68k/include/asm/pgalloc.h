/* SPDX-License-Identifier: GPL-2.0 */
#ifndef M68K_PGALLOC_H
#define M68K_PGALLOC_H

#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm-generic/setup.h>

#ifdef CONFIG_MMU
#include <asm-generic/virtconvert.h>
#if defined(CONFIG_COLDFIRE)
#include <asm-generic/mcf_pgalloc.h>
#elif defined(CONFIG_SUN3)
#include <asm-generic/sun3_pgalloc.h>
#else
#include <asm-generic/motorola_pgalloc.h>
#endif

extern void m68k_setup_node(int node);
#endif

#endif /* M68K_PGALLOC_H */
