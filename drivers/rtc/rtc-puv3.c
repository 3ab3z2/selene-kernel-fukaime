/*
 * RTC driver code specific to PKUnity SoC and UniCore ISA
 *
 *	Maintained by GUAN Xue-tao <gxt@mprc.pku.edu.cn>
 *	Copyright (C) 2001-2010 Guan Xuetao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <asm-generic/irq.h>
#include <mach/hardware.h>

static struct resource *puv3_rtc_mem;

static int puv3_rtc_alarmno = IRQ_RTCAlarm;
static int puv3_rtc_tickno  = IRQ_RTC;

static DEFINE_SPINLOCK(puv3_rtc_pie_lock);

/* IRQ Handlers */
static irqreturn_t puv3_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	writel(readl(RTC_RTSR) | RTC_RTSR_AL, RTC_RTSR);
	rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);
	return IRQ_HANDLED;
}

static irqreturn_t puv3_rtc_tickirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	writel(readl(RTC_RTSR) | RTC_RTSR_HZ, RTC_RTSR);
	rtc_update_irq(rdev, 1, RTC_PF | RTC_IRQF);
	return IRQ_HANDLED;
}

/* Update control registers */
static void puv3_rtc_setaie(struct device *dev, int to)
{
	unsigned int tmp;

	dev_dbg(dev, "%s: aie=%d\n", __func__, to);

	tmp = readl(RTC_RTSR) & ~RTC_RTSR_ALE;

	if (to)
		tmp |= RTC_RTSR_ALE;

	writel(tmp, RTC_RTSR);
}

static int puv3_rtc_setpie(struct device *dev, int enabled)
{
	unsigned int tmp;

	dev_dbg(dev, "%s: pie=%d\n", __func__, enabled);

	spin_lock_irq(&puv3_rtc_pie_lock);
	tmp = readl(RTC_RTSR) & ~RTC_RTSR_HZE;

	if (enabled)
		tmp |= RTC_RTSR_HZE;

	writel(tmp, RTC_RTSR);
	spin_unlock_irq(&puv3_rtc_pie_lock);

	return 0;
}

/* Time read/write */
static int puv3_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	rtc_time_to_tm(readl(RTC_RCNR), rtc_tm);

	dev_dbg(dev, "read time %02x.%02x.%02x %02x/%02x/%02x\n",
		 rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return 0;
}

static int puv3_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned long rtc_count = 0;

	dev_dbg(dev, "set time %02d.%02d.%02d %02d/%02d/%02d\n",
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);

	rtc_tm_to_time(tm, &rtc_count);
	writel(rtc_count, RTC_RCNR);

	return 0;
}

static int puv3_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;

	rtc_time_to_tm(readl(RTC_RTAR), alm_tm);

	alrm->enabled = readl(RTC_RTSR) & RTC_RTSR_ALE;

	dev_dbg(dev, "read alarm %02x %02x.%02x.%02x %02x/%02x/%02x\n",
		 alrm->enabled,
		 alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
		 alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);

	return 0;
}

static int puv3_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	unsigned long rtcalarm_count = 0;

	dev_dbg(dev, "puv3_rtc_setalarm: %d, %02x/%02x/%02x %02x.%02x.%02x\n",
		 alrm->enabled,
		 tm->tm_mday & 0xff, tm->tm_mon & 0xff, tm->tm_year & 0xff,
		 tm->tm_hour & 0xff, tm->tm_min & 0xff, tm->tm_sec);

	rtc_tm_to_time(tm, &rtcalarm_count);
	writel(rtcalarm_count, RTC_RTAR);

	puv3_rtc_setaie(dev, alrm->enabled);

	if (alrm->enabled)
		enable_irq_wake(puv3_rtc_alarmno);
	else
		disable_irq_wake(puv3_rtc_alarmno);

	return 0;
}

static int puv3_rtc_proc(struct device *dev, struct seq_file *seq)
{
	seq_printf(seq, "periodic_IRQ\t: %s\n",
		     (readl(RTC_RTSR) & RTC_RTSR_HZE) ? "yes" : "no");
	return 0;
}

