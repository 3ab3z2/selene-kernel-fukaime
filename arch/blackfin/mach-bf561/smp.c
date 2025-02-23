/*
 * Copyright 2007-2009 Analog Devices Inc.
 *               Philippe Gerum <rpm@xenomai.org>
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm-generic/smp.h>
#include <asm-generic/dma.h>
#include <asm-generic/time.h>

static DEFINE_SPINLOCK(boot_lock);

/*
 * platform_init_cpus() - Tell the world about how many cores we
 * have. This is called while setting up the architecture support
 * (setup_arch()), so don't be too demanding here with respect to
 * available kernel services.
 */

void __init platform_init_cpus(void)
{
	struct cpumask mask;

	cpumask_set_cpu(0, &mask); /* CoreA */
	cpumask_set_cpu(1, &mask); /* CoreB */
	init_cpu_possible(&mask);
}

void __init platform_prepare_cpus(unsigned int max_cpus)
{
	struct cpumask mask;

	bfin_relocate_coreb_l1_mem();

	/* Both cores ought to be present on a bf561! */
	cpumask_set_cpu(0, &mask); /* CoreA */
	cpumask_set_cpu(1, &mask); /* CoreB */
	init_cpu_present(&mask);
}

int __init setup_profiling_timer(unsigned int multiplier) /* not supported */
{
	return -EINVAL;
}

void platform_secondary_init(unsigned int cpu)
{
	/* Clone setup for peripheral interrupt sources from CoreA. */
	bfin_write_SICB_IMASK0(bfin_read_SIC_IMASK0());
	bfin_write_SICB_IMASK1(bfin_read_SIC_IMASK1());
	SSYNC();

	/* Clone setup for IARs from CoreA. */
	bfin_write_SICB_IAR0(bfin_read_SIC_IAR0());
	bfin_write_SICB_IAR1(bfin_read_SIC_IAR1());
	bfin_write_SICB_IAR2(bfin_read_SIC_IAR2());
	bfin_write_SICB_IAR3(bfin_read_SIC_IAR3());
	bfin_write_SICB_IAR4(bfin_read_SIC_IAR4());
	bfin_write_SICB_IAR5(bfin_read_SIC_IAR5());
	bfin_write_SICB_IAR6(bfin_read_SIC_IAR6());
	bfin_write_SICB_IAR7(bfin_read_SIC_IAR7());
	bfin_write_SICB_IWR0(IWR_DISABLE_ALL);
	bfin_write_SICB_IWR1(IWR_DISABLE_ALL);
	SSYNC();

	/* We are done with local CPU inits, unblock the boot CPU. */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int platform_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	printk(KERN_INFO "Booting Core B.\n");

	spin_lock(&boot_lock);

	if ((bfin_read_SYSCR() & COREB_SRAM_INIT) == 0) {
		/* CoreB already running, sending ipi to wakeup it */
		smp_send_reschedule(cpu);
	} else {
		/* Kick CoreB, which should start execution from CORE_SRAM_BASE. */
		bfin_write_SYSCR(bfin_read_SYSCR() & ~COREB_SRAM_INIT);
		SSYNC();
	}

	timeout = jiffies + HZ;
	/* release the lock and let coreb run */
	spin_unlock(&boot_lock);
	while (time_before(jiffies, timeout)) {
		if (cpu_online(cpu))
			break;
		udelay(100);
		barrier();
	}

	if (cpu_online(cpu)) {
		return 0;
	} else
		panic("CPU%u: processor failed to boot\n", cpu);
}

static const char supple0[] = "IRQ_SUPPLE_0";
static const char supple1[] = "IRQ_SUPPLE_1";
void __init platform_request_ipi(int irq, void *handler)
{
	int ret;
	const char *name = (irq == IRQ_SUPPLE_0) ? supple0 : supple1;

	ret = request_irq(irq, handler, IRQF_PERCPU | IRQF_NO_SUSPEND |
			IRQF_FORCE_RESUME, name, handler);
	if (ret)
		panic("Cannot request %s for IPI service", name);
}

void platform_send_ipi(cpumask_t callmap, int irq)
{
	unsigned int cpu;
	int offset = (irq == IRQ_SUPPLE_0) ? 6 : 8;

	for_each_cpu(cpu, &callmap) {
		BUG_ON(cpu >= 2);
		SSYNC();
		bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << (offset + cpu)));
		SSYNC();
	}
}

void platform_send_ipi_cpu(unsigned int cpu, int irq)
{
	int offset = (irq == IRQ_SUPPLE_0) ? 6 : 8;
	BUG_ON(cpu >= 2);
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << (offset + cpu)));
	SSYNC();
}

void platform_clear_ipi(unsigned int cpu, int irq)
{
	int offset = (irq == IRQ_SUPPLE_0) ? 10 : 12;
	BUG_ON(cpu >= 2);
	SSYNC();
	bfin_write_SICB_SYSCR(bfin_read_SICB_SYSCR() | (1 << (offset + cpu)));
	SSYNC();
}

/*
 * Setup core B's local core timer.
 * In SMP, core timer is used for clock event device.
 */
void bfin_local_timer_setup(void)
{
#if defined(CONFIG_TICKSOURCE_CORETMR)
	struct irq_data *data = irq_get_irq_data(IRQ_CORETMR);
	struct irq_chip *chip = irq_data_get_irq_chip(data);

	bfin_coretmr_init();
	bfin_coretmr_clockevent_init();

	chip->irq_unmask(data);
#else
	/* Power down the core timer, just to play safe. */
	bfin_write_TCNTL(0);
#endif

}
