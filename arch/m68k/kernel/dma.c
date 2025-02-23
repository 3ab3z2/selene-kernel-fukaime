/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#undef DEBUG

#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/export.h>

#include <asm-generic/pgalloc.h>

#if defined(CONFIG_MMU) && !defined(CONFIG_COLDFIRE)

static void *m68k_dma_alloc(struct device *dev, size_t size, dma_addr_t *handle,
		gfp_t flag, unsigned long attrs)
{
	struct page *page, **map;
	pgprot_t pgprot;
	void *addr;
	int i, order;

	pr_debug("dma_alloc_coherent: %d,%x\n", size, flag);

	size = PAGE_ALIGN(size);
	order = get_order(size);

	page = alloc_pages(flag, order);
	if (!page)
		return NULL;

	*handle = page_to_phys(page);
	map = kmalloc(sizeof(struct page *) << order, flag & ~__GFP_DMA);
	if (!map) {
		__free_pages(page, order);
		return NULL;
	}
	split_page(page, order);

	order = 1 << order;
	size >>= PAGE_SHIFT;
	map[0] = page;
	for (i = 1; i < size; i++)
		map[i] = page + i;
	for (; i < order; i++)
		__free_page(page + i);
	pgprot = __pgprot(_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY);
	if (CPU_IS_040_OR_060)
		pgprot_val(pgprot) |= _PAGE_GLOBAL040 | _PAGE_NOCACHE_S;
	else
		pgprot_val(pgprot) |= _PAGE_NOCACHE030;
	addr = vmap(map, size, VM_MAP, pgprot);
	kfree(map);

	return addr;
}

static void m68k_dma_free(struct device *dev, size_t size, void *addr,
		dma_addr_t handle, unsigned long attrs)
{
	pr_debug("dma_free_coherent: %p, %x\n", addr, handle);
	vfree(addr);
}

#else

#include <asm-generic/cacheflush.h>

static void *m68k_dma_alloc(struct device *dev, size_t size,
		dma_addr_t *dma_handle, gfp_t gfp, unsigned long attrs)
{
	void *ret;
	/* ignore region specifiers */
	gfp &= ~(__GFP_DMA | __GFP_HIGHMEM);

	if (dev == NULL || (*dev->dma_mask < 0xffffffff))
		gfp |= GFP_DMA;
	ret = (void *)__get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
	}
	return ret;
}

static void m68k_dma_free(struct device *dev, size_t size, void *vaddr,
		dma_addr_t dma_handle, unsigned long attrs)
{
	free_pages((unsigned long)vaddr, get_order(size));
}

#endif /* CONFIG_MMU && !CONFIG_COLDFIRE */

static void m68k_dma_sync_single_for_device(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_BIDIRECTIONAL:
	case DMA_TO_DEVICE:
		cache_push(handle, size);
		break;
	case DMA_FROM_DEVICE:
		cache_clear(handle, size);
		break;
	default:
		pr_err_ratelimited("dma_sync_single_for_device: unsupported dir %u\n",
				   dir);
		break;
	}
}

static void m68k_dma_sync_sg_for_device(struct device *dev,
		struct scatterlist *sglist, int nents, enum dma_data_direction dir)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nents, i) {
		dma_sync_single_for_device(dev, sg->dma_address, sg->length,
					   dir);
	}
}

static dma_addr_t m68k_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs)
{
	dma_addr_t handle = page_to_phys(page) + offset;

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		dma_sync_single_for_device(dev, handle, size, dir);

	return handle;
}

static int m68k_dma_map_sg(struct device *dev, struct scatterlist *sglist,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nents, i) {
		sg->dma_address = sg_phys(sg);

		if (attrs & DMA_ATTR_SKIP_CPU_SYNC)
			continue;

		dma_sync_single_for_device(dev, sg->dma_address, sg->length,
					   dir);
	}
	return nents;
}

const struct dma_map_ops m68k_dma_ops = {
	.alloc			= m68k_dma_alloc,
	.free			= m68k_dma_free,
	.map_page		= m68k_dma_map_page,
	.map_sg			= m68k_dma_map_sg,
	.sync_single_for_device	= m68k_dma_sync_single_for_device,
	.sync_sg_for_device	= m68k_dma_sync_sg_for_device,
};
EXPORT_SYMBOL(m68k_dma_ops);
