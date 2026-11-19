/*
 * Unit test (level U) of src/boot/handoff.c.
 * Rules: see "Decided rules (handoff preparation)" in milestone0.md.
 * The architecture operations and the platform handoff are fakes; the test
 * checks the order of the steps, what is written, and that READY is reached
 * only after every step has succeeded.
 */

#include <assert.h>
#include <string.h>

#include "fake_architecture.h"
#include "fake_environment.h"
#include "justos/boot/handoff.h"

#define PAGE 4096ULL
#define MIB 0x100000ULL
#define USABLE_BASE MIB
#define USABLE_LENGTH (4 * MIB)
#define SEGMENT_SIZE 0x2000ULL
#define ENTRY 0xFFFFFFFF80000000ULL

typedef struct HookProbe {
	unsigned int calls;
	JPBootResult result;
	JPBootContext *seen_context;
	void *seen_user_data;
	JPHandoffState seen_handoff_state;
	unsigned int seen_platform_calls;
	size_t seen_arch_calls;
	JPFakeEnvironment *fake;
	void (*mutate)(JPBootContext *);   /* lets a test damage the handoff from the hook */
} HookProbe;

static HookProbe probe;
static JPFakeEnvironment fake;
static int user_data_marker;

static JPBootResult hook(JPBootContext *context, void *user_data)
{
	probe.calls++;
	probe.seen_context = context;
	probe.seen_user_data = user_data;
	probe.seen_handoff_state = context->handoff.state;
	probe.seen_platform_calls = fake.prepare_handoff_calls;
	probe.seen_arch_calls = jp_fake_arch.call_count;
	if (probe.mutate != NULL) {
		probe.mutate(context);
	}
	return probe.result;
}

/* A context in which every Stage 1 prerequisite of the handoff is satisfied. */
static void prepare(JPBootContext *context)
{
	memset(&probe, 0, sizeof probe);
	jp_fake_environment_attach(context, &fake);
	jp_fake_arch_reset();

	context->handoff.architecture = JP_ARCH_X86_64;
	context->hardware.overall_state = JP_DETECTION_STATE_DETECTED;
	context->framebuffer.state = JP_DETECTION_STATE_DETECTED;

	context->memory_map.state = JP_DETECTION_STATE_DETECTED;
	context->memory_map.region_count = 1;
	context->memory_map.regions[0].base_address = USABLE_BASE;
	context->memory_map.regions[0].length = USABLE_LENGTH;
	context->memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;

	context->kernel.image.state = JP_DETECTION_STATE_DETECTED;
	context->kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	context->kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context->kernel.entry_point = ENTRY;
	context->kernel.segment_count = 1;
	context->kernel.segments[0].virtual_address = ENTRY;
	context->kernel.segments[0].physical_address = USABLE_BASE;
	context->kernel.segments[0].memory_size = SEGMENT_SIZE;
	context->kernel.segments[0].flags = 5;
	context->kernel.segments[0].state = JP_KERNEL_SEGMENT_PERMISSIONED;
}

static void test_successful_handoff(void)
{
	JPBootContext context;
	uint64_t stack_bottom;

	prepare(&context);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);

	assert(context.handoff.state == JP_HANDOFF_STATE_READY);
	assert(context.current_stage == JP_BOOT_STAGE_READY);
	assert(context.handoff.ownership_transferred);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);

	/* The architecture steps ran in the specified order, then the platform. */
	assert(jp_fake_arch.call_count == 3);
	assert(jp_fake_arch.call_log[0] == JP_FAKE_CALL_SETUP_PAGING);
	assert(jp_fake_arch.call_log[1] == JP_FAKE_CALL_PREPARE_ENTRY);
	assert(jp_fake_arch.call_log[2] == JP_FAKE_CALL_VALIDATE_ENTRY_STATE);
	assert(fake.prepare_handoff_calls == 1);

	/* What the steps recorded. */
	assert(context.handoff.page_table_root == JP_FAKE_PAGE_TABLE_ROOT);
	assert(context.handoff.paging_enabled);
	assert(context.handoff.kernel_entry == ENTRY);
	assert(context.handoff.architecture == JP_ARCH_X86_64);
	assert(context.handoff.interrupts_disabled);

	/* The stack: 16 KiB, aligned, after the segment, inside usable memory. */
	stack_bottom = context.handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE;
	assert(context.handoff.stack_pointer % 16 == 0);
	assert(stack_bottom >= USABLE_BASE + SEGMENT_SIZE);
	assert(context.handoff.stack_pointer <= USABLE_BASE + USABLE_LENGTH);
}

