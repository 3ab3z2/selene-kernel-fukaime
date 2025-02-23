/*
 *  Atheros AR71XX/AR724X/AR913X specific setup
 *
 *  Copyright (C) 2010-2011 Jaiganesh Narayanan <jnarayanan@atheros.com>
 *  Copyright (C) 2008-2011 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Parts of this file are based on Atheros' 2.6.15/2.6.31 BSP
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_fdt.h>

#include <asm-generic/bootinfo.h>
#include <asm-generic/idle.h>
#include <asm-generic/time.h>		/* for mips_hpt_frequency */
#include <asm-generic/reboot.h>		/* for _machine_{restart,halt} */
#include <asm-generic/mips_machine.h>
#include <asm-generic/prom.h>
#include <asm-generic/fw/fw.h>

#include <asm-generic/mach-ath79/ath79.h>
#include <asm-generic/mach-ath79/ar71xx_regs.h>
#include "common.h"
#include "dev-common.h"
#include "machtypes.h"

#define ATH79_SYS_TYPE_LEN	64

static char ath79_sys_type[ATH79_SYS_TYPE_LEN];

static void ath79_restart(char *command)
{
	local_irq_disable();
	ath79_device_reset_set(AR71XX_RESET_FULL_CHIP);
	for (;;)
		if (cpu_wait)
			cpu_wait();
}

static void ath79_halt(void)
{
	while (1)
		cpu_wait();
}

static void __init ath79_detect_sys_type(void)
{
	char *chip = "????";
	u32 id;
	u32 major;
	u32 minor;
	u32 rev = 0;

	id = ath79_reset_rr(AR71XX_RESET_REG_REV_ID);
	major = id & REV_ID_MAJOR_MASK;

	switch (major) {
	case REV_ID_MAJOR_AR71XX:
		minor = id & AR71XX_REV_ID_MINOR_MASK;
		rev = id >> AR71XX_REV_ID_REVISION_SHIFT;
		rev &= AR71XX_REV_ID_REVISION_MASK;
		switch (minor) {
		case AR71XX_REV_ID_MINOR_AR7130:
			ath79_soc = ATH79_SOC_AR7130;
			chip = "7130";
			break;

		case AR71XX_REV_ID_MINOR_AR7141:
			ath79_soc = ATH79_SOC_AR7141;
			chip = "7141";
			break;

		case AR71XX_REV_ID_MINOR_AR7161:
			ath79_soc = ATH79_SOC_AR7161;
			chip = "7161";
			break;
		}
		break;

	case REV_ID_MAJOR_AR7240:
		ath79_soc = ATH79_SOC_AR7240;
		chip = "7240";
		rev = id & AR724X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR7241:
		ath79_soc = ATH79_SOC_AR7241;
		chip = "7241";
		rev = id & AR724X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR7242:
		ath79_soc = ATH79_SOC_AR7242;
		chip = "7242";
		rev = id & AR724X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR913X:
		minor = id & AR913X_REV_ID_MINOR_MASK;
		rev = id >> AR913X_REV_ID_REVISION_SHIFT;
		rev &= AR913X_REV_ID_REVISION_MASK;
		switch (minor) {
		case AR913X_REV_ID_MINOR_AR9130:
			ath79_soc = ATH79_SOC_AR9130;
			chip = "9130";
			break;

		case AR913X_REV_ID_MINOR_AR9132:
			ath79_soc = ATH79_SOC_AR9132;
			chip = "9132";
			break;
		}
		break;

	case REV_ID_MAJOR_AR9330:
		ath79_soc = ATH79_SOC_AR9330;
		chip = "9330";
		rev = id & AR933X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR9331:
		ath79_soc = ATH79_SOC_AR9331;
		chip = "9331";
		rev = id & AR933X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR9341:
		ath79_soc = ATH79_SOC_AR9341;
		chip = "9341";
		rev = id & AR934X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR9342:
		ath79_soc = ATH79_SOC_AR9342;
		chip = "9342";
		rev = id & AR934X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_AR9344:
		ath79_soc = ATH79_SOC_AR9344;
		chip = "9344";
		rev = id & AR934X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_QCA9556:
		ath79_soc = ATH79_SOC_QCA9556;
		chip = "9556";
		rev = id & QCA955X_REV_ID_REVISION_MASK;
		break;

	case REV_ID_MAJOR_QCA9558:
		ath79_soc = ATH79_SOC_QCA9558;
		chip = "9558";
		rev = id & QCA955X_REV_ID_REVISION_MASK;
		break;

	default:
		panic("ath79: unknown SoC, id:0x%08x", id);
	}

	ath79_soc_rev = rev;

	if (soc_is_qca955x())
		sprintf(ath79_sys_type, "Qualcomm Atheros QCA%s rev %u",
			chip, rev);
	else
		sprintf(ath79_sys_type, "Atheros AR%s rev %u", chip, rev);
	pr_info("SoC: %s\n", ath79_sys_type);
}

