// SPDX-License-Identifier: GPL-2.0
/*
 * ip22-setup.c: SGI specific setup, including init of the feature struct.
 *
 * Copyright (C) 1996 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1997, 1998 Ralf Baechle (ralf@gnu.org)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/console.h>
#include <linux/sched.h>
#include <linux/tty.h>

#include <asm-generic/addrspace.h>
#include <asm-generic/bcache.h>
#include <asm-generic/bootinfo.h>
#include <asm-generic/irq.h>
#include <asm-generic/reboot.h>
#include <asm-generic/time.h>
#include <asm-generic/io.h>
#include <asm-generic/traps.h>
#include <asm-generic/sgialib.h>
#include <asm-generic/sgi/mc.h>
#include <asm-generic/sgi/hpc3.h>
#include <asm-generic/sgi/ip22.h>

extern void ip22_be_init(void) __init;

void __init plat_mem_setup(void)
{
	char *ctype;
	char *cserial;

	board_be_init = ip22_be_init;

	/* Init the INDY HPC I/O controller.  Need to call this before
	 * fucking with the memory controller because it needs to know the
	 * boardID and whether this is a Guiness or a FullHouse machine.
	 */
	sgihpc_init();

	/* Init INDY memory controller. */
	sgimc_init();

#ifdef CONFIG_BOARD_SCACHE
	/* Now enable boardcaches, if any. */
	indy_sc_init();
#endif

	/* Set EISA IO port base for Indigo2
	 * ioremap cannot fail */
	set_io_port_base((unsigned long)ioremap(0x00080000,
						0x1fffffff - 0x00080000));
	/* ARCS console environment variable is set to "g?" for
	 * graphics console, it is set to "d" for the first serial
	 * line and "d2" for the second serial line.
	 *
	 * Need to check if the case is 'g' but no keyboard:
	 * (ConsoleIn/Out = serial)
	 */
	ctype = ArcGetEnvironmentVariable("console");
	cserial = ArcGetEnvironmentVariable("ConsoleOut");

	if ((ctype && *ctype == 'd') || (cserial && *cserial == 's')) {
		static char options[8] __initdata;
		char *baud = ArcGetEnvironmentVariable("dbaud");
		if (baud)
			strcpy(options, baud);
		add_preferred_console("ttyS", *(ctype + 1) == '2' ? 1 : 0,
				      baud ? options : NULL);
	} else if (!ctype || *ctype != 'g') {
		/* Use ARC if we don't want serial ('d') or graphics ('g'). */
		prom_flags |= PROM_FLAG_USE_AS_CONSOLE;
		add_preferred_console("arc", 0, NULL);
	}
}
