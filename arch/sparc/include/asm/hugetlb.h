/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SPARC64_HUGETLB_H
#define _ASM_SPARC64_HUGETLB_H

#include <asm-generic/page.h>
#include <asm-generic/hugetlb.h>

#ifdef CONFIG_HUGETLB_PAGE
struct pud_huge_patch_entry {
	unsigned int addr;
	unsigned int insn;
};
extern struct pud_huge_patch_entry __pud_huge_patch, __pud_huge_patch_end;
#endif

void set_huge_pte_at(struct mm_struct *mm, unsigned long addr,
		     pte_t *ptep, pte_t pte);

pte_t huge_ptep_get_and_clear(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep);

static inline int is_hugepage_only_range(struct mm_struct *mm,
					 unsigned long addr,
					 unsigned long len) {
	return 0;
}

/*
 * If the arch doesn't supply something else, assume that hugepage
 * size aligned regions are ok without further preparation.
 */
static inline int prepare_hugepage_range(struct file *file,
			unsigned long addr, unsigned long len)
{
	struct hstate *h = hstate_file(file);

	if (len & ~huge_page_mask(h))
		return -EINVAL;
	if (addr & ~huge_page_mask(h))
		return -EINVAL;
	return 0;
}

static inline void huge_ptep_clear_flush(struct vm_area_struct *vma,
					 unsigned long addr, pte_t *ptep)
{
}

static inline int huge_pte_none(pte_t pte)
{
	return pte_none(pte);
}

static inline pte_t huge_pte_wrprotect(pte_t pte)
{
	return pte_wrprotect(pte);
}

static inline void huge_ptep_set_wrprotect(struct mm_struct *mm,
					   unsigned long addr, pte_t *ptep)
{
	pte_t old_pte = *ptep;
	set_huge_pte_at(mm, addr, ptep, pte_wrprotect(old_pte));
}

static inline int huge_ptep_set_access_flags(struct vm_area_struct *vma,
					     unsigned long addr, pte_t *ptep,
					     pte_t pte, int dirty)
{
	int changed = !pte_same(*ptep, pte);
	if (changed) {
		set_huge_pte_at(vma->vm_mm, addr, ptep, pte);
		flush_tlb_page(vma, addr);
	}
	return changed;
}

static inline pte_t huge_ptep_get(pte_t *ptep)
{
	return *ptep;
}

static inline void arch_clear_hugepage_flags(struct page *page)
{
}

void hugetlb_free_pgd_range(struct mmu_gather *tlb, unsigned long addr,
			    unsigned long end, unsigned long floor,
			    unsigned long ceiling);

#endif /* _ASM_SPARC64_HUGETLB_H */
