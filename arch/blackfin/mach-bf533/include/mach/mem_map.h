/*
 * BF533 memory map
 *
 * Copyright 2004-2009 Analog Devices Inc.
 * Licensed under the GPL-2 or later.
 */

#ifndef __BFIN_MACH_MEM_MAP_H__
#define __BFIN_MACH_MEM_MAP_H__

#ifndef __BFIN_MEM_MAP_H__
# error "do not include mach/mem_map.h directly -- use asm-generic/mem_map.h"
#endif

/* Async Memory Banks */
#define ASYNC_BANK3_BASE	0x20300000	 /* Async Bank 3 */
#define ASYNC_BANK3_SIZE	0x00100000	/* 1M */
#define ASYNC_BANK2_BASE	0x20200000	 /* Async Bank 2 */
#define ASYNC_BANK2_SIZE	0x00100000	/* 1M */
#define ASYNC_BANK1_BASE	0x20100000	 /* Async Bank 1 */
#define ASYNC_BANK1_SIZE	0x00100000	/* 1M */
#define ASYNC_BANK0_BASE	0x20000000	 /* Async Bank 0 */
#define ASYNC_BANK0_SIZE	0x00100000	/* 1M */

/* Boot ROM Memory */

#define BOOT_ROM_START		0xEF000000
#define BOOT_ROM_LENGTH		0x400

/* Level 1 Memory */

#ifdef CONFIG_BFIN_ICACHE
#define BFIN_ICACHESIZE	(16*1024)
#else
#define BFIN_ICACHESIZE	(0*1024)
#endif

/* Memory Map for ADSP-BF533 processors */

#ifdef CONFIG_BF533
#define L1_CODE_START       0xFFA00000
#define L1_DATA_A_START     0xFF800000
#define L1_DATA_B_START     0xFF900000

#ifdef CONFIG_BFIN_ICACHE
#define L1_CODE_LENGTH      (0x14000 - 0x4000)
#else
#define L1_CODE_LENGTH      0x14000
#endif

#ifdef CONFIG_BFIN_DCACHE

#ifdef CONFIG_BFIN_DCACHE_BANKA
#define DMEM_CNTR (ACACHE_BSRAM | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      (0x8000 - 0x4000)
#define L1_DATA_B_LENGTH      0x8000
#define BFIN_DCACHESIZE	(16*1024)
#define BFIN_DSUPBANKS	1
#else
#define DMEM_CNTR (ACACHE_BCACHE | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      (0x8000 - 0x4000)
#define L1_DATA_B_LENGTH      (0x8000 - 0x4000)
#define BFIN_DCACHESIZE	(32*1024)
#define BFIN_DSUPBANKS	2
#endif

#else
#define DMEM_CNTR (ASRAM_BSRAM | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      0x8000
#define L1_DATA_B_LENGTH      0x8000
#define BFIN_DCACHESIZE	(0*1024)
#define BFIN_DSUPBANKS	0
#endif /*CONFIG_BFIN_DCACHE*/
#endif

/* Memory Map for ADSP-BF532 processors */

#ifdef CONFIG_BF532
#define L1_CODE_START       0xFFA08000
#define L1_DATA_A_START     0xFF804000
#define L1_DATA_B_START     0xFF904000

#ifdef CONFIG_BFIN_ICACHE
#define L1_CODE_LENGTH      (0xC000 - 0x4000)
#else
#define L1_CODE_LENGTH      0xC000
#endif

#ifdef CONFIG_BFIN_DCACHE

#ifdef CONFIG_BFIN_DCACHE_BANKA
#define DMEM_CNTR (ACACHE_BSRAM | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      (0x4000 - 0x4000)
#define L1_DATA_B_LENGTH      0x4000
#define BFIN_DCACHESIZE	(16*1024)
#define BFIN_DSUPBANKS	1

#else
#define DMEM_CNTR (ACACHE_BCACHE | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      (0x4000 - 0x4000)
#define L1_DATA_B_LENGTH      (0x4000 - 0x4000)
#define BFIN_DCACHESIZE	(32*1024)
#define BFIN_DSUPBANKS	2
#endif

#else
#define DMEM_CNTR (ASRAM_BSRAM | ENDCPLB | PORT_PREF0)
#define L1_DATA_A_LENGTH      0x4000
#define L1_DATA_B_LENGTH      0x4000
#define BFIN_DCACHESIZE	(0*1024)
#define BFIN_DSUPBANKS	0
#endif /*CONFIG_BFIN_DCACHE*/
#endif

/* Memory Map for ADSP-BF531 processors */

#ifdef CONFIG_BF531
#define L1_CODE_START       0xFFA08000
#define L1_DATA_A_START     0xFF804000
#define L1_DATA_B_START     0xFF904000
#define L1_CODE_LENGTH      0x4000
#define L1_DATA_B_LENGTH      0x0000


#ifdef CONFIG_BFIN_DCACHE
#define DMEM_CNTR (ACACHE_BSRAM | ENDCPLB  | PORT_PREF0)
#define L1_DATA_A_LENGTH      (0x4000 - 0x4000)
#define BFIN_DCACHESIZE	(16*1024)
#define BFIN_DSUPBANKS	1
#else
#define DMEM_CNTR (ASRAM_BSRAM | ENDCPLB  | PORT_PREF0)
#define L1_DATA_A_LENGTH      0x4000
#define BFIN_DCACHESIZE	(0*1024)
#define BFIN_DSUPBANKS	0
#endif

#endif

#endif