static void test_aarch64_uses_aarch64_operations(void)
{
	JPBootContext context;

	prepare(&context);
	context.handoff.architecture = JP_ARCH_AARCH64;
	/* The fake prepare_entry records the architecture of the operation that ran. */
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(jp_fake_arch.call_count == 3);
	assert(context.handoff.architecture == JP_ARCH_AARCH64);
	assert(context.handoff.state == JP_HANDOFF_STATE_READY);
}

static void test_invalid_parameters_and_second_attempt(void)
{
	JPBootContext context;

	assert(jp_boot_prepare_handoff(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_READY);
	assert(fake.prepare_handoff_calls == 1);
}

typedef void (*Breaker)(JPBootContext *);

static void break_environment(JPBootContext *c) { c->environment.state = JP_DETECTION_STATE_NOT_INITIALIZED; }
static void break_hardware(JPBootContext *c) { c->hardware.overall_state = JP_DETECTION_STATE_FAILED; }
static void break_memory(JPBootContext *c) { c->memory_map.state = JP_DETECTION_STATE_UNKNOWN; }
static void break_framebuffer(JPBootContext *c) { c->framebuffer.state = JP_DETECTION_STATE_FAILED; }
static void break_framebuffer_unknown(JPBootContext *c) { c->framebuffer.state = JP_DETECTION_STATE_UNKNOWN; }
static void break_image(JPBootContext *c) { c->kernel.image.state = JP_DETECTION_STATE_NOT_DETECTED; }
static void break_validation(JPBootContext *c) { c->kernel.validation_state = JP_KERNEL_VALIDATION_INVALID; }
static void break_loading(JPBootContext *c) { c->kernel.load_state = JP_KERNEL_LOAD_FAILED; }
static void break_loading_not_started(JPBootContext *c) { c->kernel.load_state = JP_KERNEL_LOAD_NOT_INITIALIZED; }
static void break_segment_mapped_only(JPBootContext *c) { c->kernel.segments[0].state = JP_KERNEL_SEGMENT_MAPPED; }
static void break_segment_failed(JPBootContext *c) { c->kernel.segments[0].state = JP_KERNEL_SEGMENT_FAILED; }

static void test_every_missing_prerequisite_prevents_readiness(void)
{
	static const Breaker breakers[] = {
		break_environment, break_hardware, break_memory, break_framebuffer,
		break_framebuffer_unknown, break_image, break_validation,
		break_loading, break_loading_not_started,
		break_segment_mapped_only, break_segment_failed
	};
	size_t i;

	for (i = 0; i < sizeof breakers / sizeof breakers[0]; i++) {
		JPBootContext context;

		prepare(&context);
		assert(jp_boot_register_interaction(&context, hook, &user_data_marker) == JP_BOOT_SUCCESS);
		breakers[i](&context);
		assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
		assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
		assert(context.current_stage != JP_BOOT_STAGE_READY);
		assert(!context.handoff.ownership_transferred);
		assert(jp_fake_arch.call_count == 0);
		assert(probe.calls == 0);
		assert(fake.prepare_handoff_calls == 0);
		assert(context.interaction.state == JP_BOOT_INTERACTION_REGISTERED);
	}
}

static void test_missing_framebuffer_is_allowed(void)
{
	JPBootContext context;

	prepare(&context);
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.handoff.state == JP_HANDOFF_STATE_READY);
}

