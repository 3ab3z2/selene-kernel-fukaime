/*
 * Copyright (C) 2013 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm-generic/compiler.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#ifdef CONFIG_MT_TRUSTY_DEBUGFS
#include <linux/random.h>
#endif
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/trusty/smcall.h>
#include <linux/trusty/sm_err.h>
#include <linux/trusty/trusty.h>

#if 0
#ifdef CONFIG_MTK_ENABLE_GENIEZONE
#ifdef CONFIG_MTK_RAM_CONSOLE
#include "trusty-ramconsole.h"
#endif
#endif
#endif

struct trusty_state;

struct trusty_work {
	struct trusty_state *ts;
	struct work_struct work;
};

struct trusty_state {
	struct mutex smc_lock;
	struct atomic_notifier_head notifier;
	struct completion cpu_idle_completion;
	char *version_str;
	u32 api_version;
	struct device *dev;
	struct workqueue_struct *nop_wq;
	struct trusty_work __percpu *nop_works;
	struct list_head nop_queue;
	spinlock_t nop_lock; /* protects nop_queue */
};

#ifdef CONFIG_ARM64
#define SMC_ARG0		"x0"
#define SMC_ARG1		"x1"
#define SMC_ARG2		"x2"
#define SMC_ARG3		"x3"
#define SMC_ARCH_EXTENSION	""
#define SMC_REGISTERS_TRASHED	"x4","x5","x6","x7","x8","x9","x10","x11", \
				"x12","x13","x14","x15","x16","x17"
#else
#define SMC_ARG0		"r0"
#define SMC_ARG1		"r1"
#define SMC_ARG2		"r2"
#define SMC_ARG3		"r3"
#define SMC_ARCH_EXTENSION	".arch_extension sec\n"
#define SMC_REGISTERS_TRASHED	"ip"
#endif

static inline ulong smc(ulong r0, ulong r1, ulong r2, ulong r3)
{
	register ulong _r0 asm(SMC_ARG0) = r0;
	register ulong _r1 asm(SMC_ARG1) = r1;
	register ulong _r2 asm(SMC_ARG2) = r2;
	register ulong _r3 asm(SMC_ARG3) = r3;

	asm volatile(
		__asmeq("%0", SMC_ARG0)
		__asmeq("%1", SMC_ARG1)
		__asmeq("%2", SMC_ARG2)
		__asmeq("%3", SMC_ARG3)
		__asmeq("%4", SMC_ARG0)
		__asmeq("%5", SMC_ARG1)
		__asmeq("%6", SMC_ARG2)
		__asmeq("%7", SMC_ARG3)
		SMC_ARCH_EXTENSION
		"smc	#0"	/* switch to secure world */
		: "=r" (_r0), "=r" (_r1), "=r" (_r2), "=r" (_r3)
		: "r" (_r0), "r" (_r1), "r" (_r2), "r" (_r3)
		: SMC_REGISTERS_TRASHED);
	return _r0;
}

#ifdef CONFIG_TRUSTY_WDT_FIQ_ARMV7_SUPPORT
s32 trusty_fast_call32_nodev(u32 smcnr, u32 a0, u32 a1, u32 a2)
{
	BUG_ON(!SMC_IS_FASTCALL(smcnr));
	BUG_ON(SMC_IS_SMC64(smcnr));

	return smc(smcnr, a0, a1, a2);
}
#endif

s32 trusty_fast_call32(struct device *dev, u32 smcnr, u32 a0, u32 a1, u32 a2)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	BUG_ON(!s);
	BUG_ON(!SMC_IS_FASTCALL(smcnr));
	BUG_ON(SMC_IS_SMC64(smcnr));

	return smc(smcnr, a0, a1, a2);
}
EXPORT_SYMBOL(trusty_fast_call32);

#ifdef CONFIG_64BIT
s64 trusty_fast_call64(struct device *dev, u64 smcnr, u64 a0, u64 a1, u64 a2)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	BUG_ON(!s);
	BUG_ON(!SMC_IS_FASTCALL(smcnr));
	BUG_ON(!SMC_IS_SMC64(smcnr));

	return smc(smcnr, a0, a1, a2);
}
#endif

