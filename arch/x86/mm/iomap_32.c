/*
 * Copyright © 2008 Ingo Molnar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <asm-generic/iomap.h>
#include <asm-generic/pat.h>
#include <linux/export.h>
#include <linux/highmem.h>

static int is_io_mapping_possible(resource_size_t base, unsigned long size)
{
#if !defined(CONFIG_X86_PAE) && defined(CONFIG_PHYS_ADDR_T_64BIT)
	/* There is no way to map greater than 1 << 32 address without PAE */
	if (base + size > 0x100000000ULL)
		return 0;
#endif
	return 1;
}

int iomap_create_wc(resource_size_t base, unsigned long size, pgprot_t *prot)
{
	enum page_cache_mode pcm = _PAGE_CACHE_MODE_WC;
	int ret;

	if (!is_io_mapping_possible(base, size))
		return -EINVAL;

	ret = io_reserve_memtype(base, base + size, &pcm);
	if (ret)
		return ret;

	*prot = __pgprot(__PAGE_KERNEL | cachemode2protval(pcm));
	return 0;
}
EXPORT_SYMBOL_GPL(iomap_create_wc);

void iomap_free(resource_size_t base, unsigned long size)
{
	io_free_memtype(base, base + size);
}
EXPORT_SYMBOL_GPL(iomap_free);

void *kmap_atomic_prot_pfn(unsigned long pfn, pgprot_t prot)
{
	unsigned long vaddr;
	int idx, type;

	preempt_disable();
	pagefault_disable();

	type = kmap_atomic_idx_push();
	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
	set_pte(kmap_pte - idx, pfn_pte(pfn, prot));
	arch_flush_lazy_mmu_mode();

	return (void *)vaddr;
}

/*
 * Map 'pfn' using protections 'prot'
 */
void __iomem *
iomap_atomic_prot_pfn(unsigned long pfn, pgprot_t prot)
{
	/*
	 * For non-PAT systems, translate non-WB request to UC- just in
	 * case the caller set the PWT bit to prot directly without using
	 * pgprot_writecombine(). UC- translates to uncached if the MTRR
	 * is UC or WC. UC- gets the real intention, of the user, which is
	 * "WC if the MTRR is WC, UC if you can't do that."
	 */
	if (!pat_enabled() && pgprot2cachemode(prot) != _PAGE_CACHE_MODE_WB)
		prot = __pgprot(__PAGE_KERNEL |
				cachemode2protval(_PAGE_CACHE_MODE_UC_MINUS));

	return (void __force __iomem *) kmap_atomic_prot_pfn(pfn, prot);
}
EXPORT_SYMBOL_GPL(iomap_atomic_prot_pfn);

void
iounmap_atomic(void __iomem *kvaddr)
{
	unsigned long vaddr = (unsigned long) kvaddr & PAGE_MASK;

	if (vaddr >= __fix_to_virt(FIX_KMAP_END) &&
	    vaddr <= __fix_to_virt(FIX_KMAP_BEGIN)) {
		int idx, type;

		type = kmap_atomic_idx();
		idx = type + KM_TYPE_NR * smp_processor_id();

#ifdef CONFIG_DEBUG_HIGHMEM
		WARN_ON_ONCE(vaddr != __fix_to_virt(FIX_KMAP_BEGIN + idx));
#endif
		/*
		 * Force other mappings to Oops if they'll try to access this
		 * pte without first remap it.  Keeping stale mappings around
		 * is a bad idea also, in case the page changes cacheability
		 * attributes or becomes a protected page in a hypervisor.
		 */
		kpte_clear_flush(kmap_pte-idx, vaddr);
		kmap_atomic_idx_pop();
	}

	pagefault_enable();
	preempt_enable();
}
EXPORT_SYMBOL_GPL(iounmap_atomic);
