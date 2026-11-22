/*
 * arch_dispatch.h - choosing the architecture code in the Stage 2 modules.
 *
 * Purpose:
 *   Stage 2 modules choose the architecture part by handoff.architecture. A
 *   kernel is built for one architecture and must not need the register
 *   functions of the other: JP_KERNEL_ONLY_X86_64 or JP_KERNEL_ONLY_AARCH64
 *   removes the other call. Private to src/kernel.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 */

#ifndef JUSTOS_SRC_KERNEL_ARCH_DISPATCH_H
#define JUSTOS_SRC_KERNEL_ARCH_DISPATCH_H

#include "justos/boot/boot_types.h"

#if defined(JP_KERNEL_ONLY_X86_64)
#define JP_KERNEL_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) (x86_64_call)
#define JP_KERNEL_ARCH_IS_SUPPORTED(architecture) ((architecture) == JP_ARCH_X86_64)
#elif defined(JP_KERNEL_ONLY_AARCH64)
#define JP_KERNEL_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) (aarch64_call)
#define JP_KERNEL_ARCH_IS_SUPPORTED(architecture) ((architecture) == JP_ARCH_AARCH64)
#else
#define JP_KERNEL_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) \
	((architecture) == JP_ARCH_X86_64 ? (x86_64_call) : (aarch64_call))
#define JP_KERNEL_ARCH_IS_SUPPORTED(architecture) \
	((architecture) == JP_ARCH_X86_64 || (architecture) == JP_ARCH_AARCH64)
#endif

#endif /* JUSTOS_SRC_KERNEL_ARCH_DISPATCH_H */
