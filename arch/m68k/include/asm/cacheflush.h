/* SPDX-License-Identifier: GPL-2.0 */
#ifdef __uClinux__
#include <asm-generic/cacheflush_no.h>
#else
#include <asm-generic/cacheflush_mm.h>
#endif
