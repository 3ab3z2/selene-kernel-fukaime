/*
 *  Copyright 2006 Michael Ellerman, IBM Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <asm-generic/machdep.h>
#include <asm-generic/page.h>
#include <asm-generic/firmware.h>
#include <asm-generic/kexec.h>
#include <asm-generic/xics.h>
#include <asm-generic/xive.h>
#include <asm-generic/smp.h>
#include <asm-generic/plpar_wrappers.h>

#include "pseries.h"

void pseries_kexec_cpu_down(int crash_shutdown, int secondary)
{
	/* Don't risk a hypervisor call if we're crashing */
	if (firmware_has_feature(FW_FEATURE_SPLPAR) && !crash_shutdown) {
		int ret;
		int cpu = smp_processor_id();
		int hwcpu = hard_smp_processor_id();

		if (get_lppaca()->dtl_enable_mask) {
			ret = unregister_dtl(hwcpu);
			if (ret) {
				pr_err("WARNING: DTL deregistration for cpu "
				       "%d (hw %d) failed with %d\n",
				       cpu, hwcpu, ret);
			}
		}

		ret = unregister_slb_shadow(hwcpu);
		if (ret) {
			pr_err("WARNING: SLB shadow buffer deregistration "
			       "for cpu %d (hw %d) failed with %d\n",
			       cpu, hwcpu, ret);
		}

		ret = unregister_vpa(hwcpu);
		if (ret) {
			pr_err("WARNING: VPA deregistration for cpu %d "
			       "(hw %d) failed with %d\n", cpu, hwcpu, ret);
		}
	}

	if (xive_enabled())
		xive_kexec_teardown_cpu(secondary);
	else
		xics_kexec_teardown_cpu(secondary);
}