const char *get_system_type(void)
{
	return ath79_sys_type;
}

int get_c0_perfcount_int(void)
{
	return ATH79_MISC_IRQ(5);
}
EXPORT_SYMBOL_GPL(get_c0_perfcount_int);

unsigned int get_c0_compare_int(void)
{
	return CP0_LEGACY_COMPARE_IRQ;
}

void __init plat_mem_setup(void)
{
	unsigned long fdt_start;

	set_io_port_base(KSEG1);

	/* Get the position of the FDT passed by the bootloader */
	fdt_start = fw_getenvl("fdt_start");
	if (fdt_start)
		__dt_setup_arch((void *)KSEG0ADDR(fdt_start));
	else if (fw_passed_dtb)
		__dt_setup_arch((void *)KSEG0ADDR(fw_passed_dtb));

	if (mips_machtype != ATH79_MACH_GENERIC_OF) {
		ath79_reset_base = ioremap_nocache(AR71XX_RESET_BASE,
						   AR71XX_RESET_SIZE);
		ath79_pll_base = ioremap_nocache(AR71XX_PLL_BASE,
						 AR71XX_PLL_SIZE);
		ath79_detect_sys_type();
		ath79_ddr_ctrl_init();

		detect_memory_region(0, ATH79_MEM_SIZE_MIN, ATH79_MEM_SIZE_MAX);

		/* OF machines should use the reset driver */
		_machine_restart = ath79_restart;
	}

	_machine_halt = ath79_halt;
	pm_power_off = ath79_halt;
}

static void __init ath79_of_plat_time_init(void)
{
	struct device_node *np;
	struct clk *clk;
	unsigned long cpu_clk_rate;

	of_clk_init(NULL);

	np = of_get_cpu_node(0, NULL);
	if (!np) {
		pr_err("Failed to get CPU node\n");
		return;
	}

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		pr_err("Failed to get CPU clock: %ld\n", PTR_ERR(clk));
		return;
	}

	cpu_clk_rate = clk_get_rate(clk);

	pr_info("CPU clock: %lu.%03lu MHz\n",
		cpu_clk_rate / 1000000, (cpu_clk_rate / 1000) % 1000);

	mips_hpt_frequency = cpu_clk_rate / 2;

	clk_put(clk);
}

void __init plat_time_init(void)
{
	unsigned long cpu_clk_rate;
	unsigned long ahb_clk_rate;
	unsigned long ddr_clk_rate;
	unsigned long ref_clk_rate;

	if (IS_ENABLED(CONFIG_OF) && mips_machtype == ATH79_MACH_GENERIC_OF) {
		ath79_of_plat_time_init();
		return;
	}

	ath79_clocks_init();

	cpu_clk_rate = ath79_get_sys_clk_rate("cpu");
	ahb_clk_rate = ath79_get_sys_clk_rate("ahb");
	ddr_clk_rate = ath79_get_sys_clk_rate("ddr");
	ref_clk_rate = ath79_get_sys_clk_rate("ref");

	pr_info("Clocks: CPU:%lu.%03luMHz, DDR:%lu.%03luMHz, AHB:%lu.%03luMHz, Ref:%lu.%03luMHz\n",
		cpu_clk_rate / 1000000, (cpu_clk_rate / 1000) % 1000,
		ddr_clk_rate / 1000000, (ddr_clk_rate / 1000) % 1000,
		ahb_clk_rate / 1000000, (ahb_clk_rate / 1000) % 1000,
		ref_clk_rate / 1000000, (ref_clk_rate / 1000) % 1000);

	mips_hpt_frequency = cpu_clk_rate / 2;
}

static int __init ath79_setup(void)
{
	if  (mips_machtype == ATH79_MACH_GENERIC_OF)
		return 0;

	ath79_gpio_init();
	ath79_register_uart();
	ath79_register_wdt();

	mips_machine_setup();

	return 0;
}

arch_initcall(ath79_setup);

void __init device_tree_init(void)
{
	unflatten_and_copy_device_tree();
}

MIPS_MACHINE(ATH79_MACH_GENERIC,
	     "Generic",
	     "Generic AR71XX/AR724X/AR913X based board",
	     NULL);

MIPS_MACHINE(ATH79_MACH_GENERIC_OF,
	     "DTB",
	     "Generic AR71XX/AR724X/AR913X based board (DT)",
	     NULL);
