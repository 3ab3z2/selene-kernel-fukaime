/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017 Oracle and/or its affiliates. All rights reserved.
 */

#include <asm-generic/xor.h>
#include <asm-generic/checksum.h>
#include <asm-generic/trap_block.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/atomic.h>
#include <asm-generic/ftrace.h>
#include <asm-generic/cacheflush.h>
#include <asm-generic/oplib.h>
#include <linux/atomic.h>

void *__memscan_zero(void *, size_t);
void *__memscan_generic(void *, int, size_t);
void *__bzero(void *, size_t);
void VISenter(void); /* Dummy prototype to supress warning */
#undef memcpy
#undef memset
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
typedef int TItype __attribute__((mode(TI)));
TItype __multi3(TItype a, TItype b);
