/*
 * Unit test (level U) of src/boot/boot_environment.c.
 * The caller sets environment.type and environment.platform_context; the
 * fake providers (tests/support) stand in for the real UEFI and BIOS code.
 * Every test body is generated for both providers.
 */

#include <assert.h>
#include <string.h>

#include "fake_bios.h"
#include "fake_uefi.h"
#include "justos/boot/boot_environment.h"

static JPBootInputEvent key_event(JPBootInputKey key)
{
	JPBootInputEvent event = { JP_BOOT_INPUT_KEY, key, 0 };

	return event;
}

static JPBootInputEvent char_event(uint32_t character)
{
	JPBootInputEvent event = { JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, character };

	return event;
}

/* Generates the tests of one provider. */
#define DEFINE_PROVIDER_TESTS(NAME, CTX, ENV, RESET, PUSH, TAG) \
\
static void NAME##_prepare(JPBootContext *context, CTX *fake) \
{ \
	memset(context, 0, sizeof *context); \
	RESET(fake); \
	context->environment.type = ENV; \
	context->environment.platform_context = fake; \
} \
\
static void NAME##_initialize_success(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(fake.initialize_calls == 1); \
	assert(fake.called_by == TAG); \
	assert(context.environment.state == JP_DETECTION_STATE_DETECTED); \
	assert(context.environment.type == ENV); \
	assert(context.environment.platform_context == &fake); \
} \
\
static void NAME##_initialize_invalid(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	assert(jp_boot_environment_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM); \
\
	NAME##_prepare(&context, &fake); \
	context.environment.type = JP_BOOT_ENVIRONMENT_UNKNOWN; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_ERROR_INVALID_PARAM); \
	assert(fake.initialize_calls == 0); \
	assert(context.environment.state != JP_DETECTION_STATE_DETECTED); \
\
	NAME##_prepare(&context, &fake); \
	context.environment.platform_context = NULL; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_ERROR_INVALID_PARAM); \
	assert(context.environment.state != JP_DETECTION_STATE_DETECTED); \
} \
\
static void NAME##_initialize_failure_is_propagated(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	fake.initialize_result = JP_BOOT_FAILED; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_FAILED); \
	assert(context.environment.state == JP_DETECTION_STATE_FAILED); \
} \
\
static void NAME##_operations_dispatch_and_fill_context(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	fake.memory_map.region_count = 2; \
	fake.memory_map.regions[1].length = 4096; \
	fake.framebuffer.width = 800; \
	fake.framebuffer.height = 600; \
	fake.kernel_image.size = 1234; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
\
	fake.called_by = 0; \
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_SUCCESS); \
	assert(fake.called_by == TAG); \
	assert(context.memory_map.region_count == 2); \
	assert(context.memory_map.regions[1].length == 4096); \
	fake.called_by = 0; \
	assert(jp_boot_environment_get_framebuffer(&context) == JP_BOOT_SUCCESS); \
	assert(fake.called_by == TAG); \
	assert(context.framebuffer.width == 800 && context.framebuffer.height == 600); \
	fake.called_by = 0; \
	assert(jp_boot_environment_get_kernel_image(&context) == JP_BOOT_SUCCESS); \
	assert(fake.called_by == TAG); \
	assert(context.kernel.image.size == 1234); \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS); \
	assert(fake.prepare_called_by == TAG); \
	/* get_memory_map once for the early map, and once more for the final map at the handoff. */ \
	assert(fake.memory_map_calls == 2 && fake.framebuffer_calls == 1); \
	assert(fake.kernel_image_calls == 1 && fake.prepare_handoff_calls == 1); \
	/* Every call went to the fake of the selected provider, not to the other one. */ \
	assert(fake.called_by == TAG); \
} \
\
static void NAME##_operation_failures_are_propagated(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	fake.memory_map_result = JP_BOOT_ERROR_CORRUPTED_DATA; \
	fake.framebuffer_result = JP_BOOT_NOT_DETECTED; \
	fake.kernel_image_result = JP_BOOT_FAILED; \
	fake.prepare_handoff_result = JP_BOOT_FAILED; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_ERROR_CORRUPTED_DATA); \
	assert(jp_boot_environment_get_framebuffer(&context) == JP_BOOT_NOT_DETECTED); \
	assert(jp_boot_environment_get_kernel_image(&context) == JP_BOOT_FAILED); \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_FAILED); \
} \
\
static void NAME##_operations_need_initialization(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	assert(jp_boot_environment_get_memory_map(&context) != JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_get_framebuffer(&context) != JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_get_kernel_image(&context) != JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_prepare_handoff(&context) != JP_BOOT_SUCCESS); \
	assert(fake.memory_map_calls == 0 && fake.framebuffer_calls == 0); \
	assert(fake.kernel_image_calls == 0 && fake.prepare_handoff_calls == 0); \
} \
\
static void NAME##_input_dispatch(void) \
{ \
	JPBootContext context; \
	CTX fake; \
	JPBootInputEvent event; \
\
	NAME##_prepare(&context, &fake); \
\
	/* Not initialized: rejected without calling the provider. */ \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_NOT_INITIALIZED); \
	assert(fake.input_calls == 0); \
