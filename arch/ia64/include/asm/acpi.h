/*
 *  Copyright (C) 1999 VA Linux Systems
 *  Copyright (C) 1999 Walt Drummond <drummond@valinux.com>
 *  Copyright (C) 2000,2001 J.I. Lee <jung-ik.lee@intel.com>
 *  Copyright (C) 2001,2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef _ASM_ACPI_H
#define _ASM_ACPI_H

#ifdef __KERNEL__

#include <acpi/pdc_intel.h>

#include <linux/init.h>
#include <linux/numa.h>
#include <asm-generic/numa.h>

#ifdef	CONFIG_ACPI
extern int acpi_lapic;
#define acpi_disabled 0	/* ACPI always enabled on IA64 */
#define acpi_noirq 0	/* ACPI always enabled on IA64 */
#define acpi_pci_disabled 0 /* ACPI PCI always enabled on IA64 */
#define acpi_strict 1	/* no ACPI spec workarounds on IA64 */

static inline bool acpi_has_cpu_in_madt(void)
{
	return !!acpi_lapic;
}
#endif
#define acpi_processor_cstate_check(x) (x) /* no idle limits on IA64 :) */
static inline void disable_acpi(void) { }

#ifdef CONFIG_IA64_GENERIC
const char *acpi_get_sysname (void);
#else
static inline const char *acpi_get_sysname (void)
{
# if defined (CONFIG_IA64_HP_SIM)
	return "hpsim";
# elif defined (CONFIG_IA64_HP_ZX1)
	return "hpzx1";
# elif defined (CONFIG_IA64_HP_ZX1_SWIOTLB)
	return "hpzx1_swiotlb";
# elif defined (CONFIG_IA64_SGI_SN2)
	return "sn2";
# elif defined (CONFIG_IA64_SGI_UV)
	return "uv";
# elif defined (CONFIG_IA64_DIG)
	return "dig";
# elif defined(CONFIG_IA64_DIG_VTD)
	return "dig_vtd";
# else
#	error Unknown platform.  Fix acpi.c.
# endif
}
#endif
int acpi_request_vector (u32 int_type);
int acpi_gsi_to_irq (u32 gsi, unsigned int *irq);

/* Low-level suspend routine. */
extern int acpi_suspend_lowlevel(void);

extern unsigned long acpi_wakeup_address;

/*
 * Record the cpei override flag and current logical cpu. This is
 * useful for CPU removal.
 */
extern unsigned int can_cpei_retarget(void);
extern unsigned int is_cpu_cpei_target(unsigned int cpu);
extern void set_cpei_target_cpu(unsigned int cpu);
extern unsigned int get_cpei_target_cpu(void);
extern void prefill_possible_map(void);
#ifdef CONFIG_ACPI_HOTPLUG_CPU
extern int additional_cpus;
#else
#define additional_cpus 0
#endif

#ifdef CONFIG_ACPI_NUMA
#if MAX_NUMNODES > 256
#define MAX_PXM_DOMAINS MAX_NUMNODES
#else
#define MAX_PXM_DOMAINS (256)
#endif
extern int pxm_to_nid_map[MAX_PXM_DOMAINS];
extern int __initdata nid_to_pxm_map[MAX_NUMNODES];
#endif

static inline bool arch_has_acpi_pdc(void) { return true; }
static inline void arch_acpi_set_pdc_bits(u32 *buf)
{
	buf[2] |= ACPI_PDC_EST_CAPABILITY_SMP;
}

#define acpi_unlazy_tlb(x)

#ifdef CONFIG_ACPI_NUMA
extern cpumask_t early_cpu_possible_map;
#define for_each_possible_early_cpu(cpu)  \
	for_each_cpu((cpu), &early_cpu_possible_map)

static inline void per_cpu_scan_finalize(int min_cpus, int reserve_cpus)
{
	int low_cpu, high_cpu;
	int cpu;
	int next_nid = 0;

	low_cpu = cpumask_weight(&early_cpu_possible_map);

	high_cpu = max(low_cpu, min_cpus);
	high_cpu = min(high_cpu + reserve_cpus, NR_CPUS);

	for (cpu = low_cpu; cpu < high_cpu; cpu++) {
		cpumask_set_cpu(cpu, &early_cpu_possible_map);
		if (node_cpuid[cpu].nid == NUMA_NO_NODE) {
			node_cpuid[cpu].nid = next_nid;
			next_nid++;
			if (next_nid >= num_online_nodes())
				next_nid = 0;
		}
	}
}

extern void acpi_numa_fixup(void);

#endif /* CONFIG_ACPI_NUMA */

#endif /*__KERNEL__*/

#endif /*_ASM_ACPI_H*/
