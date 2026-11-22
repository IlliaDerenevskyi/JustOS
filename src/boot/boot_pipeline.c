/*
 * boot_pipeline.c - the Stage 1 pipeline.
 *
 * Purpose:
 *   Calls the public Stage 1 functions in the one order in which they can
 *   succeed, and stops at the first failure.
 *
 * Responsibilities:
 *   - the order of the nine steps;
 *   - returning the first failing result unchanged.
 *
 * Limitations:
 *   The caller zero-fills the context and sets environment.type,
 *   environment.platform_context, and handoff.architecture first (and registers
 *   the interaction hook, if one is wanted). The function writes no field of
 *   the context itself and does not enter the kernel.
 *
 * Depends on:
 *   The public headers of the Stage 1 modules.
 *
 * Must not depend on:
 *   Any firmware or architecture header.
 */

#include "justos/boot/boot_pipeline.h"

#include "justos/boot/boot_environment.h"
#include "justos/boot/framebuffer.h"
#include "justos/boot/handoff.h"
#include "justos/boot/hardware.h"
#include "justos/boot/kernel_image.h"
#include "justos/boot/kernel_loader.h"
#include "justos/boot/memory_map.h"

/* ---- Public API ---- */

JPBootResult jp_boot_run_pipeline(JPBootContext *context);

/* ---- Implementation ---- */

JPBootResult jp_boot_run_pipeline(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}

	result = jp_boot_environment_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_discover_hardware(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_discover_memory(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_validate_memory_map(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	/* A machine without a framebuffer can still start: the pipeline goes on. */
	result = jp_boot_initialize_framebuffer(context);
	if (result != JP_BOOT_SUCCESS && result != JP_BOOT_NOT_DETECTED) {
		return result;
	}
	result = jp_boot_acquire_kernel_image(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_validate_kernel(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_load_kernel(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	return jp_boot_prepare_handoff(context);
}
