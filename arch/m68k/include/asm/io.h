/* SPDX-License-Identifier: GPL-2.0 */
#ifdef __uClinux__
#include <asm-generic/io_no.h>
#else
#include <asm-generic/io_mm.h>
#endif

#define readb_relaxed(addr)	readb(addr)
#define readw_relaxed(addr)	readw(addr)
#define readl_relaxed(addr)	readl(addr)

#define writeb_relaxed(b, addr)	writeb(b, addr)
#define writew_relaxed(b, addr)	writew(b, addr)
#define writel_relaxed(b, addr)	writel(b, addr)
