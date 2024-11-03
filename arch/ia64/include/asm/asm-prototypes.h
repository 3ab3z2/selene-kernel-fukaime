/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_IA64_ASM_PROTOTYPES_H
#define _ASM_IA64_ASM_PROTOTYPES_H

#include <asm-generic/cacheflush.h>
#include <asm-generic/checksum.h>
#include <asm-generic/esi.h>
#include <asm-generic/ftrace.h>
#include <asm-generic/page.h>
#include <asm-generic/pal.h>
#include <asm-generic/string.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/unwind.h>
#include <asm-generic/xor.h>

extern const char ia64_ivt[];

signed int __divsi3(signed int, unsigned int);
signed int __modsi3(signed int, unsigned int);

signed long long __divdi3(signed long long, unsigned long long);
signed long long __moddi3(signed long long, unsigned long long);

unsigned int __udivsi3(unsigned int, unsigned int);
unsigned int __umodsi3(unsigned int, unsigned int);

unsigned long long __udivdi3(unsigned long long, unsigned long long);
unsigned long long __umoddi3(unsigned long long, unsigned long long);

#endif /* _ASM_IA64_ASM_PROTOTYPES_H */
