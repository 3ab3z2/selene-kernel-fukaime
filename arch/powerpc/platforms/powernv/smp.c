/*
 * SMP support for PowerNV machines.
 *
 * Copyright 2011 IBM Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/hotplug.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/cpu.h>

#include <asm-generic/irq.h>
#include <asm-generic/smp.h>
#include <asm-generic/paca.h>
#include <asm-generic/machdep.h>
#include <asm-generic/cputable.h>
#include <asm-generic/firmware.h>
#include <asm-generic/vdso_datapage.h>
#include <asm-generic/cputhreads.h>
#include <asm-generic/xics.h>
#include <asm-generic/xive.h>
#include <asm-generic/opal.h>
#include <asm-generic/runlatch.h>
#include <asm-generic/code-patching.h>
#include <asm-generic/dbell.h>
#include <asm-generic/kvm_ppc.h>
#include <asm-generic/ppc-opcode.h>
#include <asm-generic/cpuidle.h>

#include "powernv.h"

#ifdef DEBUG
#include <asm-generic/udbg.h>
#define DBG(fmt...) udbg_printf(fmt)
#else
#define DBG(fmt...) do { } while (0)
#endif

static void pnv_smp_setup_cpu(int cpu)
{
	if (xive_enabled())
		xive_smp_setup_cpu();
	else if (cpu != boot_cpuid)
		xics_setup_cpu();
}

static int pnv_smp_kick_cpu(int nr)
{
	unsigned int pcpu;
	unsigned long start_here =
			__pa(ppc_function_entry(generic_secondary_smp_init));
	long rc;
	uint8_t status;

	if (nr < 0 || nr >= nr_cpu_ids)
		return -EINVAL;

	pcpu = get_hard_smp_processor_id(nr);
	/*
	 * If we already started or OPAL is not supported, we just
	 * kick the CPU via the PACA
	 */
	if (paca[nr].cpu_start || !firmware_has_feature(FW_FEATURE_OPAL))
		goto kick;

	/*
	 * At this point, the CPU can either be spinning on the way in
	 * from kexec or be inside OPAL waiting to be started for the
	 * first time. OPAL v3 allows us to query OPAL to know if it
	 * has the CPUs, so we do that
	 */
	rc = opal_query_cpu_status(pcpu, &status);
	if (rc != OPAL_SUCCESS) {
		pr_warn("OPAL Error %ld querying CPU %d state\n", rc, nr);
		return -ENODEV;
	}

	/*
	 * Already started, just kick it, probably coming from
	 * kexec and spinning
	 */
	if (status == OPAL_THREAD_STARTED)
		goto kick;

	/*
	 * Available/inactive, let's kick it
	 */
	if (status == OPAL_THREAD_INACTIVE) {
		pr_devel("OPAL: Starting CPU %d (HW 0x%x)...\n", nr, pcpu);
		rc = opal_start_cpu(pcpu, start_here);
		if (rc != OPAL_SUCCESS) {
			pr_warn("OPAL Error %ld starting CPU %d\n", rc, nr);
			return -ENODEV;
		}
	} else {
		/*
		 * An unavailable CPU (or any other unknown status)
		 * shouldn't be started. It should also
		 * not be in the possible map but currently it can
		 * happen
		 */
		pr_devel("OPAL: CPU %d (HW 0x%x) is unavailable"
			 " (status %d)...\n", nr, pcpu, status);
		return -ENODEV;
	}

kick:
	return smp_generic_kick_cpu(nr);
}

#ifdef CONFIG_HOTPLUG_CPU

static int pnv_smp_cpu_disable(void)
{
	int cpu = smp_processor_id();

	/* This is identical to pSeries... might consolidate by
	 * moving migrate_irqs_away to a ppc_md with default to
	 * the generic fixup_irqs. --BenH.
	 */
	set_cpu_online(cpu, false);
	vdso_data->processorCount--;
	if (cpu == boot_cpuid)
		boot_cpuid = cpumask_any(cpu_online_mask);
	if (xive_enabled())
		xive_smp_disable_cpu();
	else
		xics_migrate_irqs_away();
	return 0;
}

