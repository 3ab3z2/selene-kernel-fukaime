/*
 *  DEC I/O ASIC's counter clocksource
 *
 *  Copyright (C) 2008	Yoichi Yuasa <yuasa@linux-mips.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <linux/clocksource.h>
#include <linux/sched_clock.h>
#include <linux/init.h>

#include <asm-generic/ds1287.h>
#include <asm-generic/time.h>
#include <asm-generic/dec/ioasic.h>
#include <asm-generic/dec/ioasic_addrs.h>

static u64 dec_ioasic_hpt_read(struct clocksource *cs)
{
	return ioasic_read(IO_REG_FCTR);
}

static struct clocksource clocksource_dec = {
	.name		= "dec-ioasic",
	.read		= dec_ioasic_hpt_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static u64 notrace dec_ioasic_read_sched_clock(void)
{
	return ioasic_read(IO_REG_FCTR);
}

int __init dec_ioasic_clocksource_init(void)
{
	unsigned int freq;
	u32 start, end;
	int i = HZ / 8;

	ds1287_timer_state();
	while (!ds1287_timer_state())
		;

	start = dec_ioasic_hpt_read(&clocksource_dec);

	while (i--)
		while (!ds1287_timer_state())
			;

	end = dec_ioasic_hpt_read(&clocksource_dec);

	freq = (end - start) * 8;

	/* An early revision of the I/O ASIC didn't have the counter.  */
	if (!freq)
		return -ENXIO;

	printk(KERN_INFO "I/O ASIC clock frequency %dHz\n", freq);

	clocksource_dec.rating = 200 + freq / 10000000;
	clocksource_register_hz(&clocksource_dec, freq);

	sched_clock_register(dec_ioasic_read_sched_clock, 32, freq);

	return 0;
}
