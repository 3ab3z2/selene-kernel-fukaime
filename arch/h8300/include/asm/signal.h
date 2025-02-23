/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _H8300_SIGNAL_H
#define _H8300_SIGNAL_H

#include <uapi/asm-generic/signal.h>

/* Most things should be clean enough to redefine this at will, if care
   is taken to make libc match.  */

#define _NSIG		64
#define _NSIG_BPW	32
#define _NSIG_WORDS	(_NSIG / _NSIG_BPW)

typedef unsigned long old_sigset_t;		/* at least 32 bits */

typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

#define __ARCH_HAS_SA_RESTORER
#include <asm-generic/sigcontext.h>

#endif /* _H8300_SIGNAL_H */
