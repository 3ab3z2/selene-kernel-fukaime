/*
 * Copyright (C) 2008-2009 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2008-2009 PetaLogix
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_MICROBLAZE_CURRENT_H
#define _ASM_MICROBLAZE_CURRENT_H

/*
 * Register used to hold the current task pointer while in the kernel.
 * Any `call clobbered' register without a special meaning should be OK,
 * but check asm-generic/microblaze/kernel/entry.S to be sure.
 */
#define CURRENT_TASK	r31
# ifndef __ASSEMBLY__
/*
 * Dedicate r31 to keeping the current task pointer
 */
register struct task_struct *current asm("r31");

# define get_current()	current
# endif /* __ASSEMBLY__ */

#endif /* _ASM_MICROBLAZE_CURRENT_H */
