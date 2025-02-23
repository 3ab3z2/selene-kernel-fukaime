/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/export.h>
#include <asm-generic/bootinfo.h>

#include <loongson.h>

/* raw */
unsigned long loongson_uart_base[MAX_UARTS] = {};
/* ioremapped */
unsigned long _loongson_uart_base[MAX_UARTS] = {};

EXPORT_SYMBOL(loongson_uart_base);
EXPORT_SYMBOL(_loongson_uart_base);

void prom_init_loongson_uart_base(void)
{
	switch (mips_machtype) {
	case MACH_LOONGSON_GENERIC:
		/* The CPU provided serial port (CPU) */
		loongson_uart_base[0] = LOONGSON_REG_BASE + 0x1e0;
		break;
	case MACH_LEMOTE_FL2E:
		loongson_uart_base[0] = LOONGSON_PCIIO_BASE + 0x3f8;
		break;
	case MACH_LEMOTE_FL2F:
	case MACH_LEMOTE_LL2F:
		loongson_uart_base[0] = LOONGSON_PCIIO_BASE + 0x2f8;
		break;
	case MACH_LEMOTE_ML2F7:
	case MACH_LEMOTE_YL2F89:
	case MACH_DEXXON_GDIUM2F10:
	case MACH_LEMOTE_NAS:
	default:
		/* The CPU provided serial port (LPC) */
		loongson_uart_base[0] = LOONGSON_LIO1_BASE + 0x3f8;
		break;
	}

	_loongson_uart_base[0] =
		(unsigned long)ioremap_nocache(loongson_uart_base[0], 8);
}
