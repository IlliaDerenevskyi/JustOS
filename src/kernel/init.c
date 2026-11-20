/*
 * init.c - Stage 2 orchestration.
 *
 * Purpose:
 *   Runs the Stage 2 subsystems in order and gives the result to the entry
 *   stub. The exact rules are in the Stage 2 section of milestone0.md
 *   ("Decided rules (checking the entry and a broken handoff)").
 *
 * Responsibilities:
 *   - jp_kernel_init: checks the handoff, runs CPU, Memory, Paging, Stack,
 *     Heap, ELF; stops at the first failure and returns its result; checks
 *     the completion state;
 *   - jp_kernel_stage2_failure: records the result in context->stage2_result
 *     and goes on to jp_kernel_stage2_complete; with a null context it stops
 *     the processor.
 *
 * Limitations:
 *   Writes only context->stage2_result. Does not roll anything back.
 *   jp_kernel_stage2_complete is declared here and implemented by Stage 4.
 *
 * Depends on:
 *   justos/kernel/{init,cpu,memory,paging,stack,heap,elf}.h, arch_dispatch.h,
 *   halt_hw.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3 behaviour, Stage 4 behaviour.
 */

#include "justos/kernel/init.h"

#include "arch_dispatch.h"
#include "halt_hw.h"
#include "justos/kernel/cpu.h"
#include "justos/kernel/elf.h"
#include "justos/kernel/heap.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"
#include "justos/kernel/stack.h"

/* ---- Private helpers ---- */

static bool handoff_is_ready(const JPBootContext *context)
{
	const JPHandoffInfo *handoff = &context->handoff;

	return context->current_stage == JP_BOOT_STAGE_READY &&
	       handoff->state == JP_HANDOFF_STATE_READY &&
	       handoff->ownership_transferred &&
	       handoff->interrupts_disabled &&
	       handoff->paging_enabled &&
	       JP_KERNEL_ARCH_IS_SUPPORTED(handoff->architecture);
}

/* ---- Public API ---- */

JPBootResult jp_kernel_init(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!handoff_is_ready(context)) {
		return JP_BOOT_FAILED;
	}

	result = jp_kernel_cpu_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_kernel_memory_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_kernel_paging_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_kernel_stack_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_kernel_heap_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_kernel_elf_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}

	/* The completion state: the ownership table and the active map agree with everything that was set up. */
	result = jp_kernel_memory_validate_ownership(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	return jp_kernel_paging_validate(context);
}

_Noreturn void jp_kernel_stage2_failure(JPBootContext *context, JPBootResult result)
{
	if (context == NULL) {
		jp_kernel_halt();
	}
	/* A failure is never recorded as success. */
	context->stage2_result = result == JP_BOOT_SUCCESS ? JP_BOOT_FAILED : result;
	jp_kernel_stage2_complete(context);
}
