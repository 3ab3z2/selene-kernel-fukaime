// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/arch/arm/mach-footbridge/dc21285-timer.c
 *
 *  Copyright (C) 1998 Russell King.
 *  Copyright (C) 1998 Phil Blundell
 */
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched_clock.h>

#include <asm-generic/irq.h>

#include <asm-generic/hardware/dec21285.h>
#include <asm-generic/mach/time.h>
#include <asm-generic/system_info.h>

#include "common.h"

static u64 cksrc_dc21285_read(struct clocksource *cs)
{
	return cs->mask - *CSR_TIMER2_VALUE;
}

static int cksrc_dc21285_enable(struct clocksource *cs)
{
	*CSR_TIMER2_LOAD = cs->mask;
	*CSR_TIMER2_CLR = 0;
	*CSR_TIMER2_CNTL = TIMER_CNTL_ENABLE | TIMER_CNTL_DIV16;
	return 0;
}

static void cksrc_dc21285_disable(struct clocksource *cs)
{
	*CSR_TIMER2_CNTL = 0;
}

static struct clocksource cksrc_dc21285 = {
	.name		= "dc21285_timer2",
	.rating		= 200,
	.read		= cksrc_dc21285_read,
	.enable		= cksrc_dc21285_enable,
	.disable	= cksrc_dc21285_disable,
	.mask		= CLOCKSOURCE_MASK(24),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int ckevt_dc21285_set_next_event(unsigned long delta,
	struct clock_event_device *c)
{
	*CSR_TIMER1_CLR = 0;
	*CSR_TIMER1_LOAD = delta;
	*CSR_TIMER1_CNTL = TIMER_CNTL_ENABLE | TIMER_CNTL_DIV16;

	return 0;
}

static int ckevt_dc21285_shutdown(struct clock_event_device *c)
{
	*CSR_TIMER1_CNTL = 0;
	return 0;
}

static int ckevt_dc21285_set_periodic(struct clock_event_device *c)
{
	*CSR_TIMER1_CLR = 0;
	*CSR_TIMER1_LOAD = (mem_fclk_21285 + 8 * HZ) / (16 * HZ);
	*CSR_TIMER1_CNTL = TIMER_CNTL_ENABLE | TIMER_CNTL_AUTORELOAD |
			   TIMER_CNTL_DIV16;
	return 0;
}

static struct clock_event_device ckevt_dc21285 = {
	.name			= "dc21285_timer1",
	.features		= CLOCK_EVT_FEAT_PERIODIC |
				  CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 200,
	.irq			= IRQ_TIMER1,
	.set_next_event		= ckevt_dc21285_set_next_event,
	.set_state_shutdown	= ckevt_dc21285_shutdown,
	.set_state_periodic	= ckevt_dc21285_set_periodic,
	.set_state_oneshot	= ckevt_dc21285_shutdown,
	.tick_resume		= ckevt_dc21285_set_periodic,
};

static irqreturn_t timer1_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *ce = dev_id;

	*CSR_TIMER1_CLR = 0;

	/* Stop the timer if in one-shot mode */
	if (clockevent_state_oneshot(ce))
		*CSR_TIMER1_CNTL = 0;

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static struct irqaction footbridge_timer_irq = {
	.name		= "dc21285_timer1",
	.handler	= timer1_interrupt,
	.flags		= IRQF_TIMER | IRQF_IRQPOLL,
	.dev_id		= &ckevt_dc21285,
};

/*
 * Set up timer interrupt.
 */
void __init footbridge_timer_init(void)
{
	struct clock_event_device *ce = &ckevt_dc21285;
	unsigned rate = DIV_ROUND_CLOSEST(mem_fclk_21285, 16);

	clocksource_register_hz(&cksrc_dc21285, rate);

	setup_irq(ce->irq, &footbridge_timer_irq);

	ce->cpumask = cpumask_of(smp_processor_id());
	clockevents_config_and_register(ce, rate, 0x4, 0xffffff);
}

static u64 notrace footbridge_read_sched_clock(void)
{
	return ~*CSR_TIMER3_VALUE;
}

void __init footbridge_sched_clock(void)
{
	unsigned rate = DIV_ROUND_CLOSEST(mem_fclk_21285, 16);

	*CSR_TIMER3_LOAD = 0;
	*CSR_TIMER3_CLR = 0;
	*CSR_TIMER3_CNTL = TIMER_CNTL_ENABLE | TIMER_CNTL_DIV16;

	sched_clock_register(footbridge_read_sched_clock, 24, rate);
}
