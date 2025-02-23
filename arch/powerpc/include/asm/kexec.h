/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_POWERPC_KEXEC_H
#define _ASM_POWERPC_KEXEC_H
#ifdef __KERNEL__

#if defined(CONFIG_FSL_BOOKE) || defined(CONFIG_44x)

/*
 * On FSL-BookE we setup a 1:1 mapping which covers the first 2GiB of memory
 * and therefore we can only deal with memory within this range
 */
#define KEXEC_SOURCE_MEMORY_LIMIT	(2 * 1024 * 1024 * 1024UL - 1)
#define KEXEC_DESTINATION_MEMORY_LIMIT	(2 * 1024 * 1024 * 1024UL - 1)
#define KEXEC_CONTROL_MEMORY_LIMIT	(2 * 1024 * 1024 * 1024UL - 1)

#else

/*
 * Maximum page that is mapped directly into kernel memory.
 * XXX: Since we copy virt we can use any page we allocate
 */
#define KEXEC_SOURCE_MEMORY_LIMIT (-1UL)

/*
 * Maximum address we can reach in physical address mode.
 * XXX: I want to allow initrd in highmem. Otherwise set to rmo on LPAR.
 */
#define KEXEC_DESTINATION_MEMORY_LIMIT (-1UL)

/* Maximum address we can use for the control code buffer */
#ifdef __powerpc64__
#define KEXEC_CONTROL_MEMORY_LIMIT (-1UL)
#else
/* TASK_SIZE, probably left over from use_mm ?? */
#define KEXEC_CONTROL_MEMORY_LIMIT TASK_SIZE
#endif
#endif

#define KEXEC_CONTROL_PAGE_SIZE 4096

/* The native architecture */
#ifdef __powerpc64__
#define KEXEC_ARCH KEXEC_ARCH_PPC64
#else
#define KEXEC_ARCH KEXEC_ARCH_PPC
#endif

#define KEXEC_STATE_NONE 0
#define KEXEC_STATE_IRQS_OFF 1
#define KEXEC_STATE_REAL_MODE 2

#ifndef __ASSEMBLY__
#include <asm-generic/reg.h>

typedef void (*crash_shutdown_t)(void);

#ifdef CONFIG_KEXEC_CORE

/*
 * This function is responsible for capturing register states if coming
 * via panic or invoking dump using sysrq-trigger.
 */
static inline void crash_setup_regs(struct pt_regs *newregs,
					struct pt_regs *oldregs)
{
	if (oldregs)
		memcpy(newregs, oldregs, sizeof(*newregs));
	else
		ppc_save_regs(newregs);
}

extern void kexec_smp_wait(void);	/* get and clear naca physid, wait for
					  master to copy new code to 0 */
extern int crashing_cpu;
extern void crash_send_ipi(void (*crash_ipi_callback)(struct pt_regs *));

struct kimage;
struct pt_regs;
extern void default_machine_kexec(struct kimage *image);
extern int default_machine_kexec_prepare(struct kimage *image);
extern void default_machine_crash_shutdown(struct pt_regs *regs);
extern int crash_shutdown_register(crash_shutdown_t handler);
extern int crash_shutdown_unregister(crash_shutdown_t handler);

extern void crash_kexec_secondary(struct pt_regs *regs);
extern int overlaps_crashkernel(unsigned long start, unsigned long size);
extern void reserve_crashkernel(void);
extern void machine_kexec_mask_interrupts(void);

static inline bool kdump_in_progress(void)
{
	return crashing_cpu >= 0;
}

#ifdef CONFIG_KEXEC_FILE
extern struct kexec_file_ops kexec_elf64_ops;

#ifdef CONFIG_IMA_KEXEC
#define ARCH_HAS_KIMAGE_ARCH

struct kimage_arch {
	phys_addr_t ima_buffer_addr;
	size_t ima_buffer_size;
};
#endif

int setup_purgatory(struct kimage *image, const void *slave_code,
		    const void *fdt, unsigned long kernel_load_addr,
		    unsigned long fdt_load_addr);
int setup_new_fdt(const struct kimage *image, void *fdt,
		  unsigned long initrd_load_addr, unsigned long initrd_len,
		  const char *cmdline);
int delete_fdt_mem_rsv(void *fdt, unsigned long start, unsigned long size);
#endif /* CONFIG_KEXEC_FILE */

#else /* !CONFIG_KEXEC_CORE */
static inline void crash_kexec_secondary(struct pt_regs *regs) { }

static inline int overlaps_crashkernel(unsigned long start, unsigned long size)
{
	return 0;
}

static inline void reserve_crashkernel(void) { ; }

static inline int crash_shutdown_register(crash_shutdown_t handler)
{
	return 0;
}

static inline int crash_shutdown_unregister(crash_shutdown_t handler)
{
	return 0;
}

static inline bool kdump_in_progress(void)
{
	return false;
}

#endif /* CONFIG_KEXEC_CORE */
#endif /* ! __ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* _ASM_POWERPC_KEXEC_H */
