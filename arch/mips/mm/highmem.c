// SPDX-License-Identifier: GPL-2.0
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <asm-generic/fixmap.h>
#include <asm-generic/tlbflush.h>

static pte_t *kmap_pte;

unsigned long highstart_pfn, highend_pfn;

void *kmap(struct page *page)
{
	void *addr;

	might_sleep();
	if (!PageHighMem(page))
		return page_address(page);
	addr = kmap_high(page);
	flush_tlb_one((unsigned long)addr);

	return addr;
}
EXPORT_SYMBOL(kmap);

void kunmap(struct page *page)
{
	BUG_ON(in_interrupt());
	if (!PageHighMem(page))
		return;
	kunmap_high(page);
}
EXPORT_SYMBOL(kunmap);

/*
 * kmap_atomic/kunmap_atomic is significantly faster than kmap/kunmap because
 * no global lock is needed and because the kmap code must perform a global TLB
 * invalidation when the kmap pool wraps.
 *
 * However when holding an atomic kmap is is not legal to sleep, so atomic
 * kmaps are appropriate for short, tight code paths only.
 */

void *kmap_atomic(struct page *page)
{
	unsigned long vaddr;
	int idx, type;

	preempt_disable();
	pagefault_disable();
	if (!PageHighMem(page))
		return page_address(page);

	type = kmap_atomic_idx_push();
	idx = type + KM_TYPE_NR*smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
	BUG_ON(!pte_none(*(kmap_pte - idx)));
#endif
	set_pte(kmap_pte-idx, mk_pte(page, PAGE_KERNEL));
	local_flush_tlb_one((unsigned long)vaddr);

	return (void*) vaddr;
}
EXPORT_SYMBOL(kmap_atomic);

void __kunmap_atomic(void *kvaddr)
{
	unsigned long vaddr = (unsigned long) kvaddr & PAGE_MASK;
	int type __maybe_unused;

	if (vaddr < FIXADDR_START) { // FIXME
		pagefault_enable();
		preempt_enable();
		return;
	}

	type = kmap_atomic_idx();
#ifdef CONFIG_DEBUG_HIGHMEM
	{
		int idx = type + KM_TYPE_NR * smp_processor_id();

		BUG_ON(vaddr != __fix_to_virt(FIX_KMAP_BEGIN + idx));

		/*
		 * force other mappings to Oops if they'll try to access
		 * this pte without first remap it
		 */
		pte_clear(&init_mm, vaddr, kmap_pte-idx);
		local_flush_tlb_one(vaddr);
	}
#endif
	kmap_atomic_idx_pop();
	pagefault_enable();
	preempt_enable();
}
EXPORT_SYMBOL(__kunmap_atomic);

/*
 * This is the same as kmap_atomic() but can map memory that doesn't
 * have a struct page associated with it.
 */
void *kmap_atomic_pfn(unsigned long pfn)
{
	unsigned long vaddr;
	int idx, type;

	preempt_disable();
	pagefault_disable();

	type = kmap_atomic_idx_push();
	idx = type + KM_TYPE_NR*smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
	set_pte(kmap_pte-idx, pfn_pte(pfn, PAGE_KERNEL));
	flush_tlb_one(vaddr);

	return (void*) vaddr;
}

void __init kmap_init(void)
{
	unsigned long kmap_vstart;

	/* cache the first kmap pte */
	kmap_vstart = __fix_to_virt(FIX_KMAP_BEGIN);
	kmap_pte = kmap_get_fixmap_pte(kmap_vstart);
}