static void pnv_smp_cpu_kill_self(void)
{
	unsigned int cpu;
	unsigned long srr1, wmask;

	/* Standard hot unplug procedure */
	/*
	 * This hard disables local interurpts, ensuring we have no lazy
	 * irqs pending.
	 */
	WARN_ON(irqs_disabled());
	hard_irq_disable();
	WARN_ON(lazy_irq_pending());

	idle_task_exit();
	current->active_mm = NULL; /* for sanity */
	cpu = smp_processor_id();
	DBG("CPU%d offline\n", cpu);
	generic_set_cpu_dead(cpu);
	smp_wmb();

	wmask = SRR1_WAKEMASK;
	if (cpu_has_feature(CPU_FTR_ARCH_207S))
		wmask = SRR1_WAKEMASK_P8;

	while (!generic_check_cpu_restart(cpu)) {
		/*
		 * Clear IPI flag, since we don't handle IPIs while
		 * offline, except for those when changing micro-threading
		 * mode, which are handled explicitly below, and those
		 * for coming online, which are handled via
		 * generic_check_cpu_restart() calls.
		 */
		kvmppc_set_host_ipi(cpu, 0);

		srr1 = pnv_cpu_offline(cpu);

		WARN_ON(lazy_irq_pending());

		/*
		 * If the SRR1 value indicates that we woke up due to
		 * an external interrupt, then clear the interrupt.
		 * We clear the interrupt before checking for the
		 * reason, so as to avoid a race where we wake up for
		 * some other reason, find nothing and clear the interrupt
		 * just as some other cpu is sending us an interrupt.
		 * If we returned from power7_nap as a result of
		 * having finished executing in a KVM guest, then srr1
		 * contains 0.
		 */
		if (((srr1 & wmask) == SRR1_WAKEEE) ||
		    ((srr1 & wmask) == SRR1_WAKEHVI)) {
			if (cpu_has_feature(CPU_FTR_ARCH_300)) {
				if (xive_enabled())
					xive_flush_interrupt();
				else
					icp_opal_flush_interrupt();
			} else
				icp_native_flush_interrupt();
		} else if ((srr1 & wmask) == SRR1_WAKEHDBELL) {
			unsigned long msg = PPC_DBELL_TYPE(PPC_DBELL_SERVER);
			asm volatile(PPC_MSGCLR(%0) : : "r" (msg));
		}
		smp_mb();

		if (cpu_core_split_required())
			continue;

		if (srr1 && !generic_check_cpu_restart(cpu))
			DBG("CPU%d Unexpected exit while offline srr1=%lx!\n",
					cpu, srr1);

	}

	DBG("CPU%d coming online...\n", cpu);
}

#endif /* CONFIG_HOTPLUG_CPU */

static int pnv_cpu_bootable(unsigned int nr)
{
	/*
	 * Starting with POWER8, the subcore logic relies on all threads of a
	 * core being booted so that they can participate in split mode
	 * switches. So on those machines we ignore the smt_enabled_at_boot
	 * setting (smt-enabled on the kernel command line).
	 */
	if (cpu_has_feature(CPU_FTR_ARCH_207S))
		return 1;

	return smp_generic_cpu_bootable(nr);
}

static int pnv_smp_prepare_cpu(int cpu)
{
	if (xive_enabled())
		return xive_smp_prepare_cpu(cpu);
	return 0;
}

/* Cause IPI as setup by the interrupt controller (xics or xive) */
static void (*ic_cause_ipi)(int cpu);

static void pnv_cause_ipi(int cpu)
{
	if (doorbell_try_core_ipi(cpu))
		return;

	ic_cause_ipi(cpu);
}

static void pnv_p9_dd1_cause_ipi(int cpu)
{
	int this_cpu = get_cpu();

	/*
	 * POWER9 DD1 has a global addressed msgsnd, but for now we restrict
	 * IPIs to same core, because it requires additional synchronization
	 * for inter-core doorbells which we do not implement.
	 */
	if (cpumask_test_cpu(cpu, cpu_sibling_mask(this_cpu)))
		doorbell_global_ipi(cpu);
	else
		ic_cause_ipi(cpu);

	put_cpu();
}

static void __init pnv_smp_probe(void)
{
	if (xive_enabled())
		xive_smp_probe();
	else
		xics_smp_probe();

	if (cpu_has_feature(CPU_FTR_DBELL)) {
		ic_cause_ipi = smp_ops->cause_ipi;
		WARN_ON(!ic_cause_ipi);

		if (cpu_has_feature(CPU_FTR_ARCH_300)) {
			if (cpu_has_feature(CPU_FTR_POWER9_DD1))
				smp_ops->cause_ipi = pnv_p9_dd1_cause_ipi;
			else
				smp_ops->cause_ipi = doorbell_global_ipi;
		} else {
			smp_ops->cause_ipi = pnv_cause_ipi;
		}
	}
}

static struct smp_ops_t pnv_smp_ops = {
	.message_pass	= NULL, /* Use smp_muxed_ipi_message_pass */
	.cause_ipi	= NULL,	/* Filled at runtime by pnv_smp_probe() */
	.cause_nmi_ipi	= NULL,
	.probe		= pnv_smp_probe,
	.prepare_cpu	= pnv_smp_prepare_cpu,
	.kick_cpu	= pnv_smp_kick_cpu,
	.setup_cpu	= pnv_smp_setup_cpu,
	.cpu_bootable	= pnv_cpu_bootable,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable	= pnv_smp_cpu_disable,
	.cpu_die	= generic_cpu_die,
#endif /* CONFIG_HOTPLUG_CPU */
};

/* This is called very early during platform setup_arch */
void __init pnv_smp_init(void)
{
	smp_ops = &pnv_smp_ops;

#ifdef CONFIG_HOTPLUG_CPU
	ppc_md.cpu_die	= pnv_smp_cpu_kill_self;
#endif
}
