/*
 * Hexagon Virtual Machine TLB functions
 *
 * Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/*
 * The Hexagon Virtual Machine conceals the real workings of
 * the TLB, but there are one or two functions that need to
 * be instantiated for it, differently from a native build.
 */
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm-generic/page.h>
#include <asm-generic/hexagon_vm.h>

/*
 * Initial VM implementation has only one map active at a time, with
 * TLB purgings on changes.  So either we're nuking the current map,
 * or it's a no-op.  This operation is messy on true SMPs where other
 * processors must be induced to flush the copies in their local TLBs,
 * but Hexagon thread-based virtual processors share the same MMU.
 */
void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
			unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (mm->context.ptbase == current->active_mm->context.ptbase)
		__vmclrmap((void *)start, end - start);
}

/*
 * Flush a page from the kernel virtual map - used by highmem
 */
void flush_tlb_one(unsigned long vaddr)
{
	__vmclrmap((void *)vaddr, PAGE_SIZE);
}

/*
 * Flush all TLBs across all CPUs, virtual or real.
 * A single Hexagon core has 6 thread contexts but
 * only one TLB.
 */
void tlb_flush_all(void)
{
	/*  should probably use that fixaddr end or whateve label  */
	__vmclrmap(0, 0xffff0000);
}

/*
 * Flush TLB entries associated with a given mm_struct mapping.
 */
void flush_tlb_mm(struct mm_struct *mm)
{
	/* Current Virtual Machine has only one map active at a time */
	if (current->active_mm->context.ptbase == mm->context.ptbase)
		tlb_flush_all();
}

/*
 * Flush TLB state associated with a page of a vma.
 */
void flush_tlb_page(struct vm_area_struct *vma, unsigned long vaddr)
{
	struct mm_struct *mm = vma->vm_mm;

	if (mm->context.ptbase  == current->active_mm->context.ptbase)
		__vmclrmap((void *)vaddr, PAGE_SIZE);
}

/*
 * Flush TLB entries associated with a kernel address range.
 * Like flush range, but without the check on the vma->vm_mm.
 */
void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
		__vmclrmap((void *)start, end - start);
}
