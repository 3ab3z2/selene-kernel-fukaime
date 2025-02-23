/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _M68K_PAGE_H
#define _M68K_PAGE_H

#include <linux/const.h>
#include <asm-generic/setup.h>
#include <asm-generic/page_offset.h>

/* PAGE_SHIFT determines the page size */
#if defined(CONFIG_SUN3) || defined(CONFIG_COLDFIRE)
#define PAGE_SHIFT	13
#else
#define PAGE_SHIFT	12
#endif
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))
#define PAGE_OFFSET	(PAGE_OFFSET_RAW)

#ifndef __ASSEMBLY__

/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd[16]; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;
typedef struct page *pgtable_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((&x)->pmd[0])
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) } )
#define __pmd(x)	((pmd_t) { { (x) }, })
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

extern unsigned long _rambase;
extern unsigned long _ramstart;
extern unsigned long _ramend;

#endif /* !__ASSEMBLY__ */

#ifdef CONFIG_MMU
#include <asm-generic/page_mm.h>
#else
#include <asm-generic/page_no.h>
#endif

#define __phys_to_pfn(paddr)	((unsigned long)((paddr) >> PAGE_SHIFT))
#define __pfn_to_phys(pfn)	PFN_PHYS(pfn)

#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | VM_EXEC | \
				 VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#include <asm-generic/getorder.h>

#endif /* _M68K_PAGE_H */
