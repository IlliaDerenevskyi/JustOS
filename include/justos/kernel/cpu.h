/*
 * cpu.h - Stage 2 CPU initialization.
 *
 * Purpose:
 *   Checks the processor state that Stage 1 handed over and sets the few
 *   control bits that Stage 2 needs.
 *
 * Responsibilities:
 *   - x86_64: long mode, privilege level 0, IF = 0, "no execute" support;
 *     sets EFER.NXE and CR0.WP;
 *   - AArch64: EL1, DAIF = 1111, SCTLR_EL1.M = 1;
 *   - reads every register back and reports success only if it is right.
 *
 * Limitations:
 *   Does not build page tables, touch the stack, allocate memory, or access
 *   firmware. Does not turn on SSE or the floating-point unit.
 *
 * Public API:
 *   jp_kernel_cpu_initialize.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_CPU_H
#define JUSTOS_KERNEL_CPU_H

#include "justos/boot/boot_types.h"

/*
 * JP_BOOT_ERROR_INVALID_PARAM for a null context; JP_BOOT_FAILED when a
 * mandatory processor state is missing or wrong.
 */
JPBootResult jp_kernel_cpu_initialize(
	JPBootContext *context
);

#endif /* JUSTOS_KERNEL_CPU_H */
