/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_LIBGCC_H
#define __ASM_LIBGCC_H

#include <asm-generic/byteorder.h>

#ifdef __BIG_ENDIAN
struct DWstruct {
	int high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DWstruct {
	int low, high;
};
#else
#error I feel sick.
#endif

typedef union {
	struct DWstruct s;
	long long ll;
} DWunion;

#endif /* __ASM_LIBGCC_H */
