// SPDX-License-Identifier: GPL-2.0
/* Glue code to lib/swiotlb.c */

#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/cache.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <asm-generic/swiotlb.h>
#include <asm-generic/dma.h>
#include <asm-generic/iommu.h>
#include <asm-generic/machvec.h>

int swiotlb __read_mostly;
EXPORT_SYMBOL(swiotlb);

static void *ia64_swiotlb_alloc_coherent(struct device *dev, size_t size,
					 dma_addr_t *dma_handle, gfp_t gfp,
					 unsigned long attrs)
{
	if (dev->coherent_dma_mask != DMA_BIT_MASK(64))
		gfp |= GFP_DMA;
	return swiotlb_alloc_coherent(dev, size, dma_handle, gfp);
}

static void ia64_swiotlb_free_coherent(struct device *dev, size_t size,
				       void *vaddr, dma_addr_t dma_addr,
				       unsigned long attrs)
{
	swiotlb_free_coherent(dev, size, vaddr, dma_addr);
}

const struct dma_map_ops swiotlb_dma_ops = {
	.alloc = ia64_swiotlb_alloc_coherent,
	.free = ia64_swiotlb_free_coherent,
	.map_page = swiotlb_map_page,
	.unmap_page = swiotlb_unmap_page,
	.map_sg = swiotlb_map_sg_attrs,
	.unmap_sg = swiotlb_unmap_sg_attrs,
	.sync_single_for_cpu = swiotlb_sync_single_for_cpu,
	.sync_single_for_device = swiotlb_sync_single_for_device,
	.sync_sg_for_cpu = swiotlb_sync_sg_for_cpu,
	.sync_sg_for_device = swiotlb_sync_sg_for_device,
	.dma_supported = swiotlb_dma_supported,
	.mapping_error = swiotlb_dma_mapping_error,
};

void __init swiotlb_dma_init(void)
{
	dma_ops = &swiotlb_dma_ops;
	swiotlb_init(1);
}

void __init pci_swiotlb_init(void)
{
	if (!iommu_detected) {
#ifdef CONFIG_IA64_GENERIC
		swiotlb = 1;
		printk(KERN_INFO "PCI-DMA: Re-initialize machine vector.\n");
		machvec_init("dig");
		swiotlb_init(1);
		dma_ops = &swiotlb_dma_ops;
#else
		panic("Unable to find Intel IOMMU");
#endif
	}
}
