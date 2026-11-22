/*
 * stack.c - Stage 2 stack validation.
 *
 * Purpose:
 *   Checks the stack that Stage 1 prepared; Stage 2 keeps using it. The exact
 *   steps are in "Decided rules (what is checked and where the range comes
 *   from)" of milestone0.md.
 *
 * Responsibilities:
 *   - the KERNEL_STACK range exists and has exactly the Stage 1 size;
 *   - the top and the current stack pointer are aligned as the ABI says and
 *     the pointer is inside the range;
 *   - the range is read-write and not executable in the active map.
 *
 * Limitations:
 *   Never changes the stack pointer and never switches stacks. Allocates
 *   nothing.
 *
 * Depends on:
 *   justos/kernel/stack.h, memory.h, paging.h, arch_dispatch.h, stack_hw.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/stack.h"

#include "arch_dispatch.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"
#include "stack_hw.h"

#define JP_STACK_TOP_ALIGNMENT 16ULL        /* the stack top, both architectures */
#define JP_STACK_POINTER_ALIGNMENT_X86_64 8ULL   /* inside a function: a call pushed 8 bytes */
#define JP_STACK_POINTER_ALIGNMENT_AARCH64 16ULL

/* True when the byte at `address` is owned as the stack. */
static bool byte_is_stack(const JPBootContext *context, uint64_t address)
{
	JPKernelMemoryOwner owner;

	return jp_kernel_memory_query(context, address, 1, &owner) == JP_BOOT_SUCCESS &&
	       owner == JP_KERNEL_MEMORY_KERNEL_STACK;
}

/* The owned stack range is exactly [base, top). */
static bool stack_range_is_exact(const JPBootContext *context, uint64_t base, uint64_t top)
{
	JPKernelMemoryOwner owner;

	if (jp_kernel_memory_query(context, base, JP_BOOT_KERNEL_STACK_SIZE, &owner) != JP_BOOT_SUCCESS ||
	    owner != JP_KERNEL_MEMORY_KERNEL_STACK) {
		return false;
	}
	return !byte_is_stack(context, base - 1) && !byte_is_stack(context, top);
}

JPBootResult jp_kernel_stack_initialize(JPBootContext *context)
{
	JPArchitectureType architecture;
	uint64_t top;
	uint64_t base;
	uint64_t pointer;
	uint64_t alignment;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage2.entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	architecture = context->handoff.architecture;
	if (!JP_KERNEL_ARCH_IS_SUPPORTED(architecture)) {
		return JP_BOOT_FAILED;
	}
	top = context->handoff.stack_pointer;
	if (top < JP_BOOT_KERNEL_STACK_SIZE || top % JP_STACK_TOP_ALIGNMENT != 0) {
		return JP_BOOT_FAILED;
	}
	base = top - JP_BOOT_KERNEL_STACK_SIZE;
	if (!stack_range_is_exact(context, base, top)) {
		return JP_BOOT_FAILED;
	}

	pointer = jp_kernel_stack_current_pointer();
	alignment = architecture == JP_ARCH_X86_64 ? JP_STACK_POINTER_ALIGNMENT_X86_64 : JP_STACK_POINTER_ALIGNMENT_AARCH64;
	if (pointer < base || pointer > top || pointer % alignment != 0) {
		return JP_BOOT_FAILED;
	}
	if (jp_kernel_paging_validate(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}
