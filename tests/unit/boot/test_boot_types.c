/*
 * Unit test (level U) of include/justos/boot/boot_types.h.
 * Checks the contract values of Stage 1 types and constants, and that a
 * zero-initialized context reads as "nothing happened yet" (Rule 30).
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_types.h"

/* Constants fixed by the specification. */
_Static_assert(JP_MAX_MEMORY_REGIONS == 128, "memory region capacity");
_Static_assert(JP_MAX_KERNEL_SEGMENTS == 16, "kernel segment capacity");
_Static_assert(JP_BOOT_PAGE_SIZE == 4096, "page size");
_Static_assert(JP_BOOT_PROTECTED_LOW_MEMORY == 0x100000, "protected low memory");
_Static_assert(JP_BOOT_KERNEL_STACK_SIZE == 16384, "kernel stack size");
_Static_assert(JP_ELF_MAGIC == 0x464C457F, "ELF magic");
_Static_assert(JP_ELF_CLASS_64 == 2, "ELF class 64");
_Static_assert(JP_ELF_TYPE_EXEC == 2, "ELF type EXEC");
_Static_assert(JP_ELF_TYPE_DYN == 3, "ELF type DYN");
_Static_assert(JP_ELF_PT_LOAD == 1, "ELF PT_LOAD");
_Static_assert(JP_ELF_MACHINE_X86_64 == 62, "EM_X86_64");
_Static_assert(JP_ELF_MACHINE_AARCH64 == 183, "EM_AARCH64");

/* The zero value of every state enumeration is the "nothing known" state. */
_Static_assert(JP_BOOT_SUCCESS == 0, "success is zero");
_Static_assert(JP_BOOT_STAGE_UNINITIALIZED == 0, "stage zero");
_Static_assert(JP_DETECTION_STATE_UNKNOWN == 0, "detection zero");
_Static_assert(JP_BOOT_ENVIRONMENT_UNKNOWN == 0, "environment zero");
_Static_assert(JP_ARCH_UNKNOWN == 0, "architecture zero");
_Static_assert(JP_MEMORY_REGION_UNKNOWN == 0, "region type zero");
_Static_assert(JP_FRAMEBUFFER_FORMAT_UNKNOWN == 0, "format zero");
_Static_assert(JP_KERNEL_VALIDATION_UNKNOWN == 0, "validation zero");
_Static_assert(JP_KERNEL_LOAD_UNKNOWN == 0, "load zero");
_Static_assert(JP_HANDOFF_STATE_UNINITIALIZED == 0, "handoff zero");
_Static_assert(JP_BOOT_INPUT_NONE == 0, "input kind zero");
_Static_assert(JP_BOOT_INTERACTION_NOT_REGISTERED == 0, "interaction zero");

static void test_enum_order(void)
{
	assert(JP_BOOT_NOT_INITIALIZED == 1);
	assert(JP_BOOT_NOT_DETECTED == 2);
	assert(JP_BOOT_UNKNOWN == 3);
	assert(JP_BOOT_FAILED == 4);
	assert(JP_BOOT_ERROR_INVALID_PARAM == 5);
	assert(JP_BOOT_ERROR_CORRUPTED_DATA == 6);

	assert(JP_BOOT_STAGE_READY == JP_BOOT_STAGE_HANDOFF_PREPARATION + 1);
	assert(JP_DETECTION_STATE_DETECTED == 3);
	assert(JP_DETECTION_STATE_FAILED == 4);
	assert(JP_HANDOFF_STATE_READY == 3);
}

static void test_zeroed_context_claims_nothing(void)
{
	JPBootContext context;

	memset(&context, 0, sizeof context);

	assert(context.current_stage == JP_BOOT_STAGE_UNINITIALIZED);
	assert(context.environment.state == JP_DETECTION_STATE_UNKNOWN);
	assert(context.environment.platform_context == NULL);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_UNKNOWN);
	assert(context.memory_map.state == JP_DETECTION_STATE_UNKNOWN);
	assert(context.memory_map.region_count == 0);
	assert(context.framebuffer.state == JP_DETECTION_STATE_UNKNOWN);
	assert(context.kernel.validation_state == JP_KERNEL_VALIDATION_UNKNOWN);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_UNKNOWN);
	assert(context.kernel.segment_count == 0);
	assert(context.handoff.state == JP_HANDOFF_STATE_UNINITIALIZED);
	assert(!context.handoff.ownership_transferred);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);
	assert(context.interaction.hook == NULL);
	assert(context.stage2_result == JP_BOOT_SUCCESS);
}

static void test_capacities_hold_data(void)
{
	JPBootContext context;

	memset(&context, 0, sizeof context);
	context.memory_map.region_count = JP_MAX_MEMORY_REGIONS;
	context.memory_map.regions[JP_MAX_MEMORY_REGIONS - 1].length = 1;
	context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS;
	context.kernel.segments[JP_MAX_KERNEL_SEGMENTS - 1].memory_size = 1;

	assert(context.memory_map.regions[JP_MAX_MEMORY_REGIONS - 1].length == 1);
	assert(context.kernel.segments[JP_MAX_KERNEL_SEGMENTS - 1].memory_size == 1);
}

int main(void)
{
	test_enum_order();
	test_zeroed_context_claims_nothing();
	test_capacities_hold_data();
	return 0;
}
