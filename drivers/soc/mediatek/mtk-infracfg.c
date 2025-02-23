/*
 * Copyright (c) 2015 Pengutronix, Sascha Hauer <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/export.h>
#include <linux/jiffies.h>
#include <linux/regmap.h>
#include <linux/soc/mediatek/infracfg.h>
#include <asm-generic/processor.h>

#define MTK_POLL_DELAY_US   10
#define MTK_POLL_TIMEOUT    (jiffies_to_usecs(HZ))

#define INFRA_TOPAXI_SI1_CTL		0x0204
#define INFRA_TOPAXI_PROTECTEN		0x0220
#define INFRA_TOPAXI_PROTECTSTA1	0x0228
#define INFRA_TOPAXI_PROTECTEN_SET	0x0260
#define INFRA_TOPAXI_PROTECTEN_CLR	0x0264

/**
 * mtk_infracfg_set_axi_si1_way_en - enable AXI way_en
 * @regmap: The infracfg regmap
 * @mask: The mask containing the way_en bits to be enabled.
 *
 * This function enables the AXI way_en bits for enabled power
 * domains so that registers on the power domain can be accessed.
 */
void mtk_infracfg_set_axi_si1_way_en(struct regmap *infracfg, u32 mask)
{
	regmap_update_bits(infracfg, INFRA_TOPAXI_SI1_CTL, mask, mask);
}

/**
 * mtk_infracfg_clear_axi_si1_way_en - disable AXI way_en
 * @regmap: The infracfg regmap
 * @mask: The mask containing the way_en bits to be disabled.
 *
 * This function disables the AXI way_en bits previously enabled with
 * mtk_infracfg_set_axi_si1_way_en, to prevent system hang while accessing
 * registers.
 */
void mtk_infracfg_clear_axi_si1_way_en(struct regmap *infracfg, u32 mask)
{
	regmap_update_bits(infracfg, INFRA_TOPAXI_SI1_CTL, mask, 0);
}

/**
 * mtk_infracfg_set_bus_protection - enable bus protection
 * @regmap: The infracfg regmap
 * @mask: The mask containing the protection bits to be enabled.
 * @reg_update: The boolean flag determines to set the protection bits
 *              by regmap_update_bits with enable register(PROTECTEN) or
 *              by regmap_write with set register(PROTECTEN_SET).
 *
 * This function enables the bus protection bits for disabled power
 * domains so that the system does not hang when some unit accesses the
 * bus while in power down.
 */
int mtk_infracfg_set_bus_protection(struct regmap *infracfg, u32 mask,
		bool reg_update)
{
	u32 val;
	int ret;

	if (reg_update)
		regmap_update_bits(infracfg, INFRA_TOPAXI_PROTECTEN, mask,
				mask);
	else
		regmap_write(infracfg, INFRA_TOPAXI_PROTECTEN_SET, mask);

	ret = regmap_read_poll_timeout(infracfg, INFRA_TOPAXI_PROTECTSTA1,
				       val, (val & mask) == mask,
				       MTK_POLL_DELAY_US, MTK_POLL_TIMEOUT);

	return ret;
}

/**
 * mtk_infracfg_clear_bus_protection - disable bus protection
 * @regmap: The infracfg regmap
 * @mask: The mask containing the protection bits to be disabled.
 * @reg_update: The boolean flag determines to clear the protection bits
 *              by regmap_update_bits with enable register(PROTECTEN) or
 *              by regmap_write with clear register(PROTECTEN_CLR).
 *
 * This function disables the bus protection bits previously enabled with
 * mtk_infracfg_set_bus_protection.
 */

int mtk_infracfg_clear_bus_protection(struct regmap *infracfg, u32 mask,
		bool reg_update)
{
	int ret;
	u32 val;

	if (reg_update)
		regmap_update_bits(infracfg, INFRA_TOPAXI_PROTECTEN, mask, 0);
	else
		regmap_write(infracfg, INFRA_TOPAXI_PROTECTEN_CLR, mask);

	ret = regmap_read_poll_timeout(infracfg, INFRA_TOPAXI_PROTECTSTA1,
				       val, !(val & mask),
				       MTK_POLL_DELAY_US, MTK_POLL_TIMEOUT);

	return ret;
}