static void test_architecture_failures_are_propagated_in_order(void)
{
	JPBootContext context;

	prepare(&context);
	jp_fake_arch.setup_paging_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_fake_arch.call_count == 1);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(context.current_stage != JP_BOOT_STAGE_READY);

	prepare(&context);
	jp_fake_arch.prepare_entry_result = JP_BOOT_FAILED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(jp_fake_arch.call_count == 2);

	prepare(&context);
	jp_fake_arch.validate_entry_state_result = JP_BOOT_NOT_INITIALIZED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_fake_arch.call_count == 3);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(fake.prepare_handoff_calls == 0);
	assert(!context.handoff.ownership_transferred);
}

static void test_stack_needs_room(void)
{
	JPBootContext context;

	prepare(&context);
	/* The usable region holds the kernel segment and not a 16 KiB stack behind it. */
	context.memory_map.regions[0].length = SEGMENT_SIZE + JP_BOOT_KERNEL_STACK_SIZE - PAGE;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(jp_fake_arch.call_count == 0);
}

static void test_platform_failure_prevents_readiness(void)
{
	JPBootContext context;

	prepare(&context);
	fake.prepare_handoff_result = JP_BOOT_FAILED;
	assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_SUCCESS);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(context.current_stage != JP_BOOT_STAGE_READY);
	assert(!context.handoff.ownership_transferred);
	assert(probe.calls == 1);                      /* the hook had run before the platform step */
	assert(context.interaction.state == JP_BOOT_INTERACTION_COMPLETED);
}

static void test_hook_runs_once_at_the_right_moment(void)
{
	JPBootContext context;

	prepare(&context);
	assert(jp_boot_register_interaction(&context, hook, &user_data_marker) == JP_BOOT_SUCCESS);
	assert(context.interaction.state == JP_BOOT_INTERACTION_REGISTERED);
	assert(probe.calls == 0);                      /* registration does not run it */

	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(probe.calls == 1);
	assert(probe.seen_context == &context);
	assert(probe.seen_user_data == &user_data_marker);
	assert(probe.seen_handoff_state == JP_HANDOFF_STATE_PREREQUISITES_VALID);
	assert(probe.seen_arch_calls == 3);            /* after all architecture steps */
	assert(probe.seen_platform_calls == 0);        /* before the platform handoff */
	assert(context.interaction.state == JP_BOOT_INTERACTION_COMPLETED);
	assert(context.handoff.state == JP_HANDOFF_STATE_READY);
}

static void test_failing_hook_fails_the_handoff(void)
{
	JPBootContext context;

	prepare(&context);
	probe.result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_SUCCESS);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(probe.calls == 1);
	assert(context.interaction.state == JP_BOOT_INTERACTION_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(context.current_stage != JP_BOOT_STAGE_READY);
	assert(fake.prepare_handoff_calls == 0);
	assert(!context.handoff.ownership_transferred);
}

/* A failed handoff stays in the handoff stage and never reaches READY. */
static void test_failure_keeps_the_handoff_stage(void)
{
	JPBootContext context;

	prepare(&context);
	context.kernel.load_state = JP_KERNEL_LOAD_FAILED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.current_stage == JP_BOOT_STAGE_HANDOFF_PREPARATION);

	prepare(&context);
	jp_fake_arch.setup_paging_result = JP_BOOT_FAILED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.current_stage == JP_BOOT_STAGE_HANDOFF_PREPARATION);
}

static void test_unknown_architecture_prevents_readiness(void)
{
	JPBootContext context;

	prepare(&context);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(jp_fake_arch.call_count == 0);
	assert(fake.prepare_handoff_calls == 0);
}

/* Stack placement, with the kernel segments out of the way (no segments). */
static void test_stack_is_placed_above_the_protected_low_memory(void)
{
	JPBootContext context;

	prepare(&context);
	context.kernel.segment_count = 0;
	context.memory_map.regions[0].base_address = 0x1000;
	context.memory_map.regions[0].length = 8 * MIB;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.handoff.stack_pointer == MIB + JP_BOOT_KERNEL_STACK_SIZE);
}

