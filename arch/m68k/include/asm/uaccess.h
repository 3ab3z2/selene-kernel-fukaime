/* SPDX-License-Identifier: GPL-2.0 */
#ifdef __uClinux__
#include <asm-generic/uaccess_no.h>
#else
#include <asm-generic/uaccess_mm.h>
#endif
#include <asm-generic/extable.h>
