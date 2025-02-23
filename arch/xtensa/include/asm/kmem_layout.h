/*
 * Kernel virtual memory layout definitions.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of
 * this archive for more details.
 *
 * Copyright (C) 2016 Cadence Design Systems Inc.
 */

#ifndef _XTENSA_KMEM_LAYOUT_H
#define _XTENSA_KMEM_LAYOUT_H

#include <asm-generic/types.h>

#ifdef CONFIG_MMU

/*
 * Fixed TLB translations in the processor.
 */

#define XCHAL_PAGE_TABLE_VADDR	__XTENSA_UL_CONST(0x80000000)
#define XCHAL_PAGE_TABLE_SIZE	__XTENSA_UL_CONST(0x00400000)

#if defined(CONFIG_XTENSA_KSEG_MMU_V2)

#define XCHAL_KSEG_CACHED_VADDR	__XTENSA_UL_CONST(0xd0000000)
#define XCHAL_KSEG_BYPASS_VADDR	__XTENSA_UL_CONST(0xd8000000)
#define XCHAL_KSEG_SIZE		__XTENSA_UL_CONST(0x08000000)
#define XCHAL_KSEG_ALIGNMENT	__XTENSA_UL_CONST(0x08000000)
#define XCHAL_KSEG_TLB_WAY	5
#define XCHAL_KIO_TLB_WAY	6

#elif defined(CONFIG_XTENSA_KSEG_256M)

#define XCHAL_KSEG_CACHED_VADDR	__XTENSA_UL_CONST(0xb0000000)
#define XCHAL_KSEG_BYPASS_VADDR	__XTENSA_UL_CONST(0xc0000000)
#define XCHAL_KSEG_SIZE		__XTENSA_UL_CONST(0x10000000)
#define XCHAL_KSEG_ALIGNMENT	__XTENSA_UL_CONST(0x10000000)
#define XCHAL_KSEG_TLB_WAY	6
#define XCHAL_KIO_TLB_WAY	6

#elif defined(CONFIG_XTENSA_KSEG_512M)

#define XCHAL_KSEG_CACHED_VADDR	__XTENSA_UL_CONST(0xa0000000)
#define XCHAL_KSEG_BYPASS_VADDR	__XTENSA_UL_CONST(0xc0000000)
#define XCHAL_KSEG_SIZE		__XTENSA_UL_CONST(0x20000000)
#define XCHAL_KSEG_ALIGNMENT	__XTENSA_UL_CONST(0x10000000)
#define XCHAL_KSEG_TLB_WAY	6
#define XCHAL_KIO_TLB_WAY	6

#else
#error Unsupported KSEG configuration
#endif

#ifdef CONFIG_KSEG_PADDR
#define XCHAL_KSEG_PADDR        __XTENSA_UL_CONST(CONFIG_KSEG_PADDR)
#else
#define XCHAL_KSEG_PADDR	__XTENSA_UL_CONST(0x00000000)
#endif

#if XCHAL_KSEG_PADDR & (XCHAL_KSEG_ALIGNMENT - 1)
#error XCHAL_KSEG_PADDR is not properly aligned to XCHAL_KSEG_ALIGNMENT
#endif

#else

#define XCHAL_KSEG_CACHED_VADDR	__XTENSA_UL_CONST(0xd0000000)
#define XCHAL_KSEG_BYPASS_VADDR	__XTENSA_UL_CONST(0xd8000000)
#define XCHAL_KSEG_SIZE		__XTENSA_UL_CONST(0x08000000)

#endif

#endif
