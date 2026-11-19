/*
 * framebuffer.c - framebuffer acquisition and validation.
 *
 * Purpose:
 *   Gets the normalized framebuffer through the boot environment and checks
 *   that its description is consistent.
 *
 * Responsibilities:
 *   - jp_boot_initialize_framebuffer(): request, check format, validate,
 *     publish the state (DETECTED only after real acquisition and validation);
 *   - jp_boot_validate_framebuffer(): validate an acquired framebuffer again.
 *
 * Limitations:
 *   RGB_888 and BGR_888 both use 4 bytes per pixel (three 8-bit channels and
 *   one unused byte). A machine without a framebuffer is a valid situation:
 *   the state is then NOT_DETECTED, not FAILED. No UEFI GOP or BIOS/VBE access.
 *
 * Depends on:
 *   justos/boot/framebuffer.h, justos/boot/boot_environment.h.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/framebuffer.h"

#include "justos/boot/boot_environment.h"

/* ---- Public API ---- */

JPBootResult jp_boot_initialize_framebuffer(JPBootContext *context);
JPBootResult jp_boot_validate_framebuffer(JPBootContext *context);

/* ---- Private helpers ---- */

enum {
	JP_FRAMEBUFFER_BYTES_PER_PIXEL = 4,
	JP_FRAMEBUFFER_PITCH_ALIGNMENT = 4
};

static bool format_is_supported(JPFramebufferFormat format)
{
	return format == JP_FRAMEBUFFER_FORMAT_RGB_888 ||
	       format == JP_FRAMEBUFFER_FORMAT_BGR_888;
}

/*
 * pitch * height always fits in 64 bits (both are 32-bit values), so there is
 * no overflow check for it. width * 4 is computed in 64 bits so that it
 * cannot wrap around.
 */
static bool validate_framebuffer(const JPFramebufferInfo *fb)
{
	uint64_t row_bytes;

	if (fb->base_address == 0 || fb->width == 0 || fb->height == 0) {
		return false;
	}
	if (!format_is_supported(fb->format)) {
		return false;
	}
	row_bytes = (uint64_t)fb->width * JP_FRAMEBUFFER_BYTES_PER_PIXEL;
	if ((uint64_t)fb->pitch < row_bytes) {
		return false;
	}
	return fb->pitch % JP_FRAMEBUFFER_PITCH_ALIGNMENT == 0;
}

/* ---- Implementation ---- */

JPBootResult jp_boot_initialize_framebuffer(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	context->current_stage = JP_BOOT_STAGE_FRAMEBUFFER_INIT;

	result = jp_boot_environment_get_framebuffer(context);
	if (result != JP_BOOT_SUCCESS) {
		context->framebuffer.state = result == JP_BOOT_NOT_DETECTED
			? JP_DETECTION_STATE_NOT_DETECTED
			: JP_DETECTION_STATE_FAILED;
		return result;
	}
	if (context->framebuffer.format == JP_FRAMEBUFFER_FORMAT_UNKNOWN) {
		context->framebuffer.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_FAILED;
	}
	if (!validate_framebuffer(&context->framebuffer)) {
		context->framebuffer.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	context->framebuffer.state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_validate_framebuffer(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->framebuffer.state != JP_DETECTION_STATE_DETECTED) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!validate_framebuffer(&context->framebuffer)) {
		context->framebuffer.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	return JP_BOOT_SUCCESS;
}
