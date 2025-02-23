/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Rabin Vincent <rabin.vincent@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/sys_soc.h>

#include <asm-generic/cputype.h>
#include <asm-generic/tlbflush.h>
#include <asm-generic/cacheflush.h>
#include <asm-generic/mach/map.h>

/**
 * struct dbx500_asic_id - fields of the ASIC ID
 * @process: the manufacturing process, 0x40 is 40 nm 0x00 is "standard"
 * @partnumber: hithereto 0x8500 for DB8500
 * @revision: version code in the series
 */
struct dbx500_asic_id {
	u16	partnumber;
	u8	revision;
	u8	process;
};

static struct dbx500_asic_id dbx500_id;

static unsigned int __init ux500_read_asicid(phys_addr_t addr)
{
	void __iomem *virt = ioremap(addr, 4);
	unsigned int asicid;

	if (!virt)
		return 0;

	asicid = readl(virt);
	iounmap(virt);

	return asicid;
}

static void ux500_print_soc_info(unsigned int asicid)
{
	unsigned int rev = dbx500_id.revision;

	pr_info("DB%4x ", dbx500_id.partnumber);

	if (rev == 0x01)
		pr_cont("Early Drop");
	else if (rev >= 0xA0)
		pr_cont("v%d.%d" , (rev >> 4) - 0xA + 1, rev & 0xf);
	else
		pr_cont("Unknown");

	pr_cont(" [%#010x]\n", asicid);
}

static unsigned int partnumber(unsigned int asicid)
{
	return (asicid >> 8) & 0xffff;
}

/*
 * SOC		MIDR		ASICID ADDRESS		ASICID VALUE
 * DB8500ed	0x410fc090	0x9001FFF4		0x00850001
 * DB8500v1	0x411fc091	0x9001FFF4		0x008500A0
 * DB8500v1.1	0x411fc091	0x9001FFF4		0x008500A1
 * DB8500v2	0x412fc091	0x9001DBF4		0x008500B0
 * DB8520v2.2	0x412fc091	0x9001DBF4		0x008500B2
 * DB5500v1	0x412fc091	0x9001FFF4		0x005500A0
 * DB9540	0x413fc090	0xFFFFDBF4		0x009540xx
 */

static void __init ux500_setup_id(void)
{
	unsigned int cpuid = read_cpuid_id();
	unsigned int asicid = 0;
	phys_addr_t addr = 0;

	switch (cpuid) {
	case 0x410fc090: /* DB8500ed */
	case 0x411fc091: /* DB8500v1 */
		addr = 0x9001FFF4;
		break;

	case 0x412fc091: /* DB8520 / DB8500v2 / DB5500v1 */
		asicid = ux500_read_asicid(0x9001DBF4);
		if (partnumber(asicid) == 0x8500 ||
		    partnumber(asicid) == 0x8520)
			/* DB8500v2 */
			break;

		/* DB5500v1 */
		addr = 0x9001FFF4;
		break;

	case 0x413fc090: /* DB9540 */
		addr = 0xFFFFDBF4;
		break;
	}

	if (addr)
		asicid = ux500_read_asicid(addr);

	if (!asicid) {
		pr_err("Unable to identify SoC\n");
		BUG();
	}

	dbx500_id.process = asicid >> 24;
	dbx500_id.partnumber = partnumber(asicid);
	dbx500_id.revision = asicid & 0xff;

	ux500_print_soc_info(asicid);
}

static const char * __init ux500_get_machine(void)
{
	return kasprintf(GFP_KERNEL, "DB%4x", dbx500_id.partnumber);
}

static const char * __init ux500_get_family(void)
{
	return kasprintf(GFP_KERNEL, "ux500");
}

static const char * __init ux500_get_revision(void)
{
	unsigned int rev = dbx500_id.revision;

	if (rev == 0x01)
		return kasprintf(GFP_KERNEL, "%s", "ED");
	else if (rev >= 0xA0)
		return kasprintf(GFP_KERNEL, "%d.%d",
				 (rev >> 4) - 0xA + 1, rev & 0xf);

	return kasprintf(GFP_KERNEL, "%s", "Unknown");
}

static ssize_t ux500_get_process(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (dbx500_id.process == 0x00)
		return sprintf(buf, "Standard\n");

	return sprintf(buf, "%02xnm\n", dbx500_id.process);
}

static const char *db8500_read_soc_id(struct device_node *backupram)
{
	void __iomem *base;
	const char *retstr;
	u32 uid[5];

	base = of_iomap(backupram, 0);
	if (!base)
		return NULL;
	memcpy_fromio(uid, base + 0x1fc0, sizeof(uid));

	/* Throw these device-specific numbers into the entropy pool */
	add_device_randomness(uid, sizeof(uid));
	retstr = kasprintf(GFP_KERNEL, "%08x%08x%08x%08x%08x",
			   uid[0], uid[1], uid[2], uid[3], uid[4]);
	iounmap(base);
	return retstr;
}

static void __init soc_info_populate(struct soc_device_attribute *soc_dev_attr,
				     struct device_node *backupram)
{
	soc_dev_attr->soc_id   = db8500_read_soc_id(backupram);
	soc_dev_attr->machine  = ux500_get_machine();
	soc_dev_attr->family   = ux500_get_family();
	soc_dev_attr->revision = ux500_get_revision();
}

static const struct device_attribute ux500_soc_attr =
	__ATTR(process,  S_IRUGO, ux500_get_process,  NULL);

static int __init ux500_soc_device_init(void)
{
	struct device *parent;
	struct soc_device *soc_dev;
	struct soc_device_attribute *soc_dev_attr;
	struct device_node *backupram;

	backupram = of_find_compatible_node(NULL, NULL, "ste,dbx500-backupram");
	if (!backupram)
		return 0;

	ux500_setup_id();

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENOMEM;

	soc_info_populate(soc_dev_attr, backupram);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
	        kfree(soc_dev_attr);
		return PTR_ERR(soc_dev);
	}

	parent = soc_device_to_device(soc_dev);
	device_create_file(parent, &ux500_soc_attr);

	return 0;
}
subsys_initcall(ux500_soc_device_init);
