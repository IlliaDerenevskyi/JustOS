#include "fake_bios.h"

#include <string.h>

void jp_fake_bios_reset(JPBiosContext *context)
{
	memset(context, 0, sizeof *context);
	context->input_empty_result = JP_BOOT_SUCCESS;
}

int jp_fake_bios_push_input(JPBiosContext *context, JPBootInputEvent event)
{
	if (context->input_count >= JP_FAKE_INPUT_QUEUE_CAPACITY) {
		return -1;
	}
	context->input_queue[context->input_count] = event;
	context->input_count++;
	return 0;
}

JPBootResult jp_bios_initialize(JPBiosContext *context)
{
	context->called_by = 'B';
	context->initialize_calls++;
	if (context->initialize_result != JP_BOOT_SUCCESS) {
		return context->initialize_result;
	}
	context->state = JP_FAKE_STATE_INITIALIZED;
	return JP_BOOT_SUCCESS;
}

static void mark_active(JPBiosContext *context)
{
	if (context->state == JP_FAKE_STATE_INITIALIZED) {
		context->state = JP_FAKE_STATE_ACTIVE;
	}
}

JPBootResult jp_bios_get_memory_map(
	JPBiosContext *context,
	JPMemoryMap *memory_map
)
{
	context->called_by = 'B';
	context->memory_map_calls++;
	if (context->state == JP_FAKE_STATE_HANDOFF_PREPARED) {
		if (context->final_map_result != JP_BOOT_SUCCESS) {
			return context->final_map_result;
		}
		*memory_map = context->final_memory_map;
		return JP_BOOT_SUCCESS;
	}
	if (context->memory_map_result != JP_BOOT_SUCCESS) {
		return context->memory_map_result;
	}
	*memory_map = context->memory_map;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_get_framebuffer(
	JPBiosContext *context,
	JPFramebufferInfo *framebuffer
)
{
	context->called_by = 'B';
	context->framebuffer_calls++;
	if (context->framebuffer_result != JP_BOOT_SUCCESS) {
		return context->framebuffer_result;
	}
	*framebuffer = context->framebuffer;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_get_kernel_image(
	JPBiosContext *context,
	JPKernelImage *image
)
{
	context->called_by = 'B';
	context->kernel_image_calls++;
	if (context->kernel_image_result != JP_BOOT_SUCCESS) {
		return context->kernel_image_result;
	}
	*image = context->kernel_image;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_prepare_handoff(JPBiosContext *context)
{
	context->called_by = 'B';
	context->prepare_called_by = 'B';
	context->prepare_handoff_calls++;
	if (context->prepare_handoff_result != JP_BOOT_SUCCESS) {
		return context->prepare_handoff_result;
	}
	context->state = JP_FAKE_STATE_HANDOFF_PREPARED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_read_input(
	JPBiosContext *context,
	JPBootInputEvent *event
)
{
	context->called_by = 'B';
	context->input_calls++;
	if (context->state != JP_FAKE_STATE_INITIALIZED &&
	    context->state != JP_FAKE_STATE_ACTIVE) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (context->input_next >= context->input_count) {
		if (context->input_empty_result != JP_BOOT_SUCCESS) {
			return context->input_empty_result;
		}
		event->kind = JP_BOOT_INPUT_NONE;
		return JP_BOOT_SUCCESS;
	}
	*event = context->input_queue[context->input_next];
	context->input_next++;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_reserve_memory(
	JPBiosContext *context,
	uint64_t physical,
	uint64_t length
)
{
	context->called_by = 'B';
	if (context->reserve_calls < JP_FAKE_RESERVE_LOG) {
		context->reserved_base[context->reserve_calls] = physical;
		context->reserved_length[context->reserve_calls] = length;
	}
	context->reserve_calls++;
	return context->reserve_result;
}
