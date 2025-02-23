/*
 * Memory subsystem initialization for Hexagon
 *
 * Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
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

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <asm-generic/atomic.h>
#include <linux/highmem.h>
#include <asm-generic/tlb.h>
#include <asm-generic/sections.h>
#include <asm-generic/vm_mmu.h>

/*
 * Define a startpg just past the end of the kernel image and a lastpg
 * that corresponds to the end of real or simulated platform memory.
 */
#define bootmem_startpg (PFN_UP(((unsigned long) _end) - PAGE_OFFSET + PHYS_OFFSET))

unsigned long bootmem_lastpg;	/*  Should be set by platform code  */
unsigned long __phys_offset;	/*  physical kernel offset >> 12  */

/*  Set as variable to limit PMD copies  */
int max_kernel_seg = 0x303;

/*  think this should be (page_size-1) the way it's used...*/
unsigned long zero_page_mask;

/*  indicate pfn's of high memory  */
unsigned long highstart_pfn, highend_pfn;

DEFINE_PER_CPU(struct mmu_gather, mmu_gathers);

/* Default cache attribute for newly created page tables */
unsigned long _dflt_cache_att = CACHEDEF;

/*
 * The current "generation" of kernel map, which should not roll
 * over until Hell freezes over.  Actual bound in years needs to be
 * calculated to confirm.
 */
DEFINE_SPINLOCK(kmap_gen_lock);

/*  checkpatch says don't init this to 0.  */
unsigned long long kmap_generation;

/*
 * mem_init - initializes memory
 *
 * Frees up bootmem
 * Fixes up more stuff for HIGHMEM
 * Calculates and displays memory available/used
 */
void __init mem_init(void)
{
	/*  No idea where this is actually declared.  Seems to evade LXR.  */
	free_all_bootmem();
	mem_init_print_info(NULL);

	/*
	 *  To-Do:  someone somewhere should wipe out the bootmem map
	 *  after we're done?
	 */

	/*
	 * This can be moved to some more virtual-memory-specific
	 * initialization hook at some point.  Set the init_mm
	 * descriptors "context" value to point to the initial
	 * kernel segment table's physical address.
	 */
	init_mm.context.ptbase = __pa(init_mm.pgd);
}

/*
 * free_initmem - frees memory used by stuff declared with __init
 *
 * Todo:  free pages between __init_begin and __init_end; possibly
 * some devtree related stuff as well.
 */
void __ref free_initmem(void)
{
}

/*
 * free_initrd_mem - frees...  initrd memory.
 * @start - start of init memory
 * @end - end of init memory
 *
 * Apparently has to be passed the address of the initrd memory.
 *
 * Wrapped by #ifdef CONFIG_BLKDEV_INITRD
 */
void free_initrd_mem(unsigned long start, unsigned long end)
{
}

void sync_icache_dcache(pte_t pte)
{
	unsigned long addr;
	struct page *page;

	page = pte_page(pte);
	addr = (unsigned long) page_address(page);

	__vmcache_idsync(addr, PAGE_SIZE);
}

/*
 * In order to set up page allocator "nodes",
 * somebody has to call free_area_init() for UMA.
 *
 * In this mode, we only have one pg_data_t
 * structure: contig_mem_data.
 */
void __init paging_init(void)
{
	unsigned long zones_sizes[MAX_NR_ZONES] = {0, };

	/*
	 *  This is not particularly well documented anywhere, but
	 *  give ZONE_NORMAL all the memory, including the big holes
	 *  left by the kernel+bootmem_map which are already left as reserved
	 *  in the bootmem_map; free_area_init should see those bits and
	 *  adjust accordingly.
	 */

	zones_sizes[ZONE_NORMAL] = max_low_pfn;

	free_area_init(zones_sizes);  /*  sets up the zonelists and mem_map  */

	/*
	 * Start of high memory area.  Will probably need something more
	 * fancy if we...  get more fancy.
	 */
	high_memory = (void *)((bootmem_lastpg + 1) << PAGE_SHIFT);
}

#ifndef DMA_RESERVE
#define DMA_RESERVE		(4)
#endif

#define DMA_CHUNKSIZE		(1<<22)
#define DMA_RESERVED_BYTES	(DMA_RESERVE * DMA_CHUNKSIZE)

/*
 * Pick out the memory size.  We look for mem=size,
 * where size is "size[KkMm]"
 */
static int __init early_mem(char *p)
{
	unsigned long size;
	char *endp;

	size = memparse(p, &endp);

	bootmem_lastpg = PFN_DOWN(size);

	return 0;
}
early_param("mem", early_mem);

