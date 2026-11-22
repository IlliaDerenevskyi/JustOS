#include "fake_environment.h"

#include <string.h>

void jp_fake_environment_attach(JPBootContext *context, JPFakeEnvironment *fake)
{
	memset(fake, 0, sizeof *fake);
	memset(context, 0, sizeof *context);
	context->environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context->environment.state = JP_DETECTION_STATE_DETECTED;
	context->environment.platform_context = fake;
}

static JPFakeEnvironment *fake_of(JPBootContext *context)
{
	return (JPFakeEnvironment *)context->environment.platform_context;
}

JPBootResult jp_boot_environment_get_memory_map(JPBootContext *context)
{
	JPFakeEnvironment *fake = fake_of(context);

	fake->memory_map_calls++;
	if (fake->memory_map_result != JP_BOOT_SUCCESS) {
		return fake->memory_map_result;
	}
	context->memory_map = fake->memory_map;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_get_framebuffer(JPBootContext *context)
{
	JPFakeEnvironment *fake = fake_of(context);

	fake->framebuffer_calls++;
	if (fake->framebuffer_result != JP_BOOT_SUCCESS) {
		return fake->framebuffer_result;
	}
	context->framebuffer = fake->framebuffer;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_get_kernel_image(JPBootContext *context)
{
	JPFakeEnvironment *fake = fake_of(context);

	fake->kernel_image_calls++;
	if (fake->kernel_image_result != JP_BOOT_SUCCESS) {
		return fake->kernel_image_result;
	}
	context->kernel.image = fake->kernel_image;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_prepare_handoff(JPBootContext *context)
{
	JPFakeEnvironment *fake = fake_of(context);

	fake->prepare_handoff_calls++;
	fake->reserve_calls_before_prepare = fake->reserve_calls;
	if (fake->on_prepare_handoff != NULL) {
		fake->on_prepare_handoff(context);
	}
	if (fake->prepare_handoff_result != JP_BOOT_SUCCESS) {
		return fake->prepare_handoff_result;
	}
	if (fake->use_final_memory_map) {
		context->memory_map = fake->final_memory_map;
		context->memory_map.state = JP_DETECTION_STATE_DETECTED;
	}
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_reserve_memory(
	JPBootContext *context,
	uint64_t physical,
	uint64_t length
)
{
	JPFakeEnvironment *fake = fake_of(context);
	unsigned int index = fake->reserve_calls;

	if (index < JP_FAKE_ENV_RESERVE_LOG) {
		fake->reserved_base[index] = physical;
		fake->reserved_length[index] = length;
		/* Segment states in table order: the n-th reservation of a load is segment n. */
		if (index < JP_MAX_KERNEL_SEGMENTS) {
			fake->segment_state_at_reserve[index] = context->kernel.segments[index].state;
		}
	}
	fake->reserve_calls++;
	if (fake->reserve_refusals > 0) {
		fake->reserve_refusals--;
		return fake->reserve_refusal_result;
	}
	return fake->reserve_result;
}
