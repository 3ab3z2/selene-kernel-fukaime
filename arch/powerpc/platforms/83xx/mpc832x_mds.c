/*
 * Copyright 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Description:
 * MPC832xE MDS board specific routines.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/root_dev.h>
#include <linux/initrd.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>

#include <linux/atomic.h>
#include <asm-generic/time.h>
#include <asm-generic/io.h>
#include <asm-generic/machdep.h>
#include <asm-generic/ipic.h>
#include <asm-generic/irq.h>
#include <asm-generic/prom.h>
#include <asm-generic/udbg.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>
#include <soc/fsl/qe/qe.h>
#include <soc/fsl/qe/qe_ic.h>

#include "mpc83xx.h"

#undef DEBUG
#ifdef DEBUG
#define DBG(fmt...) udbg_printf(fmt)
#else
#define DBG(fmt...)
#endif

/* ************************************************************************
 *
 * Setup the architecture
 *
 */
static void __init mpc832x_sys_setup_arch(void)
{
	struct device_node *np;
	u8 __iomem *bcsr_regs = NULL;

	mpc83xx_setup_arch();

	/* Map BCSR area */
	np = of_find_node_by_name(NULL, "bcsr");
	if (np) {
		struct resource res;

		of_address_to_resource(np, 0, &res);
		bcsr_regs = ioremap(res.start, resource_size(&res));
		of_node_put(np);
	}

#ifdef CONFIG_QUICC_ENGINE
	if ((np = of_find_node_by_name(NULL, "par_io")) != NULL) {
		par_io_init(np);
		of_node_put(np);

		for (np = NULL; (np = of_find_node_by_name(np, "ucc")) != NULL;)
			par_io_of_config(np);
	}

	if ((np = of_find_compatible_node(NULL, "network", "ucc_geth"))
			!= NULL){
		/* Reset the Ethernet PHYs */
#define BCSR8_FETH_RST 0x50
		clrbits8(&bcsr_regs[8], BCSR8_FETH_RST);
		udelay(1000);
		setbits8(&bcsr_regs[8], BCSR8_FETH_RST);
		iounmap(bcsr_regs);
		of_node_put(np);
	}
#endif				/* CONFIG_QUICC_ENGINE */
}

machine_device_initcall(mpc832x_mds, mpc83xx_declare_of_platform_devices);

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc832x_sys_probe(void)
{
	return of_machine_is_compatible("MPC832xMDS");
}

define_machine(mpc832x_mds) {
	.name 		= "MPC832x MDS",
	.probe 		= mpc832x_sys_probe,
	.setup_arch 	= mpc832x_sys_setup_arch,
	.init_IRQ	= mpc83xx_ipic_and_qe_init_IRQ,
	.get_irq 	= ipic_get_irq,
	.restart 	= mpc83xx_restart,
	.time_init 	= mpc83xx_time_init,
	.calibrate_decr	= generic_calibrate_decr,
	.progress 	= udbg_progress,
};