size_t hexagon_coherent_pool_size = (size_t) (DMA_RESERVE << 22);

void __init setup_arch_memory(void)
{
	int bootmap_size;
	/*  XXX Todo: this probably should be cleaned up  */
	u32 *segtable = (u32 *) &swapper_pg_dir[0];
	u32 *segtable_end;

	/*
	 * Set up boot memory allocator
	 *
	 * The Gorman book also talks about these functions.
	 * This needs to change for highmem setups.
	 */

	/*  Prior to this, bootmem_lastpg is actually mem size  */
	bootmem_lastpg += ARCH_PFN_OFFSET;

	/* Memory size needs to be a multiple of 16M */
	bootmem_lastpg = PFN_DOWN((bootmem_lastpg << PAGE_SHIFT) &
		~((BIG_KERNEL_PAGE_SIZE) - 1));

	/*
	 * Reserve the top DMA_RESERVE bytes of RAM for DMA (uncached)
	 * memory allocation
	 */

	max_low_pfn = bootmem_lastpg - PFN_DOWN(DMA_RESERVED_BYTES);
	min_low_pfn = ARCH_PFN_OFFSET;
	bootmap_size =  init_bootmem_node(NODE_DATA(0), bootmem_startpg, min_low_pfn, max_low_pfn);

	printk(KERN_INFO "bootmem_startpg:  0x%08lx\n", bootmem_startpg);
	printk(KERN_INFO "bootmem_lastpg:  0x%08lx\n", bootmem_lastpg);
	printk(KERN_INFO "bootmap_size:  %d\n", bootmap_size);
	printk(KERN_INFO "min_low_pfn:  0x%08lx\n", min_low_pfn);
	printk(KERN_INFO "max_low_pfn:  0x%08lx\n", max_low_pfn);

	/*
	 * The default VM page tables (will be) populated with
	 * VA=PA+PAGE_OFFSET mapping.  We go in and invalidate entries
	 * higher than what we have memory for.
	 */

	/*  this is pointer arithmetic; each entry covers 4MB  */
	segtable = segtable + (PAGE_OFFSET >> 22);

	/*  this actually only goes to the end of the first gig  */
	segtable_end = segtable + (1<<(30-22));

	/*
	 * Move forward to the start of empty pages; take into account
	 * phys_offset shift.
	 */

	segtable += (bootmem_lastpg-ARCH_PFN_OFFSET)>>(22-PAGE_SHIFT);
	{
		int i;

		for (i = 1 ; i <= DMA_RESERVE ; i++)
			segtable[-i] = ((segtable[-i] & __HVM_PTE_PGMASK_4MB)
				| __HVM_PTE_R | __HVM_PTE_W | __HVM_PTE_X
				| __HEXAGON_C_UNC << 6
				| __HVM_PDE_S_4MB);
	}

	printk(KERN_INFO "clearing segtable from %p to %p\n", segtable,
		segtable_end);
	while (segtable < (segtable_end-8))
		*(segtable++) = __HVM_PDE_S_INVALID;
	/* stop the pointer at the device I/O 4MB page  */

	printk(KERN_INFO "segtable = %p (should be equal to _K_io_map)\n",
		segtable);

#if 0
	/*  Other half of the early device table from vm_init_segtable. */
	printk(KERN_INFO "&_K_init_devicetable = 0x%08x\n",
		(unsigned long) _K_init_devicetable-PAGE_OFFSET);
	*segtable = ((u32) (unsigned long) _K_init_devicetable-PAGE_OFFSET) |
		__HVM_PDE_S_4KB;
	printk(KERN_INFO "*segtable = 0x%08x\n", *segtable);
#endif

	/*
	 * Free all the memory that wasn't taken up by the bootmap, the DMA
	 * reserve, or kernel itself.
	 */
	free_bootmem(PFN_PHYS(bootmem_startpg) + bootmap_size,
		     PFN_PHYS(bootmem_lastpg - bootmem_startpg) - bootmap_size -
		     DMA_RESERVED_BYTES);

	/*
	 *  The bootmem allocator seemingly just lives to feed memory
	 *  to the paging system
	 */
	printk(KERN_INFO "PAGE_SIZE=%lu\n", PAGE_SIZE);
	paging_init();  /*  See Gorman Book, 2.3  */

	/*
	 *  At this point, the page allocator is kind of initialized, but
	 *  apparently no pages are available (just like with the bootmem
	 *  allocator), and need to be freed themselves via mem_init(),
	 *  which is called by start_kernel() later on in the process
	 */
}
