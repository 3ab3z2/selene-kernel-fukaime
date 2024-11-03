// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/kvm_host.h>
#include <linux/err.h>
#include <linux/kernel_stat.h>

#include <asm-generic/kvm_book3s.h>
#include <asm-generic/kvm_ppc.h>
#include <asm-generic/hvcall.h>
#include <asm-generic/xics.h>
#include <asm-generic/debug.h>
#include <asm-generic/synch.h>
#include <asm-generic/cputhreads.h>
#include <asm-generic/pgtable.h>
#include <asm-generic/ppc-opcode.h>
#include <asm-generic/pnv-pci.h>
#include <asm-generic/opal.h>
#include <asm-generic/smp.h>
#include <asm-generic/asm-prototypes.h>
#include <asm-generic/xive.h>
#include <asm-generic/xive-regs.h>

#include "book3s_xive.h"

/* XXX */
#include <asm-generic/udbg.h>
//#define DBG(fmt...) udbg_printf(fmt)
#define DBG(fmt...) do { } while(0)

static inline void __iomem *get_tima_phys(void)
{
	return local_paca->kvm_hstate.xive_tima_phys;
}

#undef XIVE_RUNTIME_CHECKS
#define X_PFX xive_rm_
#define X_STATIC
#define X_STAT_PFX stat_rm_
#define __x_tima		get_tima_phys()
#define __x_eoi_page(xd)	((void __iomem *)((xd)->eoi_page))
#define __x_trig_page(xd)	((void __iomem *)((xd)->trig_page))
#define __x_writeb	__raw_rm_writeb
#define __x_readw	__raw_rm_readw
#define __x_readq	__raw_rm_readq
#define __x_writeq	__raw_rm_writeq

#include "book3s_xive_template.c"
