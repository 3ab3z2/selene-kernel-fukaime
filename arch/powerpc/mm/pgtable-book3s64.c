/*
 * Copyright 2015-2016, Aneesh Kumar K.V, IBM Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/sched.h>
#include <linux/mm_types.h>
#include <misc/cxl-base.h>

#include <asm-generic/pgalloc.h>
#include <asm-generic/tlb.h>

#include "mmu_decl.h"
#include <trace/events/thp.h>

int (*register_process_table)(unsigned long base, unsigned long page_size,
			      unsigned long tbl_size);

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
/*
 * This is called when relaxing access to a hugepage. It's also called in the page
 * fault path when we don't hit any of the major fault cases, ie, a minor
 * update of _PAGE_ACCESSED, _PAGE_DIRTY, etc... The generic code will have
 * handled those two for us, we additionally deal with missing execute
 * permission here on some processors
 */
int pmdp_set_access_flags(struct vm_area_struct *vma, unsigned long address,
			  pmd_t *pmdp, pmd_t entry, int dirty)
{
	int changed;
#ifdef CONFIG_DEBUG_VM
	WARN_ON(!pmd_trans_huge(*pmdp) && !pmd_devmap(*pmdp));
	assert_spin_locked(&vma->vm_mm->page_table_lock);
#endif
	changed = !pmd_same(*(pmdp), entry);
	if (changed) {
		__ptep_set_access_flags(vma->vm_mm, pmdp_ptep(pmdp),
					pmd_pte(entry), address);
		flush_pmd_tlb_range(vma, address, address + HPAGE_PMD_SIZE);
	}
	return changed;
}

int pmdp_test_and_clear_young(struct vm_area_struct *vma,
			      unsigned long address, pmd_t *pmdp)
{
	return __pmdp_test_and_clear_young(vma->vm_mm, address, pmdp);
}
/*
 * set a new huge pmd. We should not be called for updating
 * an existing pmd entry. That should go via pmd_hugepage_update.
 */
void set_pmd_at(struct mm_struct *mm, unsigned long addr,
		pmd_t *pmdp, pmd_t pmd)
{
#ifdef CONFIG_DEBUG_VM
	WARN_ON(pte_present(pmd_pte(*pmdp)) && !pte_protnone(pmd_pte(*pmdp)));
	assert_spin_locked(&mm->page_table_lock);
	WARN_ON(!(pmd_trans_huge(pmd) || pmd_devmap(pmd)));
#endif
	trace_hugepage_set_pmd(addr, pmd_val(pmd));
	return set_pte_at(mm, addr, pmdp_ptep(pmdp), pmd_pte(pmd));
}

static void do_nothing(void *unused)
{

}
/*
 * Serialize against find_current_mm_pte which does lock-less
 * lookup in page tables with local interrupts disabled. For huge pages
 * it casts pmd_t to pte_t. Since format of pte_t is different from
 * pmd_t we want to prevent transit from pmd pointing to page table
 * to pmd pointing to huge page (and back) while interrupts are disabled.
 * We clear pmd to possibly replace it with page table pointer in
 * different code paths. So make sure we wait for the parallel
 * find_current_mm_pte to finish.
 */
void serialize_against_pte_lookup(struct mm_struct *mm)
{
	smp_mb();
	smp_call_function_many(mm_cpumask(mm), do_nothing, NULL, 1);
}

/*
 * We use this to invalidate a pmdp entry before switching from a
 * hugepte to regular pmd entry.
 */
void pmdp_invalidate(struct vm_area_struct *vma, unsigned long address,
		     pmd_t *pmdp)
{
	pmd_hugepage_update(vma->vm_mm, address, pmdp, _PAGE_PRESENT, 0);
	flush_pmd_tlb_range(vma, address, address + HPAGE_PMD_SIZE);
	/*
	 * This ensures that generic code that rely on IRQ disabling
	 * to prevent a parallel THP split work as expected.
	 */
	serialize_against_pte_lookup(vma->vm_mm);
}

static pmd_t pmd_set_protbits(pmd_t pmd, pgprot_t pgprot)
{
	return __pmd(pmd_val(pmd) | pgprot_val(pgprot));
}

pmd_t pfn_pmd(unsigned long pfn, pgprot_t pgprot)
{
	unsigned long pmdv;

	pmdv = (pfn << PAGE_SHIFT) & PTE_RPN_MASK;
	return pmd_set_protbits(__pmd(pmdv), pgprot);
}

pmd_t mk_pmd(struct page *page, pgprot_t pgprot)
{
	return pfn_pmd(page_to_pfn(page), pgprot);
}

pmd_t pmd_modify(pmd_t pmd, pgprot_t newprot)
{
	unsigned long pmdv;

	pmdv = pmd_val(pmd);
	pmdv &= _HPAGE_CHG_MASK;
	return pmd_set_protbits(__pmd(pmdv), newprot);
}

/*
 * This is called at the end of handling a user page fault, when the
 * fault has been handled by updating a HUGE PMD entry in the linux page tables.
 * We use it to preload an HPTE into the hash table corresponding to
 * the updated linux HUGE PMD entry.
 */
void update_mmu_cache_pmd(struct vm_area_struct *vma, unsigned long addr,
			  pmd_t *pmd)
{
	return;
}
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */

/* For use by kexec */
void mmu_cleanup_all(void)
{
	if (radix_enabled())
		radix__mmu_cleanup_all();
	else if (mmu_hash_ops.hpte_clear_all)
		mmu_hash_ops.hpte_clear_all();
}

#ifdef CONFIG_MEMORY_HOTPLUG
int create_section_mapping(unsigned long start, unsigned long end)
{
	if (radix_enabled())
		return radix__create_section_mapping(start, end);

	return hash__create_section_mapping(start, end);
}

int remove_section_mapping(unsigned long start, unsigned long end)
{
	if (radix_enabled())
		return radix__remove_section_mapping(start, end);

	return hash__remove_section_mapping(start, end);
}
#endif /* CONFIG_MEMORY_HOTPLUG */
