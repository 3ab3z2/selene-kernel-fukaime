/*
 * Copyright (C) 2008-2009 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2008-2009 PetaLogix
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef _ASM_MICROBLAZE_ELF_H
#define _ASM_MICROBLAZE_ELF_H

#include <uapi/asm-generic/elf.h>

#ifndef __uClinux__
#ifndef ELF_GREG_T
#endif
#ifndef ELF_NGREG
#endif
#ifndef ELF_GREGSET_T
#endif
#ifndef ELF_FPREGSET_T
#endif
#ifdef __MICROBLAZEEL__
#else
#endif
#define SET_PERSONALITY(ex) \
	set_personality(PER_LINUX_32BIT | (current->personality & (~PER_MASK)))
#endif /* __uClinux__ */
#endif /* _ASM_MICROBLAZE_ELF_H */
