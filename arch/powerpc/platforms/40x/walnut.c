/*
 * Architecture- / platform-specific boot-time initialization code for
 * IBM PowerPC 4xx based boards. Adapted from original
 * code by Gary Thomas, Cort Dougan <cort@fsmlabs.com>, and Dan Malek
 * <dan@net4x.com>.
 *
 * Copyright(c) 1999-2000 Grant Erickson <grant@lcse.umn.edu>
 *
 * Rewritten and ported to the merged powerpc tree:
 * Copyright 2007 IBM Corporation
 * Josh Boyer <jwboyer@linux.vnet.ibm.com>
 *
 * 2002 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/init.h>
#include <linux/of_platform.h>
#include <linux/rtc.h>

#include <asm-generic/machdep.h>
#include <asm-generic/prom.h>
#include <asm-generic/udbg.h>
#include <asm-generic/time.h>
#include <asm-generic/uic.h>
#include <asm-generic/pci-bridge.h>
#include <asm-generic/ppc4xx.h>

static const struct of_device_id walnut_of_bus[] __initconst = {
	{ .compatible = "ibm,plb3", },
	{ .compatible = "ibm,opb", },
	{ .compatible = "ibm,ebc", },
	{},
};

static int __init walnut_device_probe(void)
{
	of_platform_bus_probe(NULL, walnut_of_bus, NULL);
	of_instantiate_rtc();

	return 0;
}
machine_device_initcall(walnut, walnut_device_probe);

static int __init walnut_probe(void)
{
	if (!of_machine_is_compatible("ibm,walnut"))
		return 0;

	pci_set_flags(PCI_REASSIGN_ALL_RSRC);

	return 1;
}

define_machine(walnut) {
	.name			= "Walnut",
	.probe			= walnut_probe,
	.progress		= udbg_progress,
	.init_IRQ		= uic_init_tree,
	.get_irq		= uic_get_irq,
	.restart		= ppc4xx_reset_system,
	.calibrate_decr		= generic_calibrate_decr,
};