static ulong trusty_std_call_inner(struct device *dev, ulong smcnr,
				   ulong a0, ulong a1, ulong a2)
{
	ulong ret;
	int retry = 5;

	dev_dbg(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx)\n",
		__func__, smcnr, a0, a1, a2);
	while (true) {
		ret = smc(smcnr, a0, a1, a2);
		while ((s32)ret == SM_ERR_FIQ_INTERRUPTED)
			ret = smc(SMC_SC_RESTART_FIQ, 0, 0, 0);
		if ((int)ret != SM_ERR_BUSY || !retry)
			break;

		dev_dbg(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx) returned busy, retry\n",
			__func__, smcnr, a0, a1, a2);
		retry--;
	}

	return ret;
}

static ulong trusty_std_call_helper(struct device *dev, ulong smcnr,
				    ulong a0, ulong a1, ulong a2)
{
	ulong ret;
	int sleep_time = 1;
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	while (true) {
		local_irq_disable();
		atomic_notifier_call_chain(&s->notifier, TRUSTY_CALL_PREPARE,
					   NULL);
		ret = trusty_std_call_inner(dev, smcnr, a0, a1, a2);
		atomic_notifier_call_chain(&s->notifier, TRUSTY_CALL_RETURNED,
					   NULL);
		local_irq_enable();

		if ((int)ret != SM_ERR_BUSY)
			break;

		if (sleep_time == 256)
			dev_warn(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx) returned busy\n",
				 __func__, smcnr, a0, a1, a2);
		dev_dbg(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx) returned busy, wait %d ms\n",
			__func__, smcnr, a0, a1, a2, sleep_time);

		msleep(sleep_time);
		if (sleep_time < 1000)
			sleep_time <<= 1;

		dev_dbg(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx) retry\n",
			__func__, smcnr, a0, a1, a2);
	}

	if (sleep_time > 256)
		dev_warn(dev, "%s(0x%lx 0x%lx 0x%lx 0x%lx) busy cleared\n",
			 __func__, smcnr, a0, a1, a2);

	return ret;
}

static void trusty_std_call_cpu_idle(struct trusty_state *s)
{
	int ret;
	unsigned long timeout = HZ * 10;

#ifdef CONFIG_TRUSTY_INTERRUPT_FIQ_ONLY
	timeout = HZ / 5; /* 200 ms */
#endif

	ret = wait_for_completion_timeout(&s->cpu_idle_completion, timeout);
	if (!ret) {
		pr_warn("%s: timed out waiting for cpu idle to clear, retry anyway\n",
			__func__);
	}
}

s32 trusty_std_call32(struct device *dev, u32 smcnr, u32 a0, u32 a1, u32 a2)
{
	int ret;
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	BUG_ON(SMC_IS_FASTCALL(smcnr));
	BUG_ON(SMC_IS_SMC64(smcnr));

	if (smcnr != SMC_SC_NOP) {
		mutex_lock(&s->smc_lock);
		reinit_completion(&s->cpu_idle_completion);
	}

	dev_dbg(dev, "%s(0x%x 0x%x 0x%x 0x%x) started\n",
		__func__, smcnr, a0, a1, a2);

	ret = trusty_std_call_helper(dev, smcnr, a0, a1, a2);
	while (ret == SM_ERR_INTERRUPTED || ret == SM_ERR_CPU_IDLE) {
		dev_dbg(dev, "%s(0x%x 0x%x 0x%x 0x%x) interrupted\n",
			__func__, smcnr, a0, a1, a2);
		if (ret == SM_ERR_CPU_IDLE)
			trusty_std_call_cpu_idle(s);
		ret = trusty_std_call_helper(dev, SMC_SC_RESTART_LAST, 0, 0, 0);
	}
	dev_dbg(dev, "%s(0x%x 0x%x 0x%x 0x%x) returned 0x%x\n",
		__func__, smcnr, a0, a1, a2, ret);

	WARN_ONCE(ret == SM_ERR_PANIC, "trusty crashed");

	if (smcnr == SMC_SC_NOP)
		complete(&s->cpu_idle_completion);
	else
		mutex_unlock(&s->smc_lock);

	return ret;
}
EXPORT_SYMBOL(trusty_std_call32);