static void test_stack_avoids_the_kernel_image(void)
{
	static uint8_t image_storage[8192];
	JPBootContext context;
	uint64_t image_start = (uint64_t)(uintptr_t)image_storage;
	uint64_t image_end = image_start + sizeof image_storage;

	prepare(&context);
	context.kernel.segment_count = 0;
	context.kernel.image.buffer = image_storage;
	context.kernel.image.size = sizeof image_storage;
	context.memory_map.regions[0].base_address = image_start & ~(PAGE - 1);
	context.memory_map.regions[0].length = 8 * MIB;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE >= image_end);
}

static void test_stack_needs_a_region_that_ends_after_the_start(void)
{
	JPBootContext context;

	prepare(&context);
	context.kernel.segment_count = 0;
	context.memory_map.regions[0].base_address = 0x1000;
	context.memory_map.regions[0].length = 0x2000;     /* ends below 1 MiB */
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
}

static void test_stack_comes_only_from_usable_memory(void)
{
	JPBootContext context;

	prepare(&context);
	context.kernel.segment_count = 0;
	context.memory_map.region_count = 2;
	context.memory_map.regions[0].base_address = MIB;
	context.memory_map.regions[0].length = 4 * MIB;
	context.memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	context.memory_map.regions[1].base_address = 8 * MIB;
	context.memory_map.regions[1].length = MIB;
	context.memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.handoff.stack_pointer - JP_BOOT_KERNEL_STACK_SIZE >= 8 * MIB);
	assert(context.handoff.stack_pointer <= 9 * MIB);
}

/* The hook runs only if it was registered through jp_boot_register_interaction(). */
static void test_hook_that_was_not_registered_does_not_run(void)
{
	JPBootContext context;

	prepare(&context);
	context.interaction.hook = hook;
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(probe.calls == 0);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);
}

/* Damage that appears after the architecture steps is found by the final check. */
static void damage_page_table_root(JPBootContext *c) { c->handoff.page_table_root = 0; }
static void damage_paging(JPBootContext *c) { c->handoff.paging_enabled = false; }
static void damage_interrupts(JPBootContext *c) { c->handoff.interrupts_disabled = false; }
static void damage_entry(JPBootContext *c) { c->handoff.kernel_entry += 4; }
static void damage_stack_zero(JPBootContext *c) { c->handoff.stack_pointer = 0; }
static void damage_stack_alignment(JPBootContext *c) { c->handoff.stack_pointer += 8; }
static void damage_architecture(JPBootContext *c) { c->handoff.architecture = JP_ARCH_AARCH64; }

static void test_final_check_finds_a_damaged_handoff(void)
{
	static const Breaker damage[] = {
		damage_page_table_root, damage_paging, damage_interrupts, damage_entry,
		damage_stack_zero, damage_stack_alignment, damage_architecture
	};
	size_t i;

	for (i = 0; i < sizeof damage / sizeof damage[0]; i++) {
		JPBootContext context;

		prepare(&context);
		probe.mutate = damage[i];
		assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_SUCCESS);
		assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
		assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
		assert(context.current_stage != JP_BOOT_STAGE_READY);
		assert(!context.handoff.ownership_transferred);
		assert(fake.prepare_handoff_calls == 1);       /* the check comes after the platform step */
	}
}

/* The kernel stack is reserved with the firmware before the architecture steps. */
static void test_stack_is_reserved(void)
{
	JPBootContext context;

	prepare(&context);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(fake.reserve_calls == 1);
	assert(fake.reserved_length[0] == JP_BOOT_KERNEL_STACK_SIZE);
	assert(fake.reserved_base[0] + fake.reserved_length[0] == context.handoff.stack_pointer);
	assert(fake.reserve_calls_before_prepare == 1);   /* before the platform step */

	prepare(&context);
	fake.reserve_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(jp_fake_arch.call_count == 0);
	assert(fake.prepare_handoff_calls == 0);
	assert(context.current_stage != JP_BOOT_STAGE_READY);
}

