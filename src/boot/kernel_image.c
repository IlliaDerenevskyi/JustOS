/*
 * kernel_image.c - kernel image acquisition.
 *
 * Purpose:
 *   Gets the raw kernel image through the boot environment and checks that the
 *   storage it returns is usable.
 *
 * Responsibilities:
 *   - request the image from the active provider;
 *   - check that the buffer exists and the size is not zero;
 *   - publish the image state (DETECTED only after a real acquisition) and
 *     reset the kernel validation and load states to NOT_INITIALIZED.
 *
 * Limitations:
 *   The image is not parsed here (see kernel_loader.c). The storage mechanism
 *   is provider-defined. The image stays valid until loading no longer needs
 *   it.
 *
 * Depends on:
 *   justos/boot/kernel_image.h, justos/boot/boot_environment.h.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/kernel_image.h"

#include "justos/boot/boot_environment.h"

/* ---- Public API ---- */

JPBootResult jp_boot_acquire_kernel_image(JPBootContext *context);

/* ---- Private helpers ---- */

static bool validate_kernel_image_buffer(const JPKernelImage *image)
{
	return image->buffer != NULL && image->size != 0;
}

/* ---- Implementation ---- */

JPBootResult jp_boot_acquire_kernel_image(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	context->current_stage = JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION;

	result = jp_boot_environment_get_kernel_image(context);
	if (result != JP_BOOT_SUCCESS) {
		context->kernel.image.state = result == JP_BOOT_NOT_DETECTED
			? JP_DETECTION_STATE_NOT_DETECTED
			: JP_DETECTION_STATE_FAILED;
		return result;
	}
	if (!validate_kernel_image_buffer(&context->kernel.image)) {
		context->kernel.image.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	context->kernel.image.state = JP_DETECTION_STATE_DETECTED;
	context->kernel.validation_state = JP_KERNEL_VALIDATION_NOT_INITIALIZED;
	context->kernel.load_state = JP_KERNEL_LOAD_NOT_INITIALIZED;
	return JP_BOOT_SUCCESS;
}