\
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_SUCCESS); \
	assert(event.kind == JP_BOOT_INPUT_NONE); \
\
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_SUCCESS); \
	assert(fake.called_by == TAG); \
	assert(PUSH(&fake, key_event(JP_BOOT_KEY_F10)) == 0); \
	assert(PUSH(&fake, char_event(0x61)) == 0); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_SUCCESS); \
	assert(event.kind == JP_BOOT_INPUT_KEY && event.key == JP_BOOT_KEY_F10); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_SUCCESS); \
	assert(event.kind == JP_BOOT_INPUT_CHARACTER && event.character == 0x61); \
\
	/* Active state (after an acquisition) still permits input. */ \
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_SUCCESS); \
\
	/* From handoff preparation onward input is rejected. */ \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS); \
	assert(PUSH(&fake, key_event(JP_BOOT_KEY_ENTER)) == 0); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_NOT_INITIALIZED); \
} \
\
static void NAME##_input_invalid_parameters(void) \
{ \
	JPBootContext context; \
	CTX fake; \
	JPBootInputEvent event; \
\
	NAME##_prepare(&context, &fake); \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_read_input(NULL, &event) == JP_BOOT_ERROR_INVALID_PARAM); \
	assert(jp_boot_environment_read_input(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM); \
	assert(fake.input_calls == 0); \
} \
\
static void NAME##_input_provider_results_are_unchanged(void) \
{ \
	JPBootContext context; \
	CTX fake; \
	JPBootInputEvent event; \
\
	NAME##_prepare(&context, &fake); \
	fake.input_empty_result = JP_BOOT_NOT_DETECTED; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_NOT_DETECTED); \
	fake.input_empty_result = JP_BOOT_FAILED; \
	assert(jp_boot_environment_read_input(&context, &event) == JP_BOOT_FAILED); \
} \
\
static void NAME##_reserve_memory(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	assert(jp_boot_environment_reserve_memory(NULL, 0x1000, 0x1000) == JP_BOOT_ERROR_INVALID_PARAM); \
\
	NAME##_prepare(&context, &fake); \
	/* Not initialized: rejected without calling the provider. */ \
	assert(jp_boot_environment_reserve_memory(&context, 0x200000, 0x3000) == JP_BOOT_NOT_INITIALIZED); \
	assert(fake.reserve_calls == 0); \
\
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_reserve_memory(&context, 0x200000, 0) == JP_BOOT_ERROR_INVALID_PARAM); \
	assert(fake.reserve_calls == 0); \
\
	assert(jp_boot_environment_reserve_memory(&context, 0x200000, 0x3000) == JP_BOOT_SUCCESS); \
	assert(fake.reserve_calls == 1); \
	assert(fake.reserved_base[0] == 0x200000 && fake.reserved_length[0] == 0x3000); \
	assert(fake.called_by == TAG); \
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE); \
\
	/* A provider failure is returned unchanged and does not spoil the provider. */ \
	fake.reserve_result = JP_BOOT_ERROR_CORRUPTED_DATA; \
	assert(jp_boot_environment_reserve_memory(&context, 0x400000, 0x1000) == JP_BOOT_ERROR_CORRUPTED_DATA); \
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE); \
	fake.reserve_result = JP_BOOT_SUCCESS; \
	assert(jp_boot_environment_reserve_memory(&context, 0x400000, 0x1000) == JP_BOOT_SUCCESS); \