static const struct rtc_class_ops puv3_rtcops = {
	.read_time	= puv3_rtc_gettime,
	.set_time	= puv3_rtc_settime,
	.read_alarm	= puv3_rtc_getalarm,
	.set_alarm	= puv3_rtc_setalarm,
	.proc	        = puv3_rtc_proc,
};

static void puv3_rtc_enable(struct device *dev, int en)
{
	if (!en) {
		writel(readl(RTC_RTSR) & ~RTC_RTSR_HZE, RTC_RTSR);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readl(RTC_RTSR) & RTC_RTSR_HZE) == 0) {
			dev_info(dev, "rtc disabled, re-enabling\n");
			writel(readl(RTC_RTSR) | RTC_RTSR_HZE, RTC_RTSR);
		}
	}
}

static int puv3_rtc_remove(struct platform_device *dev)
{
	puv3_rtc_setpie(&dev->dev, 0);
	puv3_rtc_setaie(&dev->dev, 0);

	release_resource(puv3_rtc_mem);
	kfree(puv3_rtc_mem);

	return 0;
}

static int puv3_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct resource *res;
	int ret;

	dev_dbg(&pdev->dev, "%s: probe=%p\n", __func__, pdev);

	/* find the IRQs */
	puv3_rtc_tickno = platform_get_irq(pdev, 1);
	if (puv3_rtc_tickno < 0) {
		dev_err(&pdev->dev, "no irq for rtc tick\n");
		return -ENOENT;
	}

	puv3_rtc_alarmno = platform_get_irq(pdev, 0);
	if (puv3_rtc_alarmno < 0) {
		dev_err(&pdev->dev, "no irq for alarm\n");
		return -ENOENT;
	}

	dev_dbg(&pdev->dev, "PKUnity_rtc: tick irq %d, alarm irq %d\n",
		 puv3_rtc_tickno, puv3_rtc_alarmno);

	rtc = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	ret = devm_request_irq(&pdev->dev, puv3_rtc_alarmno, puv3_rtc_alarmirq,
			       0, "pkunity-rtc alarm", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", puv3_rtc_alarmno, ret);
		return ret;
	}

	ret = devm_request_irq(&pdev->dev, puv3_rtc_tickno, puv3_rtc_tickirq,
			       0, "pkunity-rtc tick", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", puv3_rtc_tickno, ret);
		return ret;
	}

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		return -ENOENT;
	}

	puv3_rtc_mem = request_mem_region(res->start, resource_size(res),
					  pdev->name);

	if (puv3_rtc_mem == NULL) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_nores;
	}

	puv3_rtc_enable(&pdev->dev, 1);

	/* register RTC and exit */
	rtc->ops = &puv3_rtcops;
	ret = rtc_register_device(rtc);
	if (ret) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		goto err_nortc;
	}

	/* platform setup code should have handled this; sigh */
	if (!device_can_wakeup(&pdev->dev))
		device_init_wakeup(&pdev->dev, 1);

	platform_set_drvdata(pdev, rtc);
	return 0;

 err_nortc:
	puv3_rtc_enable(&pdev->dev, 0);
	release_resource(puv3_rtc_mem);

 err_nores:
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int ticnt_save;

static int puv3_rtc_suspend(struct device *dev)
{
	/* save RTAR for anyone using periodic interrupts */
	ticnt_save = readl(RTC_RTAR);
	puv3_rtc_enable(dev, 0);
	return 0;
}

static int puv3_rtc_resume(struct device *dev)
{
	puv3_rtc_enable(dev, 1);
	writel(ticnt_save, RTC_RTAR);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(puv3_rtc_pm_ops, puv3_rtc_suspend, puv3_rtc_resume);

static struct platform_driver puv3_rtc_driver = {
	.probe		= puv3_rtc_probe,
	.remove		= puv3_rtc_remove,
	.driver		= {
		.name	= "PKUnity-v3-RTC",
		.pm	= &puv3_rtc_pm_ops,
	}
};

module_platform_driver(puv3_rtc_driver);

MODULE_DESCRIPTION("RTC Driver for the PKUnity v3 chip");
MODULE_AUTHOR("Hu Dongliang");
MODULE_LICENSE("GPL v2");
