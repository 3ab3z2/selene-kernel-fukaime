/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995, 96, 97, 98, 99, 2000, 01 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */
#include <linux/errno.h>
#include <asm-generic/asm.h>
#include <asm-generic/asmmacro.h>
#include <asm-generic/irqflags.h>
#include <asm-generic/mipsregs.h>
#include <asm-generic/regdef.h>
#include <asm-generic/stackframe.h>
#include <asm-generic/thread_info.h>
#include <asm-generic/unistd.h>

#ifndef CONFIG_MIPS32_O32
/* No O32, so define handle_sys here */
#define handle_sysn32 handle_sys
#endif

	.align	5
NESTED(handle_sysn32, PT_SIZE, sp)
#ifndef CONFIG_MIPS32_O32
	.set	noat
	SAVE_SOME
	TRACE_IRQS_ON_RELOAD
	STI
	.set	at
#endif

	dsubu	t0, v0, __NR_N32_Linux	# check syscall number
	sltiu	t0, t0, __NR_N32_Linux_syscalls + 1

#ifndef CONFIG_MIPS32_O32
	ld	t1, PT_EPC(sp)		# skip syscall on return
	daddiu	t1, 4			# skip to next instruction
	sd	t1, PT_EPC(sp)
#endif
	beqz	t0, not_n32_scall

	sd	a3, PT_R26(sp)		# save a3 for syscall restarting

	li	t1, _TIF_WORK_SYSCALL_ENTRY
	LONG_L	t0, TI_FLAGS($28)	# syscall tracing enabled?
	and	t0, t1, t0
	bnez	t0, n32_syscall_trace_entry

syscall_common:
	dsll	t0, v0, 3		# offset into table
	ld	t2, (sysn32_call_table - (__NR_N32_Linux * 8))(t0)

	jalr	t2			# Do The Real Thing (TM)

	li	t0, -EMAXERRNO - 1	# error?
	sltu	t0, t0, v0
	sd	t0, PT_R7(sp)		# set error flag
	beqz	t0, 1f

	ld	t1, PT_R2(sp)		# syscall number
	dnegu	v0			# error
	sd	t1, PT_R0(sp)		# save it for syscall restarting
1:	sd	v0, PT_R2(sp)		# result

	j	syscall_exit_partial

/* ------------------------------------------------------------------------ */

n32_syscall_trace_entry:
	SAVE_STATIC
	move	a0, sp
	move	a1, v0
	jal	syscall_trace_enter

	bltz	v0, 1f			# seccomp failed? Skip syscall

	RESTORE_STATIC
	ld	v0, PT_R2(sp)		# Restore syscall (maybe modified)
	ld	a0, PT_R4(sp)		# Restore argument registers
	ld	a1, PT_R5(sp)
	ld	a2, PT_R6(sp)
	ld	a3, PT_R7(sp)
	ld	a4, PT_R8(sp)
	ld	a5, PT_R9(sp)

	dsubu	t2, v0, __NR_N32_Linux	# check (new) syscall number
	sltiu   t0, t2, __NR_N32_Linux_syscalls + 1
	beqz	t0, not_n32_scall

	j	syscall_common

1:	j	syscall_exit

not_n32_scall:
	/* This is not an n32 compatibility syscall, pass it on to
	   the n64 syscall handlers.  */
	j	handle_sys64

	END(handle_sysn32)

	.type	sysn32_call_table, @object
