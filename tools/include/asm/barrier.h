/* SPDX-License-Identifier: GPL-2.0 */
#if defined(__i386__) || defined(__x86_64__)
#include "../../arch/x86/include/asm-generic/barrier.h"
#elif defined(__arm__)
#include "../../arch/arm/include/asm-generic/barrier.h"
#elif defined(__aarch64__)
#include "../../arch/arm64/include/asm-generic/barrier.h"
#elif defined(__powerpc__)
#include "../../arch/powerpc/include/asm-generic/barrier.h"
#elif defined(__s390__)
#include "../../arch/s390/include/asm-generic/barrier.h"
#elif defined(__sh__)
#include "../../arch/sh/include/asm-generic/barrier.h"
#elif defined(__sparc__)
#include "../../arch/sparc/include/asm-generic/barrier.h"
#elif defined(__tile__)
#include "../../arch/tile/include/asm-generic/barrier.h"
#elif defined(__alpha__)
#include "../../arch/alpha/include/asm-generic/barrier.h"
#elif defined(__mips__)
#include "../../arch/mips/include/asm-generic/barrier.h"
#elif defined(__ia64__)
#include "../../arch/ia64/include/asm-generic/barrier.h"
#elif defined(__xtensa__)
#include "../../arch/xtensa/include/asm-generic/barrier.h"
#else
#include <asm-generic/barrier.h>
#endif
