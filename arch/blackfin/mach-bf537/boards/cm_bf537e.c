/*
 * Copyright 2004-2009 Analog Devices Inc.
 *           2008-2009 Bluetechnix
 *                2005 National ICT Australia (NICTA)
 *                      Aidan Williams <aidan@nicta.com.au>
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/device.h>
#include <linux/export.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#if IS_ENABLED(CONFIG_USB_ISP1362_HCD)
#include <linux/usb/isp1362.h>
#endif
#include <linux/ata_platform.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm-generic/dma.h>
#include <asm-generic/bfin5xx_spi.h>
#include <asm-generic/portmux.h>
#include <asm-generic/dpmc.h>
#include <asm-generic/bfin_sport.h>

/*
 * Name the Board for the /proc/cpuinfo
 */
const char bfin_board_name[] = "Bluetechnix CM BF537E";

#if IS_ENABLED(CONFIG_SPI_BFIN5XX)
/* all SPI peripherals info goes here */

#if IS_ENABLED(CONFIG_MTD_M25P80)
static struct mtd_partition bfin_spi_flash_partitions[] = {
	{
		.name = "bootloader(spi)",
		.size = 0x00020000,
		.offset = 0,
		.mask_flags = MTD_CAP_ROM
	}, {
		.name = "linux kernel(spi)",
		.size = 0xe0000,
		.offset = 0x20000
	}, {
		.name = "file system(spi)",
		.size = 0x700000,
		.offset = 0x00100000,
	}
};

static struct flash_platform_data bfin_spi_flash_data = {
	.name = "m25p80",
	.parts = bfin_spi_flash_partitions,
	.nr_parts = ARRAY_SIZE(bfin_spi_flash_partitions),
	.type = "m25p64",
};

/* SPI flash chip (m25p64) */
static struct bfin5xx_spi_chip spi_flash_chip_info = {
	.enable_dma = 0,         /* use dma transfer with this chip*/
};
#endif

#if IS_ENABLED(CONFIG_MMC_SPI)
static struct bfin5xx_spi_chip  mmc_spi_chip_info = {
	.enable_dma = 0,
};
#endif

static struct spi_board_info bfin_spi_board_info[] __initdata = {
#if IS_ENABLED(CONFIG_MTD_M25P80)
	{
		/* the modalias must be the same as spi device driver name */
		.modalias = "m25p80", /* Name of spi_driver for this device */
		.max_speed_hz = 25000000,     /* max spi clock (SCK) speed in HZ */
		.bus_num = 0, /* Framework bus number */
		.chip_select = 1, /* Framework chip select. On STAMP537 it is SPISSEL1*/
		.platform_data = &bfin_spi_flash_data,
		.controller_data = &spi_flash_chip_info,
		.mode = SPI_MODE_3,
	},
#endif

#if IS_ENABLED(CONFIG_SND_BF5XX_SOC_AD183X)
	{
		.modalias = "ad183x",
		.max_speed_hz = 3125000,     /* max spi clock (SCK) speed in HZ */
		.bus_num = 0,
		.chip_select = 4,
	},
#endif

#if IS_ENABLED(CONFIG_MMC_SPI)
	{
		.modalias = "mmc_spi",
		.max_speed_hz = 20000000,     /* max spi clock (SCK) speed in HZ */
		.bus_num = 0,
		.chip_select = 1,
		.controller_data = &mmc_spi_chip_info,
		.mode = SPI_MODE_3,
	},
#endif
};

/* SPI (0) */
static struct resource bfin_spi0_resource[] = {
	[0] = {
		.start = SPI0_REGBASE,
		.end   = SPI0_REGBASE + 0xFF,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = CH_SPI,
		.end   = CH_SPI,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = IRQ_SPI,
		.end   = IRQ_SPI,
		.flags = IORESOURCE_IRQ,
	},
};

