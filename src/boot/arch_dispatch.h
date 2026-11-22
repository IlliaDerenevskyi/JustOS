/*
 * arch_dispatch.h - choosing the architecture operation in generic modules.
 *
 * Purpose:
 *   Generic Stage 1 modules call an architecture operation by the value of
 *   handoff.architecture. A boot application is built for one architecture and
 *   must not need the functions of the other: JP_BOOT_ONLY_X86_64 or
 *   JP_BOOT_ONLY_AARCH64 removes the other call. Private to src/boot.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 */

#ifndef JUSTOS_BOOT_ARCH_DISPATCH_H
#define JUSTOS_BOOT_ARCH_DISPATCH_H

#include "justos/boot/boot_types.h"

#if defined(JP_BOOT_ONLY_X86_64)
#define JP_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) (x86_64_call)
#define JP_ARCH_IS_SUPPORTED(architecture) ((architecture) == JP_ARCH_X86_64)
#elif defined(JP_BOOT_ONLY_AARCH64)
#define JP_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) (aarch64_call)
#define JP_ARCH_IS_SUPPORTED(architecture) ((architecture) == JP_ARCH_AARCH64)
#else
#define JP_ARCH_DISPATCH(architecture, x86_64_call, aarch64_call) \
	((architecture) == JP_ARCH_X86_64 ? (x86_64_call) : (aarch64_call))
#define JP_ARCH_IS_SUPPORTED(architecture) \
	((architecture) == JP_ARCH_X86_64 || (architecture) == JP_ARCH_AARCH64)
#endif

#endif /* JUSTOS_BOOT_ARCH_DISPATCH_H */
