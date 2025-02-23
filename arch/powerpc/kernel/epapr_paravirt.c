/*
 * ePAPR para-virtualization support.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 */

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <asm-generic/epapr_hcalls.h>
#include <asm-generic/cacheflush.h>
#include <asm-generic/code-patching.h>
#include <asm-generic/machdep.h>

#if !defined(CONFIG_64BIT) || defined(CONFIG_PPC_BOOK3E_64)
extern void epapr_ev_idle(void);
extern u32 epapr_ev_idle_start[];
#endif

bool epapr_paravirt_enabled;
static bool __maybe_unused epapr_has_idle;

static int __init early_init_dt_scan_epapr(unsigned long node,
					   const char *uname,
					   int depth, void *data)
{
	const u32 *insts;
	int len;
	int i;

	insts = of_get_flat_dt_prop(node, "hcall-instructions", &len);
	if (!insts)
		return 0;

	if (len % 4 || len > (4 * 4))
		return -1;

	for (i = 0; i < (len / 4); i++) {
		u32 inst = be32_to_cpu(insts[i]);
		patch_instruction(epapr_hypercall_start + i, inst);
#if !defined(CONFIG_64BIT) || defined(CONFIG_PPC_BOOK3E_64)
		patch_instruction(epapr_ev_idle_start + i, inst);
#endif
	}

#if !defined(CONFIG_64BIT) || defined(CONFIG_PPC_BOOK3E_64)
	if (of_get_flat_dt_prop(node, "has-idle", NULL))
		epapr_has_idle = true;
#endif

	epapr_paravirt_enabled = true;

	return 1;
}

int __init epapr_paravirt_early_init(void)
{
	of_scan_flat_dt(early_init_dt_scan_epapr, NULL);

	return 0;
}

static int __init epapr_idle_init(void)
{
#if !defined(CONFIG_64BIT) || defined(CONFIG_PPC_BOOK3E_64)
	if (epapr_has_idle)
		ppc_md.power_save = epapr_ev_idle;
#endif

	return 0;
}

postcore_initcall(epapr_idle_init);