EXPORT(sysn32_call_table)
	PTR	sys_read			/* 6000 */
	PTR	sys_write
	PTR	sys_open
	PTR	sys_close
	PTR	sys_newstat
	PTR	sys_newfstat			/* 6005 */
	PTR	sys_newlstat
	PTR	sys_poll
	PTR	sys_lseek
	PTR	sys_mips_mmap
	PTR	sys_mprotect			/* 6010 */
	PTR	sys_munmap
	PTR	sys_brk
	PTR	compat_sys_rt_sigaction
	PTR	compat_sys_rt_sigprocmask
	PTR	compat_sys_ioctl		/* 6015 */
	PTR	sys_pread64
	PTR	sys_pwrite64
	PTR	compat_sys_readv
	PTR	compat_sys_writev
	PTR	sys_access			/* 6020 */
	PTR	sysm_pipe
	PTR	compat_sys_select
	PTR	sys_sched_yield
	PTR	sys_mremap
	PTR	sys_msync			/* 6025 */
	PTR	sys_mincore
	PTR	sys_madvise
	PTR	sys_shmget
	PTR	sys_shmat
	PTR	compat_sys_shmctl			/* 6030 */
	PTR	sys_dup
	PTR	sys_dup2
	PTR	sys_pause
	PTR	compat_sys_nanosleep
	PTR	compat_sys_getitimer		/* 6035 */
	PTR	compat_sys_setitimer
	PTR	sys_alarm
	PTR	sys_getpid
	PTR	compat_sys_sendfile
	PTR	sys_socket			/* 6040 */
	PTR	sys_connect
	PTR	sys_accept
	PTR	sys_sendto
	PTR	compat_sys_recvfrom
	PTR	compat_sys_sendmsg		/* 6045 */
	PTR	compat_sys_recvmsg
	PTR	sys_shutdown
	PTR	sys_bind
	PTR	sys_listen
	PTR	sys_getsockname			/* 6050 */
	PTR	sys_getpeername
	PTR	sys_socketpair
	PTR	compat_sys_setsockopt
	PTR	compat_sys_getsockopt
	PTR	__sys_clone			/* 6055 */
	PTR	__sys_fork
	PTR	compat_sys_execve
	PTR	sys_exit
	PTR	compat_sys_wait4
	PTR	sys_kill			/* 6060 */
	PTR	sys_newuname
	PTR	sys_semget
	PTR	sys_semop
	PTR	compat_sys_semctl
	PTR	sys_shmdt			/* 6065 */
	PTR	sys_msgget
	PTR	compat_sys_msgsnd
	PTR	compat_sys_msgrcv
	PTR	compat_sys_msgctl
	PTR	compat_sys_fcntl		/* 6070 */
	PTR	sys_flock
	PTR	sys_fsync
	PTR	sys_fdatasync
	PTR	sys_truncate
	PTR	sys_ftruncate			/* 6075 */
	PTR	compat_sys_getdents
	PTR	sys_getcwd
	PTR	sys_chdir
	PTR	sys_fchdir
	PTR	sys_rename			/* 6080 */
	PTR	sys_mkdir
	PTR	sys_rmdir
	PTR	sys_creat
	PTR	sys_link
	PTR	sys_unlink			/* 6085 */
	PTR	sys_symlink
	PTR	sys_readlink
	PTR	sys_chmod
	PTR	sys_fchmod
	PTR	sys_chown			/* 6090 */
	PTR	sys_fchown
	PTR	sys_lchown
	PTR	sys_umask
	PTR	compat_sys_gettimeofday
	PTR	compat_sys_getrlimit		/* 6095 */
	PTR	compat_sys_getrusage
	PTR	compat_sys_sysinfo
	PTR	compat_sys_times
	PTR	compat_sys_ptrace
	PTR	sys_getuid			/* 6100 */
	PTR	sys_syslog
	PTR	sys_getgid
	PTR	sys_setuid
	PTR	sys_setgid
	PTR	sys_geteuid			/* 6105 */
	PTR	sys_getegid
	PTR	sys_setpgid
	PTR	sys_getppid
	PTR	sys_getpgrp
	PTR	sys_setsid			/* 6110 */
	PTR	sys_setreuid
	PTR	sys_setregid
	PTR	sys_getgroups
	PTR	sys_setgroups
	PTR	sys_setresuid			/* 6115 */
	PTR	sys_getresuid
	PTR	sys_setresgid
	PTR	sys_getresgid
	PTR	sys_getpgid
	PTR	sys_setfsuid			/* 6120 */
	PTR	sys_setfsgid
	PTR	sys_getsid
	PTR	sys_capget
	PTR	sys_capset
	PTR	compat_sys_rt_sigpending	/* 6125 */
	PTR	compat_sys_rt_sigtimedwait
	PTR	compat_sys_rt_sigqueueinfo
	PTR	compat_sys_rt_sigsuspend
	PTR	compat_sys_sigaltstack
	PTR	compat_sys_utime		/* 6130 */
	PTR	sys_mknod
	PTR	sys_32_personality
	PTR	compat_sys_ustat
	PTR	compat_sys_statfs
	PTR	compat_sys_fstatfs		/* 6135 */
	PTR	sys_sysfs
	PTR	sys_getpriority
	PTR	sys_setpriority
	PTR	sys_sched_setparam
	PTR	sys_sched_getparam		/* 6140 */
	PTR	sys_sched_setscheduler
	PTR	sys_sched_getscheduler
	PTR	sys_sched_get_priority_max
	PTR	sys_sched_get_priority_min
	PTR	compat_sys_sched_rr_get_interval	/* 6145 */
	PTR	sys_mlock
	PTR	sys_munlock
	PTR	sys_mlockall
	PTR	sys_munlockall
	PTR	sys_vhangup			/* 6150 */
	PTR	sys_pivot_root
	PTR	compat_sys_sysctl
	PTR	sys_prctl
	PTR	compat_sys_adjtimex
	PTR	compat_sys_setrlimit		/* 6155 */
	PTR	sys_chroot
	PTR	sys_sync
	PTR	sys_acct
	PTR	compat_sys_settimeofday
	PTR	compat_sys_mount		/* 6160 */
	PTR	sys_umount
	PTR	sys_swapon
	PTR	sys_swapoff
	PTR	sys_reboot
	PTR	sys_sethostname			/* 6165 */
	PTR	sys_setdomainname
	PTR	sys_ni_syscall			/* was create_module */
	PTR	sys_init_module
	PTR	sys_delete_module
	PTR	sys_ni_syscall			/* 6170, was get_kernel_syms */
	PTR	sys_ni_syscall			/* was query_module */
	PTR	sys_quotactl
	PTR	sys_ni_syscall			/* was nfsservctl */
	PTR	sys_ni_syscall			/* res. for getpmsg */
	PTR	sys_ni_syscall			/* 6175	 for putpmsg */
	PTR	sys_ni_syscall			/* res. for afs_syscall */
	PTR	sys_ni_syscall			/* res. for security */
	PTR	sys_gettid
	PTR	sys_readahead
	PTR	sys_setxattr			/* 6180 */
	PTR	sys_lsetxattr
	PTR	sys_fsetxattr
	PTR	sys_getxattr
	PTR	sys_lgetxattr
	PTR	sys_fgetxattr			/* 6185 */
	PTR	sys_listxattr
	PTR	sys_llistxattr
	PTR	sys_flistxattr
	PTR	sys_removexattr
	PTR	sys_lremovexattr		/* 6190 */
	PTR	sys_fremovexattr
	PTR	sys_tkill
	PTR	sys_ni_syscall
	PTR	compat_sys_futex
	PTR	compat_sys_sched_setaffinity	/* 6195 */
	PTR	compat_sys_sched_getaffinity
	PTR	sys_cacheflush
	PTR	sys_cachectl
	PTR	__sys_sysmips
	PTR	compat_sys_io_setup			/* 6200 */
	PTR	sys_io_destroy
	PTR	compat_sys_io_getevents
	PTR	compat_sys_io_submit
	PTR	sys_io_cancel
	PTR	sys_exit_group			/* 6205 */
	PTR	sys_lookup_dcookie
	PTR	sys_epoll_create
	PTR	sys_epoll_ctl
	PTR	sys_epoll_wait
	PTR	sys_remap_file_pages		/* 6210 */
	PTR	sysn32_rt_sigreturn
	PTR	compat_sys_fcntl64
	PTR	sys_set_tid_address
	PTR	sys_restart_syscall
	PTR	compat_sys_semtimedop			/* 6215 */
	PTR	sys_fadvise64_64
	PTR	compat_sys_statfs64
	PTR	compat_sys_fstatfs64
	PTR	sys_sendfile64
	PTR	compat_sys_timer_create		/* 6220 */
	PTR	compat_sys_timer_settime
	PTR	compat_sys_timer_gettime
	PTR	sys_timer_getoverrun
	PTR	sys_timer_delete
	PTR	compat_sys_clock_settime		/* 6225 */
	PTR	compat_sys_clock_gettime
	PTR	compat_sys_clock_getres
	PTR	compat_sys_clock_nanosleep
	PTR	sys_tgkill
	PTR	compat_sys_utimes		/* 6230 */
	PTR	compat_sys_mbind
	PTR	compat_sys_get_mempolicy
	PTR	compat_sys_set_mempolicy
	PTR	compat_sys_mq_open
	PTR	sys_mq_unlink			/* 6235 */
	PTR	compat_sys_mq_timedsend
	PTR	compat_sys_mq_timedreceive
	PTR	compat_sys_mq_notify
	PTR	compat_sys_mq_getsetattr
	PTR	sys_ni_syscall			/* 6240, sys_vserver */
	PTR	compat_sys_waitid
	PTR	sys_ni_syscall			/* available, was setaltroot */
	PTR	sys_add_key
	PTR	sys_request_key
	PTR	compat_sys_keyctl		/* 6245 */
	PTR	sys_set_thread_area
	PTR	sys_inotify_init
	PTR	sys_inotify_add_watch
	PTR	sys_inotify_rm_watch
	PTR	compat_sys_migrate_pages	/* 6250 */
	PTR	sys_openat
	PTR	sys_mkdirat
	PTR	sys_mknodat
	PTR	sys_fchownat
	PTR	compat_sys_futimesat		/* 6255 */
	PTR	sys_newfstatat
	PTR	sys_unlinkat
	PTR	sys_renameat
	PTR	sys_linkat
	PTR	sys_symlinkat			/* 6260 */
	PTR	sys_readlinkat
	PTR	sys_fchmodat
	PTR	sys_faccessat
	PTR	compat_sys_pselect6
	PTR	compat_sys_ppoll		/* 6265 */
	PTR	sys_unshare
	PTR	sys_splice
	PTR	sys_sync_file_range
	PTR	sys_tee
	PTR	compat_sys_vmsplice		/* 6270 */
	PTR	compat_sys_move_pages
	PTR	compat_sys_set_robust_list
	PTR	compat_sys_get_robust_list
	PTR	compat_sys_kexec_load
	PTR	sys_getcpu			/* 6275 */
	PTR	compat_sys_epoll_pwait
	PTR	sys_ioprio_set
	PTR	sys_ioprio_get
	PTR	compat_sys_utimensat
	PTR	compat_sys_signalfd		/* 6280 */
	PTR	sys_ni_syscall			/* was timerfd */
	PTR	sys_eventfd
	PTR	sys_fallocate
	PTR	sys_timerfd_create
	PTR	compat_sys_timerfd_gettime	/* 6285 */
	PTR	compat_sys_timerfd_settime
	PTR	compat_sys_signalfd4
	PTR	sys_eventfd2
	PTR	sys_epoll_create1
	PTR	sys_dup3			/* 6290 */
	PTR	sys_pipe2
	PTR	sys_inotify_init1
	PTR	compat_sys_preadv
	PTR	compat_sys_pwritev
	PTR	compat_sys_rt_tgsigqueueinfo	/* 6295 */
	PTR	sys_perf_event_open
	PTR	sys_accept4
	PTR	compat_sys_recvmmsg
	PTR	sys_getdents64
	PTR	sys_fanotify_init		/* 6300 */
	PTR	sys_fanotify_mark
	PTR	sys_prlimit64
	PTR	sys_name_to_handle_at
	PTR	sys_open_by_handle_at
	PTR	compat_sys_clock_adjtime	/* 6305 */
	PTR	sys_syncfs
	PTR	compat_sys_sendmmsg
	PTR	sys_setns
	PTR	compat_sys_process_vm_readv
	PTR	compat_sys_process_vm_writev	/* 6310 */
	PTR	sys_kcmp
	PTR	sys_finit_module
	PTR	sys_sched_setattr
	PTR	sys_sched_getattr
	PTR	sys_renameat2			/* 6315 */
	PTR	sys_seccomp
	PTR	sys_getrandom
	PTR	sys_memfd_create
	PTR	sys_bpf
	PTR	compat_sys_execveat		/* 6320 */
	PTR	sys_userfaultfd
	PTR	sys_membarrier
	PTR	sys_mlock2
	PTR	sys_copy_file_range
	PTR	compat_sys_preadv2		/* 6325 */
	PTR	compat_sys_pwritev2
	PTR	sys_pkey_mprotect
	PTR	sys_pkey_alloc
	PTR	sys_pkey_free
	PTR	sys_statx			/* 6330 */
	.size	sysn32_call_table,.-sysn32_call_table
