/*
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *  JZ4740 platform devices
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General	 Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/resource.h>

#include <linux/dma-mapping.h>

#include <linux/usb/musb.h>

#include <asm-generic/mach-jz4740/platform.h>
#include <asm-generic/mach-jz4740/base.h>
#include <asm-generic/mach-jz4740/irq.h>

#include <linux/serial_core.h>
#include <linux/serial_8250.h>

#include "clock.h"

/* USB Device Controller */
struct platform_device jz4740_udc_xceiv_device = {
	.name = "usb_phy_generic",
	.id   = 0,
};

static struct resource jz4740_udc_resources[] = {
	[0] = {
		.start = JZ4740_UDC_BASE_ADDR,
		.end   = JZ4740_UDC_BASE_ADDR + 0x10000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = JZ4740_IRQ_UDC,
		.end   = JZ4740_IRQ_UDC,
		.flags = IORESOURCE_IRQ,
		.name  = "mc",
	},
};

struct platform_device jz4740_udc_device = {
	.name = "musb-jz4740",
	.id   = -1,
	.dev  = {
		.dma_mask          = &jz4740_udc_device.dev.coherent_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources = ARRAY_SIZE(jz4740_udc_resources),
	.resource      = jz4740_udc_resources,
};

/* MMC/SD controller */
static struct resource jz4740_mmc_resources[] = {
	{
		.start	= JZ4740_MSC_BASE_ADDR,
		.end	= JZ4740_MSC_BASE_ADDR + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= JZ4740_IRQ_MSC,
		.end	= JZ4740_IRQ_MSC,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device jz4740_mmc_device = {
	.name		= "jz4740-mmc",
	.id		= 0,
	.dev = {
		.dma_mask = &jz4740_mmc_device.dev.coherent_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(jz4740_mmc_resources),
	.resource	= jz4740_mmc_resources,
};

/* I2C controller */
static struct resource jz4740_i2c_resources[] = {
	{
		.start	= JZ4740_I2C_BASE_ADDR,
		.end	= JZ4740_I2C_BASE_ADDR + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= JZ4740_IRQ_I2C,
		.end	= JZ4740_IRQ_I2C,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device jz4740_i2c_device = {
	.name		= "jz4740-i2c",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(jz4740_i2c_resources),
	.resource	= jz4740_i2c_resources,
};

/* NAND controller */
static struct resource jz4740_nand_resources[] = {
	{
		.name	= "mmio",
		.start	= JZ4740_EMC_BASE_ADDR,
		.end	= JZ4740_EMC_BASE_ADDR + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "bank1",
		.start	= 0x18000000,
		.end	= 0x180C0000 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name	= "bank2",
		.start	= 0x14000000,
		.end	= 0x140C0000 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name	= "bank3",
		.start	= 0x0C000000,
		.end	= 0x0C0C0000 - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.name	= "bank4",
		.start	= 0x08000000,
		.end	= 0x080C0000 - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device jz4740_nand_device = {
	.name = "jz4740-nand",
	.num_resources = ARRAY_SIZE(jz4740_nand_resources),
	.resource = jz4740_nand_resources,
};

/* LCD controller */
static struct resource jz4740_framebuffer_resources[] = {
	{
		.start	= JZ4740_LCD_BASE_ADDR,
		.end	= JZ4740_LCD_BASE_ADDR + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device jz4740_framebuffer_device = {
	.name		= "jz4740-fb",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jz4740_framebuffer_resources),
	.resource	= jz4740_framebuffer_resources,
	.dev = {
		.dma_mask = &jz4740_framebuffer_device.dev.coherent_dma_mask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};

/* I2S controller */
static struct resource jz4740_i2s_resources[] = {
	{
		.start	= JZ4740_AIC_BASE_ADDR,
		.end	= JZ4740_AIC_BASE_ADDR + 0x38 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device jz4740_i2s_device = {
	.name		= "jz4740-i2s",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jz4740_i2s_resources),
	.resource	= jz4740_i2s_resources,
};

/* PCM */
struct platform_device jz4740_pcm_device = {
	.name		= "jz4740-pcm-audio",
	.id		= -1,
};

/* Codec */
static struct resource jz4740_codec_resources[] = {
	{
		.start	= JZ4740_AIC_BASE_ADDR + 0x80,
		.end	= JZ4740_AIC_BASE_ADDR + 0x88 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device jz4740_codec_device = {
	.name		= "jz4740-codec",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jz4740_codec_resources),
	.resource	= jz4740_codec_resources,
};

/* ADC controller */
static struct resource jz4740_adc_resources[] = {
	{
		.start	= JZ4740_SADC_BASE_ADDR,
		.end	= JZ4740_SADC_BASE_ADDR + 0x30,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= JZ4740_IRQ_SADC,
		.end	= JZ4740_IRQ_SADC,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= JZ4740_IRQ_ADC_BASE,
		.end	= JZ4740_IRQ_ADC_BASE,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device jz4740_adc_device = {
	.name		= "jz4740-adc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jz4740_adc_resources),
	.resource	= jz4740_adc_resources,
};

/* Watchdog */
static struct resource jz4740_wdt_resources[] = {
	{
		.start = JZ4740_WDT_BASE_ADDR,
		.end   = JZ4740_WDT_BASE_ADDR + 0x10 - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device jz4740_wdt_device = {
	.name	       = "jz4740-wdt",
	.id	       = -1,
	.num_resources = ARRAY_SIZE(jz4740_wdt_resources),
	.resource      = jz4740_wdt_resources,
};

/* PWM */
struct platform_device jz4740_pwm_device = {
	.name = "jz4740-pwm",
	.id   = -1,
};

/* DMA */
static struct resource jz4740_dma_resources[] = {
	{
		.start	= JZ4740_DMAC_BASE_ADDR,
		.end	= JZ4740_DMAC_BASE_ADDR + 0x400 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= JZ4740_IRQ_DMAC,
		.end	= JZ4740_IRQ_DMAC,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device jz4740_dma_device = {
	.name		= "jz4740-dma",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(jz4740_dma_resources),
	.resource	= jz4740_dma_resources,
};
