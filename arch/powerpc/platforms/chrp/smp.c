// SPDX-License-Identifier: GPL-2.0
/*
 * Smp support for CHRP machines.
 *
 * Written by Cort Dougan (cort@cs.nmt.edu) borrowing a great
 * deal of code from the sparc and intel versions.
 *
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

#include <asm-generic/ptrace.h>
#include <linux/atomic.h>
#include <asm-generic/irq.h>
#include <asm-generic/page.h>
#include <asm-generic/pgtable.h>
#include <asm-generic/sections.h>
#include <asm-generic/io.h>
#include <asm-generic/prom.h>
#include <asm-generic/smp.h>
#include <asm-generic/machdep.h>
#include <asm-generic/mpic.h>
#include <asm-generic/rtas.h>

static int smp_chrp_kick_cpu(int nr)
{
	*(unsigned long *)KERNELBASE = nr;
	asm volatile("dcbf 0,%0"::"r"(KERNELBASE):"memory");

	return 0;
}

static void smp_chrp_setup_cpu(int cpu_nr)
{
	mpic_setup_this_cpu();
}

/* CHRP with openpic */
struct smp_ops_t chrp_smp_ops = {
	.cause_nmi_ipi = NULL,
	.message_pass = smp_mpic_message_pass,
	.probe = smp_mpic_probe,
	.kick_cpu = smp_chrp_kick_cpu,
	.setup_cpu = smp_chrp_setup_cpu,
	.give_timebase = rtas_give_timebase,
	.take_timebase = rtas_take_timebase,
};