\
	/* After the handoff is prepared nothing can be reserved. */ \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_reserve_memory(&context, 0x600000, 0x1000) == JP_BOOT_NOT_INITIALIZED); \
} \
\
static void NAME##_final_memory_map_is_published(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	NAME##_prepare(&context, &fake); \
	fake.memory_map.region_count = 1; \
	fake.final_memory_map.region_count = 3; \
	fake.final_memory_map.regions[2].base_address = 0xA0000; \
	fake.final_memory_map.regions[2].length = 0x1000; \
	fake.final_memory_map.regions[2].type = JP_MEMORY_REGION_MMIO; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_SUCCESS); \
	assert(context.memory_map.region_count == 1);          /* the early map */ \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS); \
	assert(fake.memory_map_calls == 2);                     /* one more request: the final map */ \
	assert(context.memory_map.region_count == 3);          /* the final map replaced it */ \
	assert(context.memory_map.regions[2].type == JP_MEMORY_REGION_MMIO); \
	assert(context.memory_map.state == JP_DETECTION_STATE_DETECTED); \
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_HANDOFF_PREPARED); \
} \
\
static void NAME##_final_memory_map_failures(void) \
{ \
	JPBootContext context; \
	CTX fake; \
\
	/* The provider cannot hand over a final map. */ \
	NAME##_prepare(&context, &fake); \
	fake.final_map_result = JP_BOOT_FAILED; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_FAILED); \
	assert(context.memory_map.state == JP_DETECTION_STATE_FAILED); \
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED); \
\
	/* A final map with too many regions. */ \
	NAME##_prepare(&context, &fake); \
	fake.final_memory_map.region_count = JP_MAX_MEMORY_REGIONS + 1; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_ERROR_CORRUPTED_DATA); \
	assert(context.memory_map.state == JP_DETECTION_STATE_FAILED); \
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED); \
\
	/* When the provider's own handoff fails, no final map is requested. */ \
	NAME##_prepare(&context, &fake); \
	fake.prepare_handoff_result = JP_BOOT_FAILED; \
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS); \
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_FAILED); \
	assert(fake.memory_map_calls == 0); \
} \
\
static void NAME##_run(void) \
{ \
	NAME##_reserve_memory(); \
	NAME##_final_memory_map_is_published(); \
	NAME##_final_memory_map_failures(); \
	NAME##_initialize_success(); \
	NAME##_initialize_invalid(); \
	NAME##_initialize_failure_is_propagated(); \
	NAME##_operations_dispatch_and_fill_context(); \
	NAME##_operation_failures_are_propagated(); \
	NAME##_operations_need_initialization(); \
	NAME##_input_dispatch(); \
	NAME##_input_invalid_parameters(); \
	NAME##_input_provider_results_are_unchanged(); \
}

DEFINE_PROVIDER_TESTS(uefi, JPUefiContext, JP_BOOT_ENVIRONMENT_UEFI,
	jp_fake_uefi_reset, jp_fake_uefi_push_input, 'U')
DEFINE_PROVIDER_TESTS(bios, JPBiosContext, JP_BOOT_ENVIRONMENT_BIOS,
	jp_fake_bios_reset, jp_fake_bios_push_input, 'B')

/* Only the selected provider is touched. */
static void test_only_selected_provider_is_used(void)
{
	JPBootContext context;
	JPUefiContext uefi;
	JPBiosContext bios;

	memset(&context, 0, sizeof context);
	jp_fake_uefi_reset(&uefi);
	jp_fake_bios_reset(&bios);
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	context.environment.platform_context = &bios;
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_SUCCESS);
	assert(bios.initialize_calls == 1 && bios.memory_map_calls == 1);
	assert(uefi.initialize_calls == 0 && uefi.memory_map_calls == 0);
}

/* AArch64 has no BIOS path: the combination is refused and the provider is not called. */
static void test_aarch64_bios_is_refused(void)
{
	JPBootContext context;
	JPBiosContext bios;

	memset(&context, 0, sizeof context);
	jp_fake_bios_reset(&bios);
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	context.environment.platform_context = &bios;
	context.handoff.architecture = JP_ARCH_AARCH64;
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_FAILED);
	assert(bios.initialize_calls == 0);
	assert(context.environment.state == JP_DETECTION_STATE_FAILED);
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED);
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_NOT_INITIALIZED);
}

/* A second initialization is refused and does not call the provider again. */
static void test_second_initialization_is_refused(void)
{
	JPBootContext context;
	JPUefiContext uefi;

	memset(&context, 0, sizeof context);
	jp_fake_uefi_reset(&uefi);
	context.environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context.environment.platform_context = &uefi;
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_FAILED);
	assert(uefi.initialize_calls == 1);
	assert(context.environment.state == JP_DETECTION_STATE_DETECTED);
}

/* The lifecycle follows INITIALIZED, ACTIVE, HANDOFF_PREPARED. */
static void test_lifecycle_values(void)
{
	JPBootContext context;
	JPUefiContext uefi;

	memset(&context, 0, sizeof context);
	jp_fake_uefi_reset(&uefi);
	context.environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context.environment.platform_context = &uefi;
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_UNINITIALIZED);
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS);
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_INITIALIZED);
	assert(jp_boot_environment_get_kernel_image(&context) == JP_BOOT_SUCCESS);
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE);
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_HANDOFF_PREPARED);
	/* Nothing works after the transition. */
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_NOT_INITIALIZED);
}

int main(void)
{
	test_aarch64_bios_is_refused();
	test_second_initialization_is_refused();
	test_lifecycle_values();
	uefi_run();
	bios_run();
	test_only_selected_provider_is_used();
	return 0;
}