/* SPI controller data */
static struct bfin5xx_spi_master bfin_spi0_info = {
	.num_chipselect = 8,
	.enable_dma = 1,  /* master has the ability to do dma transfer */
	.pin_req = {P_SPI0_SCK, P_SPI0_MISO, P_SPI0_MOSI, 0},
};

static struct platform_device bfin_spi0_device = {
	.name = "bfin-spi",
	.id = 0, /* Bus number */
	.num_resources = ARRAY_SIZE(bfin_spi0_resource),
	.resource = bfin_spi0_resource,
	.dev = {
		.platform_data = &bfin_spi0_info, /* Passed to driver */
	},
};
#endif  /* spi master and devices */

#if IS_ENABLED(CONFIG_SPI_BFIN_SPORT)

/* SPORT SPI controller data */
static struct bfin5xx_spi_master bfin_sport_spi0_info = {
	.num_chipselect = MAX_BLACKFIN_GPIOS,
	.enable_dma = 0,  /* master don't support DMA */
	.pin_req = {P_SPORT0_DTPRI, P_SPORT0_TSCLK, P_SPORT0_DRPRI,
		P_SPORT0_RSCLK, P_SPORT0_TFS, P_SPORT0_RFS, 0},
};

static struct resource bfin_sport_spi0_resource[] = {
	[0] = {
		.start = SPORT0_TCR1,
		.end   = SPORT0_TCR1 + 0xFF,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = IRQ_SPORT0_ERROR,
		.end   = IRQ_SPORT0_ERROR,
		.flags = IORESOURCE_IRQ,
		},
};

static struct platform_device bfin_sport_spi0_device = {
	.name = "bfin-sport-spi",
	.id = 1, /* Bus number */
	.num_resources = ARRAY_SIZE(bfin_sport_spi0_resource),
	.resource = bfin_sport_spi0_resource,
	.dev = {
		.platform_data = &bfin_sport_spi0_info, /* Passed to driver */
	},
};

static struct bfin5xx_spi_master bfin_sport_spi1_info = {
	.num_chipselect = MAX_BLACKFIN_GPIOS,
	.enable_dma = 0,  /* master don't support DMA */
	.pin_req = {P_SPORT1_DTPRI, P_SPORT1_TSCLK, P_SPORT1_DRPRI,
		P_SPORT1_RSCLK, P_SPORT1_TFS, P_SPORT1_RFS, 0},
};

static struct resource bfin_sport_spi1_resource[] = {
	[0] = {
		.start = SPORT1_TCR1,
		.end   = SPORT1_TCR1 + 0xFF,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = IRQ_SPORT1_ERROR,
		.end   = IRQ_SPORT1_ERROR,
		.flags = IORESOURCE_IRQ,
		},
};

static struct platform_device bfin_sport_spi1_device = {
	.name = "bfin-sport-spi",
	.id = 2, /* Bus number */
	.num_resources = ARRAY_SIZE(bfin_sport_spi1_resource),
	.resource = bfin_sport_spi1_resource,
	.dev = {
		.platform_data = &bfin_sport_spi1_info, /* Passed to driver */
	},
};

#endif  /* sport spi master and devices */

#if IS_ENABLED(CONFIG_RTC_DRV_BFIN)
static struct platform_device rtc_device = {
	.name = "rtc-bfin",
	.id   = -1,
};
#endif

#if IS_ENABLED(CONFIG_FB_HITACHI_TX09)
static struct platform_device hitachi_fb_device = {
	.name = "hitachi-tx09",
};
#endif

#if IS_ENABLED(CONFIG_SMC91X)
#include <linux/smc91x.h>

static struct smc91x_platdata smc91x_info = {
	.flags = SMC91X_USE_16BIT | SMC91X_NOWAIT,
	.leda = RPC_LED_100_10,
	.ledb = RPC_LED_TX_RX,
};

static struct resource smc91x_resources[] = {
	{
		.start = 0x20200300,
		.end = 0x20200300 + 16,
		.flags = IORESOURCE_MEM,
	}, {
		.start = IRQ_PF14,
		.end = IRQ_PF14,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
		},
};

