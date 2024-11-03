// SPDX-License-Identifier: GPL-2.0
#include <linux/export.h>
#include <linux/types.h>

#include <asm-generic/checksum.h>
#include <asm-generic/div64.h>
#include <asm-generic/ftrace.h>
#include <asm-generic/page.h>
#include <asm-generic/string.h>
#include <asm-generic/tbx.h>

EXPORT_SYMBOL(clear_page);
EXPORT_SYMBOL(copy_page);

#ifdef CONFIG_FLATMEM
/* needed for the pfn_valid macro */
EXPORT_SYMBOL(max_pfn);
EXPORT_SYMBOL(min_low_pfn);
#endif

/* Network checksum functions */
EXPORT_SYMBOL(csum_partial);

/* TBI symbols */
EXPORT_SYMBOL(__TBI);
EXPORT_SYMBOL(__TBIFindSeg);
EXPORT_SYMBOL(__TBIPoll);
EXPORT_SYMBOL(__TBITimeStamp);

#define DECLARE_EXPORT(name) extern void name(void); EXPORT_SYMBOL(name)

/* libgcc functions */
DECLARE_EXPORT(__ashldi3);
DECLARE_EXPORT(__ashrdi3);
DECLARE_EXPORT(__lshrdi3);
DECLARE_EXPORT(__udivsi3);
DECLARE_EXPORT(__divsi3);
DECLARE_EXPORT(__umodsi3);
DECLARE_EXPORT(__modsi3);
DECLARE_EXPORT(__muldi3);
DECLARE_EXPORT(__cmpdi2);
DECLARE_EXPORT(__ucmpdi2);

/* Maths functions */
EXPORT_SYMBOL(div_u64);
EXPORT_SYMBOL(div_s64);

/* String functions */
EXPORT_SYMBOL(memcpy);
EXPORT_SYMBOL(memset);
EXPORT_SYMBOL(memmove);

#ifdef CONFIG_FUNCTION_TRACER
EXPORT_SYMBOL(mcount_wrapper);
#endif
