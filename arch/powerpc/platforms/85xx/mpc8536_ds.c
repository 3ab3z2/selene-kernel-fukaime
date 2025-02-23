/*
 * MPC8536 DS Board Setup
 *
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>

#include <asm-generic/time.h>
#include <asm-generic/machdep.h>
#include <asm-generic/pci-bridge.h>
#include <mm/mmu_decl.h>
#include <asm-generic/prom.h>
#include <asm-generic/udbg.h>
#include <asm-generic/mpic.h>
#include <asm-generic/swiotlb.h>

#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

#include "mpc85xx.h"

void __init mpc8536_ds_pic_init(void)
{
	struct mpic *mpic = mpic_alloc(NULL, 0, MPIC_BIG_ENDIAN,
			0, 256, " OpenPIC  ");
	BUG_ON(mpic == NULL);
	mpic_init(mpic);
}

/*
 * Setup the architecture
 */
static void __init mpc8536_ds_setup_arch(void)
{
	if (ppc_md.progress)
		ppc_md.progress("mpc8536_ds_setup_arch()", 0);

	fsl_pci_assign_primary();

	swiotlb_detect_4g();

	printk("MPC8536 DS board from Freescale Semiconductor\n");
}

machine_arch_initcall(mpc8536_ds, mpc85xx_common_publish_devices);

machine_arch_initcall(mpc8536_ds, swiotlb_setup_bus_notifier);

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init mpc8536_ds_probe(void)
{
	return of_machine_is_compatible("fsl,mpc8536ds");
}

define_machine(mpc8536_ds) {
	.name			= "MPC8536 DS",
	.probe			= mpc8536_ds_probe,
	.setup_arch		= mpc8536_ds_setup_arch,
	.init_IRQ		= mpc8536_ds_pic_init,
#ifdef CONFIG_PCI
	.pcibios_fixup_bus	= fsl_pcibios_fixup_bus,
	.pcibios_fixup_phb      = fsl_pcibios_fixup_phb,
#endif
	.get_irq		= mpic_get_irq,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};
