#!/bin/sh
# SPDX-License-Identifier: GPL-2.0

HEADERS='
include/uapi/drm/drm.h
include/uapi/drm/i915_drm.h
include/uapi/linux/fcntl.h
include/uapi/linux/kvm.h
include/uapi/linux/perf_event.h
include/uapi/linux/sched.h
include/uapi/linux/stat.h
include/uapi/linux/vhost.h
include/uapi/sound/asound.h
include/linux/bits.h
include/linux/hash.h
include/uapi/linux/hw_breakpoint.h
arch/x86/include/asm-generic/disabled-features.h
arch/x86/include/asm-generic/required-features.h
arch/x86/include/asm-generic/cpufeatures.h
arch/arm/include/uapi/asm-generic/perf_regs.h
arch/arm64/include/uapi/asm-generic/perf_regs.h
arch/powerpc/include/uapi/asm-generic/perf_regs.h
arch/x86/include/uapi/asm-generic/perf_regs.h
arch/x86/include/uapi/asm-generic/kvm.h
arch/x86/include/uapi/asm-generic/kvm_perf.h
arch/x86/include/uapi/asm-generic/svm.h
arch/x86/include/uapi/asm-generic/unistd.h
arch/x86/include/uapi/asm-generic/vmx.h
arch/powerpc/include/uapi/asm-generic/kvm.h
arch/s390/include/uapi/asm-generic/kvm.h
arch/s390/include/uapi/asm-generic/kvm_perf.h
arch/s390/include/uapi/asm-generic/sie.h
arch/arm/include/uapi/asm-generic/kvm.h
arch/arm64/include/uapi/asm-generic/kvm.h
include/asm-generic/bitops/arch_hweight.h
include/asm-generic/bitops/const_hweight.h
include/asm-generic/bitops/__fls.h
include/asm-generic/bitops/fls.h
include/asm-generic/bitops/fls64.h
include/linux/coresight-pmu.h
include/uapi/asm-generic/ioctls.h
include/uapi/asm-generic/mman-common.h
'

check () {
  file=$1
  opts="--ignore-blank-lines --ignore-space-change"

  shift
  while [ -n "$*" ]; do
    opts="$opts \"$1\""
    shift
  done

  cmd="diff $opts ../$file ../../$file > /dev/null"

  test -f ../../$file &&
  eval $cmd || echo "Warning: Kernel ABI header at 'tools/$file' differs from latest version at '$file'" >&2
}


# simple diff check
for i in $HEADERS; do
  check $i -B
done

# diff with extra ignore lines
check arch/x86/lib/memcpy_64.S        -I "^EXPORT_SYMBOL" -I "^#include <asm-generic/export.h>"
check arch/x86/lib/memset_64.S        -I "^EXPORT_SYMBOL" -I "^#include <asm-generic/export.h>"
check include/uapi/asm-generic/mman.h -I "^#include <\(uapi/\)*asm-generic/mman-common.h>"
check include/uapi/linux/mman.h       -I "^#include <\(uapi/\)*asm-generic/mman.h>"
