/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004, 2005 Ralf Baechle
 * Copyright (C) 2005 MIPS Technologies, Inc.
 */
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/oprofile.h>
#include <linux/smp.h>
#include <asm-generic/cpu-info.h>
#include <asm-generic/cpu-type.h>

#include "op_impl.h"

extern struct op_mips_model op_model_mipsxx_ops __weak;
extern struct op_mips_model op_model_loongson2_ops __weak;
extern struct op_mips_model op_model_loongson3_ops __weak;

static struct op_mips_model *model;

static struct op_counter_config ctr[20];

static int op_mips_setup(void)
{
	/* Pre-compute the values to stuff in the hardware registers.  */
	model->reg_setup(ctr);

	/* Configure the registers on all cpus.	 */
	on_each_cpu(model->cpu_setup, NULL, 1);

	return 0;
}

static int op_mips_create_files(struct dentry *root)
{
	int i;

	for (i = 0; i < model->num_counters; ++i) {
		struct dentry *dir;
		char buf[4];

		snprintf(buf, sizeof buf, "%d", i);
		dir = oprofilefs_mkdir(root, buf);

		oprofilefs_create_ulong(dir, "enabled", &ctr[i].enabled);
		oprofilefs_create_ulong(dir, "event", &ctr[i].event);
		oprofilefs_create_ulong(dir, "count", &ctr[i].count);
		oprofilefs_create_ulong(dir, "kernel", &ctr[i].kernel);
		oprofilefs_create_ulong(dir, "user", &ctr[i].user);
		oprofilefs_create_ulong(dir, "exl", &ctr[i].exl);
		/* Dummy.  */
		oprofilefs_create_ulong(dir, "unit_mask", &ctr[i].unit_mask);
	}

	return 0;
}

static int op_mips_start(void)
{
	on_each_cpu(model->cpu_start, NULL, 1);

	return 0;
}

static void op_mips_stop(void)
{
	/* Disable performance monitoring for all counters.  */
	on_each_cpu(model->cpu_stop, NULL, 1);
}

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	struct op_mips_model *lmodel = NULL;
	int res;

	switch (boot_cpu_type()) {
	case CPU_5KC:
	case CPU_M14KC:
	case CPU_M14KEC:
	case CPU_20KC:
	case CPU_24K:
	case CPU_25KF:
	case CPU_34K:
	case CPU_1004K:
	case CPU_74K:
	case CPU_1074K:
	case CPU_INTERAPTIV:
	case CPU_PROAPTIV:
	case CPU_P5600:
	case CPU_I6400:
	case CPU_M5150:
	case CPU_LOONGSON1:
	case CPU_SB1:
	case CPU_SB1A:
	case CPU_R10000:
	case CPU_R12000:
	case CPU_R14000:
	case CPU_R16000:
	case CPU_XLR:
		lmodel = &op_model_mipsxx_ops;
		break;

	case CPU_LOONGSON2:
		lmodel = &op_model_loongson2_ops;
		break;
	case CPU_LOONGSON3:
		lmodel = &op_model_loongson3_ops;
		break;
	};

	/*
	 * Always set the backtrace. This allows unsupported CPU types to still
	 * use timer-based oprofile.
	 */
	ops->backtrace = op_mips_backtrace;

	if (!lmodel)
		return -ENODEV;

	res = lmodel->init();
	if (res)
		return res;

	model = lmodel;

	ops->create_files	= op_mips_create_files;
	ops->setup		= op_mips_setup;
	//ops->shutdown		= op_mips_shutdown;
	ops->start		= op_mips_start;
	ops->stop		= op_mips_stop;
	ops->cpu_type		= lmodel->cpu_type;

	printk(KERN_INFO "oprofile: using %s performance monitoring.\n",
	       lmodel->cpu_type);

	return 0;
}

void oprofile_arch_exit(void)
{
	if (model)
		model->exit();
}
