/*
 *  PowerPC version
 *    Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org)
 *
 *  Modifications by Paul Mackerras (PowerMac) (paulus@cs.anu.edu.au)
 *  and Cort Dougan (PReP) (cort@cs.nmt.edu)
 *    Copyright (C) 1996 Paul Mackerras
 *  PPC44x/36-bit changes by Matt Porter (mporter@mvista.com)
 *
 *  Derived from "arch/i386/mm/init.c"
 *    Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/initrd.h>
#include <linux/pagemap.h>
#include <linux/memblock.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/hugetlb.h>

#include <asm-generic/pgalloc.h>
#include <asm-generic/prom.h>
#include <asm-generic/io.h>
#include <asm-generic/pgtable.h>
#include <asm-generic/mmu.h>
#include <asm-generic/smp.h>
#include <asm-generic/machdep.h>
#include <asm-generic/btext.h>
#include <asm-generic/tlb.h>
#include <asm-generic/sections.h>
#include <asm-generic/hugetlb.h>

#include "mmu_decl.h"

#if defined(CONFIG_KERNEL_START_BOOL) || defined(CONFIG_LOWMEM_SIZE_BOOL)
/* The amount of lowmem must be within 0xF0000000 - KERNELBASE. */
#if (CONFIG_LOWMEM_SIZE > (0xF0000000 - PAGE_OFFSET))
#error "You must adjust CONFIG_LOWMEM_SIZE or CONFIG_KERNEL_START"
#endif
#endif
#define MAX_LOW_MEM	CONFIG_LOWMEM_SIZE

phys_addr_t total_memory;
phys_addr_t total_lowmem;

phys_addr_t memstart_addr = (phys_addr_t)~0ull;
EXPORT_SYMBOL(memstart_addr);
phys_addr_t kernstart_addr;
EXPORT_SYMBOL(kernstart_addr);

#ifdef CONFIG_RELOCATABLE
/* Used in __va()/__pa() */
long long virt_phys_offset;
EXPORT_SYMBOL(virt_phys_offset);
#endif

phys_addr_t lowmem_end_addr;

int boot_mapsize;
#ifdef CONFIG_PPC_PMAC
unsigned long agp_special_page;
EXPORT_SYMBOL(agp_special_page);
#endif

void MMU_init(void);

/*
 * this tells the system to map all of ram with the segregs
 * (i.e. page tables) instead of the bats.
 * -- Cort
 */
int __map_without_bats;
int __map_without_ltlbs;

/*
 * This tells the system to allow ioremapping memory marked as reserved.
 */
int __allow_ioremap_reserved;

/* max amount of low RAM to map in */
unsigned long __max_low_memory = MAX_LOW_MEM;

/*
 * Check for command-line options that affect what MMU_init will do.
 */
void __init MMU_setup(void)
{
	/* Check for nobats option (used in mapin_ram). */
	if (strstr(boot_command_line, "nobats")) {
		__map_without_bats = 1;
	}

	if (strstr(boot_command_line, "noltlbs")) {
		__map_without_ltlbs = 1;
	}
	if (debug_pagealloc_enabled()) {
		__map_without_bats = 1;
		__map_without_ltlbs = 1;
	}
#ifdef CONFIG_STRICT_KERNEL_RWX
	if (rodata_enabled) {
		__map_without_bats = 1;
		__map_without_ltlbs = 1;
	}
#endif
}

/*
 * MMU_init sets up the basic memory mappings for the kernel,
 * including both RAM and possibly some I/O regions,
 * and sets up the page tables and the MMU hardware ready to go.
 */
void __init MMU_init(void)
{
	if (ppc_md.progress)
		ppc_md.progress("MMU:enter", 0x111);

	/* parse args from command line */
	MMU_setup();

	/*
	 * Reserve gigantic pages for hugetlb.  This MUST occur before
	 * lowmem_end_addr is initialized below.
	 */
	if (memblock.memory.cnt > 1) {
#ifndef CONFIG_WII
		memblock_enforce_memory_limit(memblock.memory.regions[0].size);
		pr_warn("Only using first contiguous memory region\n");
#else
		wii_memory_fixups();
#endif
	}

	total_lowmem = total_memory = memblock_end_of_DRAM() - memstart_addr;
	lowmem_end_addr = memstart_addr + total_lowmem;

#ifdef CONFIG_FSL_BOOKE
	/* Freescale Book-E parts expect lowmem to be mapped by fixed TLB
	 * entries, so we need to adjust lowmem to match the amount we can map
	 * in the fixed entries */
	adjust_total_lowmem();
#endif /* CONFIG_FSL_BOOKE */

	if (total_lowmem > __max_low_memory) {
		total_lowmem = __max_low_memory;
		lowmem_end_addr = memstart_addr + total_lowmem;
#ifndef CONFIG_HIGHMEM
		total_memory = total_lowmem;
		memblock_enforce_memory_limit(total_lowmem);
#endif /* CONFIG_HIGHMEM */
	}

	/* Initialize the MMU hardware */
	if (ppc_md.progress)
		ppc_md.progress("MMU:hw init", 0x300);
	MMU_init_hw();

	/* Map in all of RAM starting at KERNELBASE */
	if (ppc_md.progress)
		ppc_md.progress("MMU:mapin", 0x301);
	mapin_ram();

	/* Initialize early top-down ioremap allocator */
	ioremap_bot = IOREMAP_TOP;

	if (ppc_md.progress)
		ppc_md.progress("MMU:exit", 0x211);

	/* From now on, btext is no longer BAT mapped if it was at all */
#ifdef CONFIG_BOOTX_TEXT
	btext_unmap();
#endif

	/* Shortly after that, the entire linear mapping will be available */
	memblock_set_current_limit(lowmem_end_addr);
}