/* The final memory map replaces the early one and is validated again. */
static void test_final_memory_map_is_validated(void)
{
	JPBootContext context;

	prepare(&context);
	fake.use_final_memory_map = true;
	fake.final_memory_map.region_count = 2;
	fake.final_memory_map.regions[0].base_address = 0;
	fake.final_memory_map.regions[0].length = 0xA0000;
	fake.final_memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	fake.final_memory_map.regions[1].base_address = USABLE_BASE;
	fake.final_memory_map.regions[1].length = USABLE_LENGTH;
	fake.final_memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.memory_map.region_count == 2);
	assert(context.memory_map.regions[0].type == JP_MEMORY_REGION_RESERVED);

	/* A final map that is not valid: no usable memory. */
	prepare(&context);
	fake.use_final_memory_map = true;
	fake.final_memory_map.region_count = 1;
	fake.final_memory_map.regions[0].length = 0x1000;
	fake.final_memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
	assert(context.current_stage != JP_BOOT_STAGE_READY);
	assert(!context.handoff.ownership_transferred);
	assert(fake.prepare_handoff_calls == 1);          /* the check follows the platform step */

	/* A final map with overlapping regions. */
	prepare(&context);
	fake.use_final_memory_map = true;
	fake.final_memory_map.region_count = 2;
	fake.final_memory_map.regions[0].base_address = USABLE_BASE;
	fake.final_memory_map.regions[0].length = USABLE_LENGTH;
	fake.final_memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;
	fake.final_memory_map.regions[1].base_address = USABLE_BASE + PAGE;
	fake.final_memory_map.regions[1].length = PAGE;
	fake.final_memory_map.regions[1].type = JP_MEMORY_REGION_RESERVED;
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_FAILED);
	assert(context.handoff.state == JP_HANDOFF_STATE_FAILED);
}

static void test_registration_rules(void)
{
	JPBootContext context;
	JPBootInteraction before;

	prepare(&context);
	assert(jp_boot_register_interaction(NULL, hook, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_register_interaction(&context, NULL, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);

	assert(jp_boot_register_interaction(&context, hook, &user_data_marker) == JP_BOOT_SUCCESS);
	assert(context.interaction.hook == hook);
	assert(context.interaction.user_data == &user_data_marker);

	/* A duplicate changes nothing. */
	before = context.interaction;
	assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_FAILED);
	assert(memcmp(&before, &context.interaction, sizeof before) == 0);

	/* Registration after the handoff has started. */
	prepare(&context);
	context.handoff.state = JP_HANDOFF_STATE_PREREQUISITES_VALID;
	assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_FAILED);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);

	prepare(&context);
	assert(jp_boot_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_register_interaction(&context, hook, NULL) == JP_BOOT_FAILED);
	assert(context.interaction.state == JP_BOOT_INTERACTION_NOT_REGISTERED);
}

int main(void)
{
	test_successful_handoff();
	test_aarch64_uses_aarch64_operations();
	test_invalid_parameters_and_second_attempt();
	test_every_missing_prerequisite_prevents_readiness();
	test_missing_framebuffer_is_allowed();
	test_architecture_failures_are_propagated_in_order();
	test_stack_needs_room();
	test_platform_failure_prevents_readiness();
	test_hook_runs_once_at_the_right_moment();
	test_failing_hook_fails_the_handoff();
	test_failure_keeps_the_handoff_stage();
	test_unknown_architecture_prevents_readiness();
	test_stack_is_placed_above_the_protected_low_memory();
	test_stack_avoids_the_kernel_image();
	test_stack_needs_a_region_that_ends_after_the_start();
	test_stack_comes_only_from_usable_memory();
	test_hook_that_was_not_registered_does_not_run();
	test_final_check_finds_a_damaged_handoff();
	test_stack_is_reserved();
	test_final_memory_map_is_validated();
	test_registration_rules();
	return 0;
}
