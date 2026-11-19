/*
 * handoff.c - final Stage 1 handoff: readiness validation and preparation.
 *
 * Purpose:
 *   Registers the optional pre-transition interaction hook and prepares the
 *   Stage 1 to Stage 2 transition (jp_boot_prepare_handoff).
 *
 * Responsibilities:
 *   - check that every Stage 1 prerequisite is satisfied;
 *   - place the kernel stack, call the architecture steps, run the hook
 *     exactly once, ask the provider to prepare the platform handoff;
 *   - set JP_HANDOFF_STATE_READY and JP_BOOT_STAGE_READY only after every
 *     step has really succeeded.
 *
 * Limitations:
 *   A handoff is prepared once. The step order and the failure rules are in
 *   milestone0.md ("Decided rules (handoff preparation)"). No JPBootInfo, no
 *   ELF parsing, no kernel loading, no provider internals.
 *
 * Depends on:
 *   justos/boot/handoff.h, justos/boot/boot_environment.h,
 *   justos/boot/memory_map.h, and the architecture headers.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/handoff.h"

#include "justos/boot/architecture/aarch64.h"
#include "justos/boot/architecture/x86_64.h"
#include "justos/boot/boot_environment.h"
#include "justos/boot/memory_map.h"
#include "arch_dispatch.h"

/* ---- Public API ---- */

JPBootResult jp_boot_register_interaction(JPBootContext *context,
	JPBootInteractionHook hook, void *user_data);
JPBootResult jp_boot_prepare_handoff(JPBootContext *context);

/* ---- Private helpers: architecture dispatch ---- */

static JPBootResult arch_setup_paging(JPBootContext *context)
{
	return JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_setup_paging(context), jp_aarch64_setup_paging(context));
}

static JPBootResult arch_prepare_entry(JPBootContext *context)
{
	return JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_prepare_entry(context), jp_aarch64_prepare_entry(context));
}

static JPBootResult arch_validate_entry_state(const JPBootContext *context)
{
	return JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_validate_entry_state(context), jp_aarch64_validate_entry_state(context));
}

/* ---- Private helpers ---- */

static JPBootResult fail_handoff(JPBootContext *context, JPBootResult result)
{
	context->handoff.state = JP_HANDOFF_STATE_FAILED;
	return result;
}

static bool all_segments_are_permissioned(const JPKernelInfo *kernel)
{
	size_t i;

	for (i = 0; i < kernel->segment_count; i++) {
		if (kernel->segments[i].state != JP_KERNEL_SEGMENT_PERMISSIONED) {
			return false;
		}
	}
	return true;
}

/* Every prerequisite that is checked before anything is built. */
static bool prerequisites_are_satisfied(const JPBootContext *context)
{
	bool architecture_known = JP_ARCH_IS_SUPPORTED(context->handoff.architecture);
	/* A machine without a framebuffer can still start: Stage 3 has a text fallback. */
	bool framebuffer_ok = context->framebuffer.state == JP_DETECTION_STATE_DETECTED ||
		context->framebuffer.state == JP_DETECTION_STATE_NOT_DETECTED;

	return architecture_known &&
	       context->environment.state == JP_DETECTION_STATE_DETECTED &&
	       context->hardware.overall_state == JP_DETECTION_STATE_DETECTED &&
	       context->memory_map.state == JP_DETECTION_STATE_DETECTED &&
	       framebuffer_ok &&
	       context->kernel.image.state == JP_DETECTION_STATE_DETECTED &&
	       context->kernel.validation_state == JP_KERNEL_VALIDATION_VALID &&
	       context->kernel.load_state == JP_KERNEL_LOAD_LOADED &&
	       all_segments_are_permissioned(&context->kernel);
}

static uint64_t align_up_page(uint64_t value)
{
	return (value + JP_BOOT_PAGE_SIZE - 1) / JP_BOOT_PAGE_SIZE * JP_BOOT_PAGE_SIZE;
}

/* The end of the highest kernel segment in physical memory. */
static uint64_t end_of_kernel_segments(const JPBootContext *context)
{
	uint64_t end = 0;
	size_t i;

	for (i = 0; i < context->kernel.segment_count; i++) {
		const JPKernelSegment *segment = &context->kernel.segments[i];
		uint64_t segment_end = segment->physical_address + segment->memory_size;

		if (segment_end > end) {
			end = segment_end;
		}
	}
	return end;
}

/*
 * Places the kernel stack: the same rule as for kernel segments (usable
 * memory in map order, above the protected low memory, page aligned, not on the
 * kernel image), but behind the highest kernel segment. On success the highest
 * address of the stack is stored in handoff.stack_pointer.
 */