int trusty_call_notifier_register(struct device *dev, struct notifier_block *n)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	return atomic_notifier_chain_register(&s->notifier, n);
}
EXPORT_SYMBOL(trusty_call_notifier_register);

int trusty_call_notifier_unregister(struct device *dev,
				    struct notifier_block *n)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	return atomic_notifier_chain_unregister(&s->notifier, n);
}
EXPORT_SYMBOL(trusty_call_notifier_unregister);

static int trusty_remove_child(struct device *dev, void *data)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

ssize_t trusty_version_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	return scnprintf(buf, PAGE_SIZE, "%s\n", s->version_str);
}

DEVICE_ATTR(trusty_version, S_IRUSR, trusty_version_show, NULL);

#if 0
#ifdef CONFIG_MTK_ENABLE_GENIEZONE
#ifdef CONFIG_MTK_RAM_CONSOLE
static void init_gz_ramconsole(struct device *dev)
{
	u32 low, high;
#ifdef SRAM_TEST

	low = 0x0011E000;
	high = 0x00000000;
#else
	unsigned long *gz_irq_pa;

	gz_irq_pa = aee_rr_rec_gz_irq_pa();
	dev_info(dev, "ram console: get PA %p\n", gz_irq_pa);

	low = (u32)(((u64)gz_irq_pa << 32) >> 32);
	high = (u32)((u64)gz_irq_pa >> 32);
#endif

	dev_info(dev, "ram console: send ram console PA from kernel to GZ\n");
	trusty_std_call32(dev, MT_SMC_SC_SET_RAMCONSOLE, low, high, 0);
}
#endif
#endif
#endif

#ifdef CONFIG_MT_TRUSTY_DEBUGFS
ssize_t trusty_add(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	s32 a, b, ret;

	get_random_bytes(&a, sizeof(s32));
	a &= 0xFF;
	get_random_bytes(&b, sizeof(s32));
	b &= 0xFF;
	ret = trusty_std_call32(dev, MT_SMC_SC_ADD, a, b, 0);
	return scnprintf(buf, PAGE_SIZE, "%d + %d = %d, %s\n", a, b, ret,
		(a + b) == ret ? "PASS" : "FAIL");
}

DEVICE_ATTR(trusty_add, S_IRUSR, trusty_add, NULL);

