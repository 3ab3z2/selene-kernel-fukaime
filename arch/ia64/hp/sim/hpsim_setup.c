// SPDX-License-Identifier: GPL-2.0
/*
 * Platform dependent support for HP simulator.
 *
 * Copyright (C) 1998, 1999, 2002 Hewlett-Packard Co
 *	David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Vijay Chander <vijay@engr.sgi.com>
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/param.h>
#include <linux/root_dev.h>
#include <linux/string.h>
#include <linux/types.h>

#include <asm-generic/delay.h>
#include <asm-generic/irq.h>
#include <asm-generic/pal.h>
#include <asm-generic/machvec.h>
#include <asm-generic/pgtable.h>
#include <asm-generic/sal.h>
#include <asm-generic/hpsim.h>

#include "hpsim_ssc.h"

void
ia64_ctl_trace (long on)
{
	ia64_ssc(on, 0, 0, 0, SSC_CTL_TRACE);
}

void __init
hpsim_setup (char **cmdline_p)
{
	ROOT_DEV = Root_SDA1;		/* default to first SCSI drive */

	simcons_register();
}