static bool place_kernel_stack(JPBootContext *context)
{
	uint64_t image_start = (uint64_t)(uintptr_t)context->kernel.image.buffer;
	uint64_t image_end = image_start + context->kernel.image.size;
	uint64_t after_segments = align_up_page(end_of_kernel_segments(context));
	size_t i;

	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		uint64_t region_end;
		uint64_t start;

		if (region->type != JP_MEMORY_REGION_USABLE ||
		    region->length > UINT64_MAX - region->base_address) {
			continue;
		}
		region_end = region->base_address + region->length;
		start = region->base_address;
		if (start < JP_BOOT_PROTECTED_LOW_MEMORY) {
			start = JP_BOOT_PROTECTED_LOW_MEMORY;
		}
		if (start < after_segments) {
			start = after_segments;
		}
		start = align_up_page(start);
		if (start < image_end && image_start < start + JP_BOOT_KERNEL_STACK_SIZE) {
			start = align_up_page(image_end);
		}
		if (start > region_end || JP_BOOT_KERNEL_STACK_SIZE > region_end - start) {
			continue;
		}
		context->handoff.stack_pointer = start + JP_BOOT_KERNEL_STACK_SIZE;
		return true;
	}
	return false;
}

static JPBootResult run_architecture_steps(JPBootContext *context)
{
	JPBootResult result = arch_setup_paging(context);

	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = arch_prepare_entry(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	return arch_validate_entry_state(context);
}

static uint64_t stack_alignment(JPArchitectureType architecture)
{
	(void)architecture;    /* unused in a build for one architecture */
	return JP_ARCH_DISPATCH(architecture, JP_X86_64_STACK_ALIGNMENT, JP_AARCH64_STACK_ALIGNMENT);
}

/*
 * The final check after the platform step: the normalized handoff fields must
 * still describe the architecture entry state (the hook and the platform step
 * ran after the architecture check).
 */
static bool resulting_handoff_is_valid(const JPBootContext *context, JPArchitectureType architecture)
{
	const JPHandoffInfo *handoff = &context->handoff;

	return handoff->architecture == architecture &&
	       handoff->kernel_entry == context->kernel.entry_point &&
	       handoff->page_table_root != 0 &&
	       handoff->paging_enabled &&
	       handoff->interrupts_disabled &&
	       handoff->stack_pointer != 0 &&
	       handoff->stack_pointer % stack_alignment(architecture) == 0;
}

/* Runs the registered hook exactly once; with no hook the step is skipped. */
static JPBootResult run_interaction_hook(JPBootContext *context)
{
	JPBootResult result;

	if (context->interaction.state != JP_BOOT_INTERACTION_REGISTERED ||
	    context->interaction.hook == NULL) {
		return JP_BOOT_SUCCESS;
	}
	result = context->interaction.hook(context, context->interaction.user_data);
	context->interaction.state = result == JP_BOOT_SUCCESS
		? JP_BOOT_INTERACTION_COMPLETED
		: JP_BOOT_INTERACTION_FAILED;
	return result;
}

/* ---- Implementation ---- */

JPBootResult jp_boot_register_interaction(JPBootContext *context,
	JPBootInteractionHook hook, void *user_data)
{
	if (context == NULL || hook == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->interaction.state != JP_BOOT_INTERACTION_NOT_REGISTERED ||
	    context->handoff.state != JP_HANDOFF_STATE_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
	context->interaction.hook = hook;
	context->interaction.user_data = user_data;
	context->interaction.state = JP_BOOT_INTERACTION_REGISTERED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_prepare_handoff(JPBootContext *context)
{
	JPBootResult result;
	JPArchitectureType architecture;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->handoff.state != JP_HANDOFF_STATE_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
	architecture = context->handoff.architecture;
	context->current_stage = JP_BOOT_STAGE_HANDOFF_PREPARATION;

	if (!prerequisites_are_satisfied(context)) {
		return fail_handoff(context, JP_BOOT_FAILED);
	}
	if (!place_kernel_stack(context)) {
		return fail_handoff(context, JP_BOOT_FAILED);
	}
	/* The stack belongs to the kernel environment: tell the firmware. */
	if (jp_boot_environment_reserve_memory(context,
		context->handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE,
		JP_BOOT_KERNEL_STACK_SIZE) != JP_BOOT_SUCCESS) {
		return fail_handoff(context, JP_BOOT_FAILED);
	}
	result = run_architecture_steps(context);
	if (result != JP_BOOT_SUCCESS) {
		return fail_handoff(context, result);
	}
	context->handoff.state = JP_HANDOFF_STATE_PREREQUISITES_VALID;

	result = run_interaction_hook(context);
	if (result != JP_BOOT_SUCCESS) {
		return fail_handoff(context, result);
	}

	result = jp_boot_environment_prepare_handoff(context);
	if (result != JP_BOOT_SUCCESS) {
		return fail_handoff(context, result);
	}
	context->handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED;

	/* The final memory map replaced the early one in the platform step. */
	if (jp_boot_validate_memory_map(context) != JP_BOOT_SUCCESS ||
	    !resulting_handoff_is_valid(context, architecture)) {
		return fail_handoff(context, JP_BOOT_FAILED);
	}

	context->handoff.ownership_transferred = true;
	context->handoff.state = JP_HANDOFF_STATE_READY;
	context->current_stage = JP_BOOT_STAGE_READY;
	return JP_BOOT_SUCCESS;
}