ssize_t trusty_threads(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* Dump Trusty threads info to memlog */
	trusty_fast_call32(dev, MT_SMC_FC_THREADS, 0, 0, 0);
	/* Dump threads info from memlog to kmsg*/
	trusty_std_call32(dev, SMC_SC_NOP, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_threads, S_IRUSR, trusty_threads, NULL);

ssize_t trusty_threadstats(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* Dump Trusty threads info to memlog */
	trusty_fast_call32(dev, MT_SMC_FC_THREADSTATS, 0, 0, 0);
	/* Dump threads info from memlog to kmsg*/
	trusty_std_call32(dev, SMC_SC_NOP, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_threadstats, S_IRUSR, trusty_threadstats, NULL);

ssize_t trusty_threadload(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* Dump Trusty threads info to memlog */
	trusty_fast_call32(dev, MT_SMC_FC_THREADLOAD, 0, 0, 0);
	/* Dump threads info from memlog to kmsg*/
	trusty_std_call32(dev, SMC_SC_NOP, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_threadload, S_IRUSR, trusty_threadload, NULL);

ssize_t trusty_heap_dump(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* Dump Trusty threads info to memlog */
	trusty_fast_call32(dev, MT_SMC_FC_HEAP_DUMP, 0, 0, 0);
	/* Dump threads info from memlog to kmsg*/
	trusty_std_call32(dev, SMC_SC_NOP, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_heap_dump, S_IRUSR, trusty_heap_dump, NULL);

ssize_t trusty_apps(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* Dump Trusty threads info to memlog */
	trusty_fast_call32(dev, MT_SMC_FC_APPS, 0, 0, 0);
	/* Dump threads info from memlog to kmsg*/
	trusty_std_call32(dev, SMC_SC_NOP, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_apps, S_IRUSR, trusty_apps, NULL);

ssize_t trusty_vdev_reset(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	trusty_std_call32(dev, SMC_SC_VDEV_RESET, 0, 0, 0);
	return 0;
}

DEVICE_ATTR(trusty_vdev_reset, S_IRUSR, trusty_vdev_reset, NULL);


static void trusty_create_debugfs(struct trusty_state *s, struct device *pdev)
{
	int ret;

	ret = device_create_file(pdev, &dev_attr_trusty_add);
	if (ret)
		goto err_create_trusty_add;

	ret = device_create_file(pdev, &dev_attr_trusty_threads);
	if (ret)
		goto err_create_trusty_threads;

	ret = device_create_file(pdev, &dev_attr_trusty_threadstats);
	if (ret)
		goto err_create_trusty_threadstats;

	ret = device_create_file(pdev, &dev_attr_trusty_threadload);
	if (ret)
		goto err_create_trusty_threadload;

	ret = device_create_file(pdev, &dev_attr_trusty_heap_dump);
	if (ret)
		goto err_create_trusty_heap_dump;

	ret = device_create_file(pdev, &dev_attr_trusty_apps);
	if (ret)
		goto err_create_trusty_apps;

	ret = device_create_file(pdev, &dev_attr_trusty_vdev_reset);
	if (ret)
		goto err_create_trusty_vdev_reset;

	return;

err_create_trusty_vdev_reset:
	device_remove_file(pdev, &dev_attr_trusty_vdev_reset);
err_create_trusty_apps:
	device_remove_file(pdev, &dev_attr_trusty_apps);
err_create_trusty_heap_dump:
	device_remove_file(pdev, &dev_attr_trusty_heap_dump);
err_create_trusty_threadload:
	device_remove_file(pdev, &dev_attr_trusty_threadload);
err_create_trusty_threadstats:
	device_remove_file(pdev, &dev_attr_trusty_threadstats);
err_create_trusty_threads:
	device_remove_file(pdev, &dev_attr_trusty_threads);
err_create_trusty_add:
	device_remove_file(pdev, &dev_attr_trusty_add);
}

#endif /* CONFIG_MT_TRUSTY_DEBUGFS */

const char *trusty_version_str_get(struct device *dev)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	return s->version_str;
}
EXPORT_SYMBOL(trusty_version_str_get);

static void trusty_init_version(struct trusty_state *s, struct device *dev)
{
	int ret;
	int i;
	int version_str_len;

	ret = trusty_fast_call32(dev, SMC_FC_GET_VERSION_STR, -1, 0, 0);
	if (ret <= 0)
		goto err_get_size;

	version_str_len = ret;

	s->version_str = kmalloc(version_str_len + 1, GFP_KERNEL);
	for (i = 0; i < version_str_len; i++) {
		ret = trusty_fast_call32(dev, SMC_FC_GET_VERSION_STR, i, 0, 0);
		if (ret < 0)
			goto err_get_char;
		s->version_str[i] = ret;
	}
	s->version_str[i] = '\0';

	dev_info(dev, "trusty version: %s\n", s->version_str);

	ret = device_create_file(dev, &dev_attr_trusty_version);
	if (ret)
		goto err_create_file;

	return;

err_create_file:
err_get_char:
	kfree(s->version_str);
	s->version_str = NULL;
err_get_size:
	dev_err(dev, "failed to get version: %d\n", ret);
}

u32 trusty_get_api_version(struct device *dev)
{
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	return s->api_version;
}
EXPORT_SYMBOL(trusty_get_api_version);

static int trusty_init_api_version(struct trusty_state *s, struct device *dev)
{
	u32 api_version;

	api_version = trusty_fast_call32(dev, SMC_FC_API_VERSION,
					 TRUSTY_API_VERSION_CURRENT, 0, 0);
	if (api_version == SM_ERR_UNDEFINED_SMC)
		api_version = 0;

	if (api_version > TRUSTY_API_VERSION_CURRENT) {
		dev_err(dev, "unsupported api version %u > %u\n",
			api_version, TRUSTY_API_VERSION_CURRENT);
		return -EINVAL;
	}

	dev_info(dev, "selected api version: %u (requested %u)\n",
		 api_version, TRUSTY_API_VERSION_CURRENT);
	s->api_version = api_version;

	return 0;
}

static bool dequeue_nop(struct trusty_state *s, u32 *args)
{
	unsigned long flags;
	struct trusty_nop *nop = NULL;

	spin_lock_irqsave(&s->nop_lock, flags);
	if (!list_empty(&s->nop_queue)) {
		nop = list_first_entry(&s->nop_queue,
				       struct trusty_nop, node);
		list_del_init(&nop->node);
		args[0] = nop->args[0];
		args[1] = nop->args[1];
		args[2] = nop->args[2];
	} else {
		args[0] = 0;
		args[1] = 0;
		args[2] = 0;
	}
	spin_unlock_irqrestore(&s->nop_lock, flags);
	return nop;
}

static void locked_nop_work_func(struct work_struct *work)
{
	int ret;
	struct trusty_work *tw = container_of(work, struct trusty_work, work);
	struct trusty_state *s = tw->ts;

	dev_dbg(s->dev, "%s\n", __func__);

	ret = trusty_std_call32(s->dev, SMC_SC_LOCKED_NOP, 0, 0, 0);
	if (ret != 0)
		dev_info(s->dev, "%s: SMC_SC_LOCKED_NOP failed %d",
			__func__, ret);

	dev_dbg(s->dev, "%s: done\n", __func__);
}

static void nop_work_func(struct work_struct *work)
{
	int ret;
	bool next;
	u32 args[3];
	struct trusty_work *tw = container_of(work, struct trusty_work, work);
	struct trusty_state *s = tw->ts;

	dev_dbg(s->dev, "%s:\n", __func__);

	dequeue_nop(s, args);
	do {
		dev_dbg(s->dev, "%s: %x %x %x\n",
			__func__, args[0], args[1], args[2]);

		ret = trusty_std_call32(s->dev, SMC_SC_NOP,
					args[0], args[1], args[2]);

		next = dequeue_nop(s, args);

		if (ret == SM_ERR_NOP_INTERRUPTED)
			next = true;
		else if (ret != SM_ERR_NOP_DONE)
			dev_info(s->dev, "%s: SMC_SC_NOP failed %d",
				__func__, ret);
	} while (next);

	dev_dbg(s->dev, "%s: done\n", __func__);
}

void trusty_enqueue_nop(struct device *dev, struct trusty_nop *nop)
{
	unsigned long flags;
	struct trusty_work *tw;
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	preempt_disable();
	tw = this_cpu_ptr(s->nop_works);
	if (nop) {
		WARN_ON(s->api_version < TRUSTY_API_VERSION_SMP_NOP);

		spin_lock_irqsave(&s->nop_lock, flags);
		if (list_empty(&nop->node))
			list_add_tail(&nop->node, &s->nop_queue);
		spin_unlock_irqrestore(&s->nop_lock, flags);
	}
	queue_work(s->nop_wq, &tw->work);
	preempt_enable();
}
EXPORT_SYMBOL(trusty_enqueue_nop);

void trusty_dequeue_nop(struct device *dev, struct trusty_nop *nop)
{
	unsigned long flags;
	struct trusty_state *s = platform_get_drvdata(to_platform_device(dev));

	if (WARN_ON(!nop))
		return;

	spin_lock_irqsave(&s->nop_lock, flags);
	if (!list_empty(&nop->node))
		list_del_init(&nop->node);
	spin_unlock_irqrestore(&s->nop_lock, flags);
}
EXPORT_SYMBOL(trusty_dequeue_nop);

static int trusty_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int cpu;
	work_func_t work_func;
	struct trusty_state *s;
	struct device_node *node = pdev->dev.of_node;

	if (!node) {
		dev_err(&pdev->dev, "of_node required\n");
		return -EINVAL;
	}

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s) {
		ret = -ENOMEM;
		goto err_allocate_state;
	}

	s->dev = &pdev->dev;
	spin_lock_init(&s->nop_lock);
	INIT_LIST_HEAD(&s->nop_queue);
	mutex_init(&s->smc_lock);
	ATOMIC_INIT_NOTIFIER_HEAD(&s->notifier);
	init_completion(&s->cpu_idle_completion);
	platform_set_drvdata(pdev, s);

	trusty_init_version(s, &pdev->dev);

	ret = trusty_init_api_version(s, &pdev->dev);
	if (ret < 0)
		goto err_api_version;

	s->nop_wq = alloc_workqueue("trusty-nop-wq", WQ_CPU_INTENSIVE, 0);
	if (!s->nop_wq) {
		ret = -ENODEV;
		dev_info(&pdev->dev, "Failed create trusty-nop-wq\n");
		goto err_create_nop_wq;
	}

	s->nop_works = alloc_percpu(struct trusty_work);
	if (!s->nop_works) {
		ret = -ENOMEM;
		dev_info(&pdev->dev, "Failed to allocate works\n");
		goto err_alloc_works;
	}

	if (s->api_version < TRUSTY_API_VERSION_SMP)
		work_func = locked_nop_work_func;
	else
		work_func = nop_work_func;

	for_each_possible_cpu(cpu) {
		struct trusty_work *tw = per_cpu_ptr(s->nop_works, cpu);

		tw->ts = s;
		INIT_WORK(&tw->work, work_func);
	}

	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to add children: %d\n", ret);
		goto err_add_children;
	}

#ifdef CONFIG_MT_TRUSTY_DEBUGFS
	trusty_create_debugfs(s, &pdev->dev);
#endif

#if 0
#ifdef CONFIG_MTK_ENABLE_GENIEZONE
#ifdef CONFIG_MTK_RAM_CONSOLE
	init_gz_ramconsole(&pdev->dev);
#endif
#endif
#endif

	return 0;

err_add_children:
	for_each_possible_cpu(cpu) {
		struct trusty_work *tw = per_cpu_ptr(s->nop_works, cpu);

		flush_work(&tw->work);
	}
	free_percpu(s->nop_works);
err_alloc_works:
	destroy_workqueue(s->nop_wq);
err_create_nop_wq:
err_api_version:
	if (s->version_str) {
		device_remove_file(&pdev->dev, &dev_attr_trusty_version);
		kfree(s->version_str);
	}
	device_for_each_child(&pdev->dev, NULL, trusty_remove_child);
	mutex_destroy(&s->smc_lock);
	kfree(s);
err_allocate_state:
	return ret;
}

static int trusty_remove(struct platform_device *pdev)
{
	unsigned int cpu;
	struct trusty_state *s = platform_get_drvdata(pdev);

	device_for_each_child(&pdev->dev, NULL, trusty_remove_child);

	for_each_possible_cpu(cpu) {
		struct trusty_work *tw = per_cpu_ptr(s->nop_works, cpu);

		flush_work(&tw->work);
	}
	free_percpu(s->nop_works);
	destroy_workqueue(s->nop_wq);

	mutex_destroy(&s->smc_lock);
	if (s->version_str) {
		device_remove_file(&pdev->dev, &dev_attr_trusty_version);
		kfree(s->version_str);
	}
	kfree(s);
	return 0;
}

static const struct of_device_id trusty_of_match[] = {
	{ .compatible = "android,trusty-smc-v1", },
	{},
};

static struct platform_driver trusty_driver = {
	.probe = trusty_probe,
	.remove = trusty_remove,
	.driver	= {
		.name = "trusty",
		.owner = THIS_MODULE,
		.of_match_table = trusty_of_match,
	},
};

static int __init trusty_driver_init(void)
{
	return platform_driver_register(&trusty_driver);
}

static void __exit trusty_driver_exit(void)
{
	platform_driver_unregister(&trusty_driver);
}

subsys_initcall(trusty_driver_init);
module_exit(trusty_driver_exit);
