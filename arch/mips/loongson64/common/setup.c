/*
 * Copyright (C) 2007 Lemote Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 */
#include <linux/export.h>
#include <linux/init.h>

#include <asm-generic/wbflush.h>
#include <asm-generic/bootinfo.h>

#include <loongson.h>

#ifdef CONFIG_VT
#include <linux/console.h>
#include <linux/screen_info.h>
#endif

static void wbflush_loongson(void)
{
	asm(".set\tpush\n\t"
	    ".set\tnoreorder\n\t"
	    ".set mips3\n\t"
	    "sync\n\t"
	    "nop\n\t"
	    ".set\tpop\n\t"
	    ".set mips0\n\t");
}

void (*__wbflush)(void) = wbflush_loongson;
EXPORT_SYMBOL(__wbflush);

void __init plat_mem_setup(void)
{
#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
	conswitchp = &vga_con;

	screen_info = (struct screen_info) {
		.orig_x			= 0,
		.orig_y			= 25,
		.orig_video_cols	= 80,
		.orig_video_lines	= 25,
		.orig_video_isVGA	= VIDEO_TYPE_VGAC,
		.orig_video_points	= 16,
	};
#elif defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif
}