static struct platform_device smc91x_device = {
	.name = "smc91x",
	.id = 0,
	.num_resources = ARRAY_SIZE(smc91x_resources),
	.resource = smc91x_resources,
	.dev	= {
		.platform_data	= &smc91x_info,
	},
};
#endif

#if IS_ENABLED(CONFIG_USB_ISP1362_HCD)
static struct resource isp1362_hcd_resources[] = {
	{
		.start = 0x20308000,
		.end = 0x20308000,
		.flags = IORESOURCE_MEM,
	}, {
		.start = 0x20308004,
		.end = 0x20308004,
		.flags = IORESOURCE_MEM,
	}, {
		.start = IRQ_PG15,
		.end = IRQ_PG15,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct isp1362_platform_data isp1362_priv = {
	.sel15Kres = 1,
	.clknotstop = 0,
	.oc_enable = 0,
	.int_act_high = 0,
	.int_edge_triggered = 0,
	.remote_wakeup_connected = 0,
	.no_power_switching = 1,
	.power_switching_mode = 0,
};

static struct platform_device isp1362_hcd_device = {
	.name = "isp1362-hcd",
	.id = 0,
	.dev = {
		.platform_data = &isp1362_priv,
	},
	.num_resources = ARRAY_SIZE(isp1362_hcd_resources),
	.resource = isp1362_hcd_resources,
};
#endif

#if IS_ENABLED(CONFIG_USB_NET2272)
static struct resource net2272_bfin_resources[] = {
	{
		.start = 0x20300000,
		.end = 0x20300000 + 0x100,
		.flags = IORESOURCE_MEM,
	}, {
		.start = IRQ_PG13,
		.end = IRQ_PG13,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct platform_device net2272_bfin_device = {
	.name = "net2272",
	.id = -1,
	.num_resources = ARRAY_SIZE(net2272_bfin_resources),
	.resource = net2272_bfin_resources,
};
#endif

#if IS_ENABLED(CONFIG_MTD_GPIO_ADDR)
static struct mtd_partition cm_partitions[] = {
	{
		.name   = "bootloader(nor)",
		.size   = 0x40000,
		.offset = 0,
	}, {
		.name   = "linux kernel(nor)",
		.size   = 0x100000,
		.offset = MTDPART_OFS_APPEND,
	}, {
		.name   = "file system(nor)",
		.size   = MTDPART_SIZ_FULL,
		.offset = MTDPART_OFS_APPEND,
	}
};

static struct physmap_flash_data cm_flash_data = {
	.width    = 2,
	.parts    = cm_partitions,
	.nr_parts = ARRAY_SIZE(cm_partitions),
};

static unsigned cm_flash_gpios[] = { GPIO_PF4 };

static struct resource cm_flash_resource[] = {
	{
		.name  = "cfi_probe",
		.start = 0x20000000,
		.end   = 0x201fffff,
		.flags = IORESOURCE_MEM,
	}, {
		.start = (unsigned long)cm_flash_gpios,
		.end   = ARRAY_SIZE(cm_flash_gpios),
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device cm_flash_device = {
	.name          = "gpio-addr-flash",
	.id            = 0,
	.dev = {
		.platform_data = &cm_flash_data,
	},
	.num_resources = ARRAY_SIZE(cm_flash_resource),
	.resource      = cm_flash_resource,
};
#endif

#if IS_ENABLED(CONFIG_SERIAL_BFIN)
#ifdef CONFIG_SERIAL_BFIN_UART0
static struct resource bfin_uart0_resources[] = {
	{
		.start = UART0_THR,
		.end = UART0_GCTL+2,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_UART0_TX,
		.end = IRQ_UART0_TX,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_UART0_RX,
		.end = IRQ_UART0_RX,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_UART0_ERROR,
		.end = IRQ_UART0_ERROR,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = CH_UART0_TX,
		.end = CH_UART0_TX,
		.flags = IORESOURCE_DMA,
	},
	{
		.start = CH_UART0_RX,
		.end = CH_UART0_RX,
		.flags = IORESOURCE_DMA,
	},
#ifdef CONFIG_BFIN_UART0_CTSRTS
	{
		/*
		 * Refer to arch/blackfin/mach-xxx/include/mach/gpio.h for the GPIO map.
		 */
		.start = -1,
		.end = -1,
		.flags = IORESOURCE_IO,
	},
	{
		/*
		 * Refer to arch/blackfin/mach-xxx/include/mach/gpio.h for the GPIO map.
		 */
		.start = -1,
		.end = -1,
		.flags = IORESOURCE_IO,
	},
#endif
};

static unsigned short bfin_uart0_peripherals[] = {
	P_UART0_TX, P_UART0_RX, 0
};

static struct platform_device bfin_uart0_device = {
	.name = "bfin-uart",
	.id = 0,
	.num_resources = ARRAY_SIZE(bfin_uart0_resources),
	.resource = bfin_uart0_resources,
	.dev = {
		.platform_data = &bfin_uart0_peripherals, /* Passed to driver */
	},
};
#endif
#ifdef CONFIG_SERIAL_BFIN_UART1
static struct resource bfin_uart1_resources[] = {
	{
		.start = UART1_THR,
		.end = UART1_GCTL+2,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_UART1_TX,
		.end = IRQ_UART1_TX,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_UART1_RX,
		.end = IRQ_UART1_RX,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_UART1_ERROR,
		.end = IRQ_UART1_ERROR,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = CH_UART1_TX,
		.end = CH_UART1_TX,
		.flags = IORESOURCE_DMA,
	},
	{
		.start = CH_UART1_RX,
		.end = CH_UART1_RX,
		.flags = IORESOURCE_DMA,
	},
#ifdef CONFIG_BFIN_UART1_CTSRTS
	{
		/*
		 * Refer to arch/blackfin/mach-xxx/include/mach/gpio.h for the GPIO map.
		 */
		.start = -1,
		.end = -1,
		.flags = IORESOURCE_IO,
	},
	{
		/*
		 * Refer to arch/blackfin/mach-xxx/include/mach/gpio.h for the GPIO map.
		 */
		.start = -1,
		.end = -1,
		.flags = IORESOURCE_IO,
	},
#endif
};

static unsigned short bfin_uart1_peripherals[] = {
	P_UART1_TX, P_UART1_RX, 0
};

static struct platform_device bfin_uart1_device = {
	.name = "bfin-uart",
	.id = 1,
	.num_resources = ARRAY_SIZE(bfin_uart1_resources),
	.resource = bfin_uart1_resources,
	.dev = {
		.platform_data = &bfin_uart1_peripherals, /* Passed to driver */
	},
};
#endif
#endif

#if IS_ENABLED(CONFIG_BFIN_SIR)
#ifdef CONFIG_BFIN_SIR0
static struct resource bfin_sir0_resources[] = {
	{
		.start = 0xFFC00400,
		.end = 0xFFC004FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_UART0_RX,
		.end = IRQ_UART0_RX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = CH_UART0_RX,
		.end = CH_UART0_RX+1,
		.flags = IORESOURCE_DMA,
	},
};
static struct platform_device bfin_sir0_device = {
	.name = "bfin_sir",
	.id = 0,
	.num_resources = ARRAY_SIZE(bfin_sir0_resources),
	.resource = bfin_sir0_resources,
};
#endif
#ifdef CONFIG_BFIN_SIR1
static struct resource bfin_sir1_resources[] = {
	{
		.start = 0xFFC02000,
		.end = 0xFFC020FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_UART1_RX,
		.end = IRQ_UART1_RX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = CH_UART1_RX,
		.end = CH_UART1_RX+1,
		.flags = IORESOURCE_DMA,
	},
};
static struct platform_device bfin_sir1_device = {
	.name = "bfin_sir",
	.id = 1,
	.num_resources = ARRAY_SIZE(bfin_sir1_resources),
	.resource = bfin_sir1_resources,
};
#endif
#endif

#if IS_ENABLED(CONFIG_I2C_BLACKFIN_TWI)
static const u16 bfin_twi0_pins[] = {P_TWI0_SCL, P_TWI0_SDA, 0};

static struct resource bfin_twi0_resource[] = {
	[0] = {
		.start = TWI0_REGBASE,
		.end   = TWI0_REGBASE,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_TWI,
		.end   = IRQ_TWI,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device i2c_bfin_twi_device = {
	.name = "i2c-bfin-twi",
	.id = 0,
	.num_resources = ARRAY_SIZE(bfin_twi0_resource),
	.resource = bfin_twi0_resource,
	.dev = {
		.platform_data = &bfin_twi0_pins,
	},
};
#endif

#if IS_ENABLED(CONFIG_SERIAL_BFIN_SPORT) \
|| IS_ENABLED(CONFIG_BFIN_SPORT)
unsigned short bfin_sport0_peripherals[] = {
	P_SPORT0_TFS, P_SPORT0_DTPRI, P_SPORT0_TSCLK, P_SPORT0_RFS,
	P_SPORT0_DRPRI, P_SPORT0_RSCLK, P_SPORT0_DRSEC, P_SPORT0_DTSEC, 0
};
#endif
#if IS_ENABLED(CONFIG_SERIAL_BFIN_SPORT)
#ifdef CONFIG_SERIAL_BFIN_SPORT0_UART
static struct resource bfin_sport0_uart_resources[] = {
	{
		.start = SPORT0_TCR1,
		.end = SPORT0_MRCS3+4,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_SPORT0_RX,
		.end = IRQ_SPORT0_RX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_SPORT0_ERROR,
		.end = IRQ_SPORT0_ERROR,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device bfin_sport0_uart_device = {
	.name = "bfin-sport-uart",
	.id = 0,
	.num_resources = ARRAY_SIZE(bfin_sport0_uart_resources),
	.resource = bfin_sport0_uart_resources,
	.dev = {
		.platform_data = &bfin_sport0_peripherals, /* Passed to driver */
	},
};
#endif
#ifdef CONFIG_SERIAL_BFIN_SPORT1_UART
static struct resource bfin_sport1_uart_resources[] = {
	{
		.start = SPORT1_TCR1,
		.end = SPORT1_MRCS3+4,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_SPORT1_RX,
		.end = IRQ_SPORT1_RX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_SPORT1_ERROR,
		.end = IRQ_SPORT1_ERROR,
		.flags = IORESOURCE_IRQ,
	},
};

static unsigned short bfin_sport1_peripherals[] = {
	P_SPORT1_TFS, P_SPORT1_DTPRI, P_SPORT1_TSCLK, P_SPORT1_RFS,
	P_SPORT1_DRPRI, P_SPORT1_RSCLK, 0
};

static struct platform_device bfin_sport1_uart_device = {
	.name = "bfin-sport-uart",
	.id = 1,
	.num_resources = ARRAY_SIZE(bfin_sport1_uart_resources),
	.resource = bfin_sport1_uart_resources,
	.dev = {
		.platform_data = &bfin_sport1_peripherals, /* Passed to driver */
	},
};
#endif
#endif
#if IS_ENABLED(CONFIG_BFIN_SPORT)
static struct resource bfin_sport0_resources[] = {
	{
		.start = SPORT0_TCR1,
		.end = SPORT0_MRCS3+4,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = IRQ_SPORT0_RX,
		.end = IRQ_SPORT0_RX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_SPORT0_TX,
		.end = IRQ_SPORT0_TX+1,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = IRQ_SPORT0_ERROR,
		.end = IRQ_SPORT0_ERROR,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = CH_SPORT0_TX,
		.end = CH_SPORT0_TX,
		.flags = IORESOURCE_DMA,
	},
	{
		.start = CH_SPORT0_RX,
		.end = CH_SPORT0_RX,
		.flags = IORESOURCE_DMA,
	},
};
static struct platform_device bfin_sport0_device = {
	.name = "bfin_sport_raw",
	.id = 0,
	.num_resources = ARRAY_SIZE(bfin_sport0_resources),
	.resource = bfin_sport0_resources,
	.dev = {
		.platform_data = &bfin_sport0_peripherals, /* Passed to driver */
	},
};
#endif

#if IS_ENABLED(CONFIG_BFIN_MAC)
#include <linux/bfin_mac.h>
static const unsigned short bfin_mac_peripherals[] = P_MII0;

static struct bfin_phydev_platform_data bfin_phydev_data[] = {
	{
		.addr = 1,
		.irq = IRQ_MAC_PHYINT,
	},
};

static struct bfin_mii_bus_platform_data bfin_mii_bus_data = {
	.phydev_number = 1,
	.phydev_data = bfin_phydev_data,
	.phy_mode = PHY_INTERFACE_MODE_MII,
	.mac_peripherals = bfin_mac_peripherals,
};

static struct platform_device bfin_mii_bus = {
	.name = "bfin_mii_bus",
	.dev = {
		.platform_data = &bfin_mii_bus_data,
	}
};

static struct platform_device bfin_mac_device = {
	.name = "bfin_mac",
	.dev = {
		.platform_data = &bfin_mii_bus,
	}
};
#endif

#if IS_ENABLED(CONFIG_PATA_PLATFORM)
#define PATA_INT	IRQ_PF14

static struct pata_platform_info bfin_pata_platform_data = {
	.ioport_shift = 2,
};

static struct resource bfin_pata_resources[] = {
	{
		.start = 0x2030C000,
		.end = 0x2030C01F,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = 0x2030D018,
		.end = 0x2030D01B,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = PATA_INT,
		.end = PATA_INT,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct platform_device bfin_pata_device = {
	.name = "pata_platform",
	.id = -1,
	.num_resources = ARRAY_SIZE(bfin_pata_resources),
	.resource = bfin_pata_resources,
	.dev = {
		.platform_data = &bfin_pata_platform_data,
	}
};
#endif

static const unsigned int cclk_vlev_datasheet[] =
{
	VRPAIR(VLEV_085, 250000000),
	VRPAIR(VLEV_090, 376000000),
	VRPAIR(VLEV_095, 426000000),
	VRPAIR(VLEV_100, 426000000),
	VRPAIR(VLEV_105, 476000000),
	VRPAIR(VLEV_110, 476000000),
	VRPAIR(VLEV_115, 476000000),
	VRPAIR(VLEV_120, 500000000),
	VRPAIR(VLEV_125, 533000000),
	VRPAIR(VLEV_130, 600000000),
};

static struct bfin_dpmc_platform_data bfin_dmpc_vreg_data = {
	.tuple_tab = cclk_vlev_datasheet,
	.tabsize = ARRAY_SIZE(cclk_vlev_datasheet),
	.vr_settling_time = 25 /* us */,
};

static struct platform_device bfin_dpmc = {
	.name = "bfin dpmc",
	.dev = {
		.platform_data = &bfin_dmpc_vreg_data,
	},
};

static struct platform_device *cm_bf537e_devices[] __initdata = {

	&bfin_dpmc,

#if IS_ENABLED(CONFIG_BFIN_SPORT)
	&bfin_sport0_device,
#endif

#if IS_ENABLED(CONFIG_FB_HITACHI_TX09)
	&hitachi_fb_device,
#endif

#if IS_ENABLED(CONFIG_RTC_DRV_BFIN)
	&rtc_device,
#endif

#if IS_ENABLED(CONFIG_SERIAL_BFIN)
#ifdef CONFIG_SERIAL_BFIN_UART0
	&bfin_uart0_device,
#endif
#ifdef CONFIG_SERIAL_BFIN_UART1
	&bfin_uart1_device,
#endif
#endif

#if IS_ENABLED(CONFIG_BFIN_SIR)
#ifdef CONFIG_BFIN_SIR0
	&bfin_sir0_device,
#endif
#ifdef CONFIG_BFIN_SIR1
	&bfin_sir1_device,
#endif
#endif

#if IS_ENABLED(CONFIG_I2C_BLACKFIN_TWI)
	&i2c_bfin_twi_device,
#endif

#if IS_ENABLED(CONFIG_SERIAL_BFIN_SPORT)
#ifdef CONFIG_SERIAL_BFIN_SPORT0_UART
	&bfin_sport0_uart_device,
#endif
#ifdef CONFIG_SERIAL_BFIN_SPORT1_UART
	&bfin_sport1_uart_device,
#endif
#endif

#if IS_ENABLED(CONFIG_USB_ISP1362_HCD)
	&isp1362_hcd_device,
#endif

#if IS_ENABLED(CONFIG_SMC91X)
	&smc91x_device,
#endif

#if IS_ENABLED(CONFIG_BFIN_MAC)
	&bfin_mii_bus,
	&bfin_mac_device,
#endif

#if IS_ENABLED(CONFIG_USB_NET2272)
	&net2272_bfin_device,
#endif

#if IS_ENABLED(CONFIG_SPI_BFIN5XX)
	&bfin_spi0_device,
#endif

#if IS_ENABLED(CONFIG_SPI_BFIN_SPORT)
	&bfin_sport_spi0_device,
	&bfin_sport_spi1_device,
#endif

#if IS_ENABLED(CONFIG_PATA_PLATFORM)
	&bfin_pata_device,
#endif

#if IS_ENABLED(CONFIG_MTD_GPIO_ADDR)
	&cm_flash_device,
#endif
};

static int __init net2272_init(void)
{
#if IS_ENABLED(CONFIG_USB_NET2272)
	int ret;

	ret = gpio_request(GPIO_PG14, "net2272");
	if (ret)
		return ret;

	/* Reset USB Chip, PG14 */
	gpio_direction_output(GPIO_PG14, 0);
	mdelay(2);
	gpio_set_value(GPIO_PG14, 1);
#endif

	return 0;
}

static int __init cm_bf537e_init(void)
{
	printk(KERN_INFO "%s(): registering device resources\n", __func__);
	platform_add_devices(cm_bf537e_devices, ARRAY_SIZE(cm_bf537e_devices));
#if IS_ENABLED(CONFIG_SPI_BFIN5XX)
	spi_register_board_info(bfin_spi_board_info, ARRAY_SIZE(bfin_spi_board_info));
#endif

#if IS_ENABLED(CONFIG_PATA_PLATFORM)
	irq_set_status_flags(PATA_INT, IRQ_NOAUTOEN);
#endif

	if (net2272_init())
		pr_warning("unable to configure net2272; it probably won't work\n");

	return 0;
}

arch_initcall(cm_bf537e_init);

static struct platform_device *cm_bf537e_early_devices[] __initdata = {
#if defined(CONFIG_SERIAL_BFIN_CONSOLE) || defined(CONFIG_EARLY_PRINTK)
#ifdef CONFIG_SERIAL_BFIN_UART0
	&bfin_uart0_device,
#endif
#ifdef CONFIG_SERIAL_BFIN_UART1
	&bfin_uart1_device,
#endif
#endif

#if defined(CONFIG_SERIAL_BFIN_SPORT_CONSOLE)
#ifdef CONFIG_SERIAL_BFIN_SPORT0_UART
	&bfin_sport0_uart_device,
#endif
#ifdef CONFIG_SERIAL_BFIN_SPORT1_UART
	&bfin_sport1_uart_device,
#endif
#endif
};

void __init native_machine_early_platform_add_devices(void)
{
	printk(KERN_INFO "register early platform devices\n");
	early_platform_add_devices(cm_bf537e_early_devices,
		ARRAY_SIZE(cm_bf537e_early_devices));
}

int bfin_get_ether_addr(char *addr)
{
	return 1;
}
EXPORT_SYMBOL(bfin_get_ether_addr);
