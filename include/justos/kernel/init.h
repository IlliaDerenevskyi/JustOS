/*
 * init.h - Stage 2 orchestration.
 *
 * Purpose:
 *   Runs the Stage 2 subsystems in order and gives the result to the entry
 *   stub.
 *
 * Responsibilities:
 *   - jp_kernel_init: checks the handoff, then runs CPU, Memory, Paging,
 *     Stack, Heap, ELF; stops at the first failure and returns its result;
 *   - jp_kernel_stage2_failure: records the result in context->stage2_result
 *     and goes on to jp_kernel_stage2_complete; with a null context it stops
 *     the processor.
 *
 * Limitations:
 *   jp_kernel_stage2_complete is only declared here; Stage 4 implements it.
 *
 * Public API:
 *   jp_kernel_init, jp_kernel_stage2_failure, jp_kernel_stage2_complete.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3 behaviour, Stage 4 behaviour.
 */

#ifndef JUSTOS_KERNEL_INIT_H
#define JUSTOS_KERNEL_INIT_H

#include "justos/boot/boot_types.h"

JPBootResult jp_kernel_init(
	JPBootContext *context
);

_Noreturn void jp_kernel_stage2_failure(
	JPBootContext *context,
	JPBootResult result
);

/* Implemented by Stage 4 (boot_manager.c). */
_Noreturn void jp_kernel_stage2_complete(
	JPBootContext *context
);

#endif /* JUSTOS_KERNEL_INIT_H */
