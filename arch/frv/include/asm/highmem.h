/* highmem.h: virtual kernel memory mappings for high memory
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 * - Derived from include/asm-i386/highmem.h
 *
 * See Documentation/frv/mmu-layout.txt for more information.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _ASM_HIGHMEM_H
#define _ASM_HIGHMEM_H

#ifdef __KERNEL__

#include <linux/init.h>
#include <linux/highmem.h>
#include <asm-generic/mem-layout.h>
#include <asm-generic/spr-regs.h>
#include <asm-generic/mb-regs.h>

#define NR_TLB_LINES		64	/* number of lines in the TLB */

#ifndef __ASSEMBLY__

#include <linux/interrupt.h>
#include <asm-generic/kmap_types.h>
#include <asm-generic/pgtable.h>

#ifdef CONFIG_DEBUG_HIGHMEM
#define HIGHMEM_DEBUG 1
#else
#define HIGHMEM_DEBUG 0
#endif

/* declarations for highmem.c */
extern unsigned long highstart_pfn, highend_pfn;

#define kmap_prot PAGE_KERNEL
#define kmap_pte ______kmap_pte_in_TLB
extern pte_t *pkmap_page_table;

#define flush_cache_kmaps()  do { } while (0)

/*
 * Right now we initialize only a single pte table. It can be extended
 * easily, subsequent pte tables have to be allocated in one physical
 * chunk of RAM.
 */
#define LAST_PKMAP	PTRS_PER_PTE
#define LAST_PKMAP_MASK	(LAST_PKMAP - 1)
#define PKMAP_NR(virt)	((virt - PKMAP_BASE) >> PAGE_SHIFT)
#define PKMAP_ADDR(nr)	(PKMAP_BASE + ((nr) << PAGE_SHIFT))

extern void *kmap_high(struct page *page);
extern void kunmap_high(struct page *page);

extern void *kmap(struct page *page);
extern void kunmap(struct page *page);

#endif /* !__ASSEMBLY__ */

/*
 * The use of kmap_atomic/kunmap_atomic is discouraged - kmap/kunmap
 * gives a more generic (and caching) interface. But kmap_atomic can
 * be used in IRQ contexts, so in some (very limited) cases we need
 * it.
 */
#define KMAP_ATOMIC_CACHE_DAMR		8

#ifndef __ASSEMBLY__

#define __kmap_atomic_primary(cached, paddr, ampr)						\
({												\
	unsigned long damlr, dampr;								\
												\
	dampr = paddr | xAMPRx_L | xAMPRx_M | xAMPRx_S | xAMPRx_SS_16Kb | xAMPRx_V;		\
												\
	if (!cached)										\
		asm volatile("movgs %0,dampr"#ampr :: "r"(dampr) : "memory");			\
	else											\
		/* cache flush page attachment point */						\
		asm volatile("movgs %0,iampr"#ampr"\n"						\
			     "movgs %0,dampr"#ampr"\n"						\
			     :: "r"(dampr) : "memory"						\
			     );									\
												\
	asm("movsg damlr"#ampr",%0" : "=r"(damlr));						\
												\
	/*printk("DAMR"#ampr": PRIM sl=%d L=%08lx P=%08lx\n", type, damlr, dampr);*/		\
												\
	(void *) damlr;										\
})

#define __kmap_atomic_secondary(slot, paddr)							  \
({												  \
	unsigned long damlr = KMAP_ATOMIC_SECONDARY_FRAME + (slot) * PAGE_SIZE;			  \
	unsigned long dampr = paddr | xAMPRx_L | xAMPRx_M | xAMPRx_S | xAMPRx_SS_16Kb | xAMPRx_V; \
												  \
	asm volatile("movgs %0,tplr \n"								  \
		     "movgs %1,tppr \n"								  \
		     "tlbpr %0,gr0,#2,#1"							  \
		     : : "r"(damlr), "r"(dampr) : "memory");					  \
												  \
	/*printk("TLB: SECN sl=%d L=%08lx P=%08lx\n", slot, damlr, dampr);*/			  \
												  \
	(void *) damlr;										  \
})

static inline void *kmap_atomic_primary(struct page *page)
{
	unsigned long paddr;

	pagefault_disable();
	paddr = page_to_phys(page);

        return __kmap_atomic_primary(1, paddr, 2);
}

#define __kunmap_atomic_primary(cached, ampr)				\
do {									\
	asm volatile("movgs gr0,dampr"#ampr"\n" ::: "memory");		\
	if (cached)							\
		asm volatile("movgs gr0,iampr"#ampr"\n" ::: "memory");	\
} while(0)

#define __kunmap_atomic_secondary(slot, vaddr)				\
do {									\
	asm volatile("tlbpr %0,gr0,#4,#1" : : "r"(vaddr) : "memory");	\
} while(0)

static inline void kunmap_atomic_primary(void *kvaddr)
{
        __kunmap_atomic_primary(1, 2);
	pagefault_enable();
}

void *kmap_atomic(struct page *page);
void __kunmap_atomic(void *kvaddr);

#endif /* !__ASSEMBLY__ */

#endif /* __KERNEL__ */

#endif /* _ASM_HIGHMEM_H */
