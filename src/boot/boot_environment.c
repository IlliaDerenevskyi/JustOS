/*
 * boot_environment.c - generic boot-environment implementation.
 *
 * Purpose:
 *   Selects the provider the caller named, initializes it, and dispatches the
 *   normalized operations to it, without any UEFI or BIOS logic of its own.
 *
 * Responsibilities:
 *   - validate the context;
 *   - dispatch to the UEFI or BIOS provider;
 *   - keep the provider lifecycle (environment.lifecycle) and the detection
 *     state truthful: a state is set only after the provider reported success;
 *   - allow input only while the provider services are valid.
 *
 * Limitations:
 *   The caller (the platform entry point) sets environment.type,
 *   environment.platform_context, and handoff.architecture before
 *   jp_boot_environment_initialize(). No firmware is detected here.
 *
 * Depends on:
 *   justos/boot/boot_environment.h and the provider headers.
 *
 * Must not depend on:
 *   Any firmware-native structure.
 */

#include "justos/boot/boot_environment.h"

#ifndef JP_BOOT_WITHOUT_BIOS
#include "justos/boot/platform/bios.h"
#endif
#ifndef JP_BOOT_WITHOUT_UEFI
#include "justos/boot/platform/uefi.h"
#endif

/*
 * A build without the BIOS provider (JP_BOOT_WITHOUT_BIOS) removes every call of
 * a jp_bios_* function; the BIOS environment type is then refused at
 * initialization, so these branches are never reached.
 */
#ifdef JP_BOOT_WITHOUT_BIOS
#define JP_BIOS_CALL(call) JP_BOOT_FAILED
#else
#define JP_BIOS_CALL(call) (call)
#endif

/* The same for the UEFI provider (JP_BOOT_WITHOUT_UEFI, the BIOS loader). */
#ifdef JP_BOOT_WITHOUT_UEFI
#define JP_UEFI_CALL(call) JP_BOOT_FAILED
#else
#define JP_UEFI_CALL(call) (call)
#endif

/* ---- Public API ---- */

JPBootResult jp_boot_environment_initialize(JPBootContext *context);
JPBootResult jp_boot_environment_get_memory_map(JPBootContext *context);
JPBootResult jp_boot_environment_get_framebuffer(JPBootContext *context);
JPBootResult jp_boot_environment_get_kernel_image(JPBootContext *context);
JPBootResult jp_boot_environment_prepare_handoff(JPBootContext *context);
JPBootResult jp_boot_environment_read_input(JPBootContext *context, JPBootInputEvent *event);
JPBootResult jp_boot_environment_reserve_memory(JPBootContext *context, uint64_t physical, uint64_t length);

/* ---- Private helpers ---- */

static bool lifecycle_permits_use(JPBootEnvironmentLifecycle lifecycle)
{
	return lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_INITIALIZED ||
	       lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE;
}

static bool provider_type_is_valid(JPBootEnvironmentType type)
{
	return type == JP_BOOT_ENVIRONMENT_UEFI || type == JP_BOOT_ENVIRONMENT_BIOS;
}

static JPBootResult provider_initialize(JPBootEnvironment *environment)
{
	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_initialize((JPUefiContext *)environment->platform_context));
	}
	return JP_BIOS_CALL(jp_bios_initialize((JPBiosContext *)environment->platform_context));
}

static JPBootResult provider_get_memory_map(JPBootContext *context)
{
	JPBootEnvironment *environment = &context->environment;

	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_get_memory_map((JPUefiContext *)environment->platform_context,
			&context->memory_map));
	}
	return JP_BIOS_CALL(jp_bios_get_memory_map((JPBiosContext *)environment->platform_context,
		&context->memory_map));
}

static JPBootResult provider_get_framebuffer(JPBootContext *context)
{
	JPBootEnvironment *environment = &context->environment;

	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_get_framebuffer((JPUefiContext *)environment->platform_context,
			&context->framebuffer));
	}
	return JP_BIOS_CALL(jp_bios_get_framebuffer((JPBiosContext *)environment->platform_context,
		&context->framebuffer));
}

static JPBootResult provider_get_kernel_image(JPBootContext *context)
{
	JPBootEnvironment *environment = &context->environment;

	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_get_kernel_image((JPUefiContext *)environment->platform_context,
			&context->kernel.image));
	}
	return JP_BIOS_CALL(jp_bios_get_kernel_image((JPBiosContext *)environment->platform_context,
		&context->kernel.image));
}

static JPBootResult provider_prepare_handoff(JPBootEnvironment *environment)
{
	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_prepare_handoff((JPUefiContext *)environment->platform_context));
	}
	return JP_BIOS_CALL(jp_bios_prepare_handoff((JPBiosContext *)environment->platform_context));
}

static JPBootResult provider_read_input(JPBootEnvironment *environment, JPBootInputEvent *event)
{
	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_read_input((JPUefiContext *)environment->platform_context, event));
	}
	return JP_BIOS_CALL(jp_bios_read_input((JPBiosContext *)environment->platform_context, event));
}

static JPBootResult provider_reserve_memory(JPBootEnvironment *environment, uint64_t physical, uint64_t length)
{
	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		return JP_UEFI_CALL(jp_uefi_reserve_memory((JPUefiContext *)environment->platform_context, physical, length));
	}
	return JP_BIOS_CALL(jp_bios_reserve_memory((JPBiosContext *)environment->platform_context, physical, length));
}

/* Common check of the operations that need a usable provider. */
static JPBootResult require_usable_provider(const JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!lifecycle_permits_use(context->environment.lifecycle)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	return JP_BOOT_SUCCESS;
}

/* ---- Implementation ---- */

JPBootResult jp_boot_environment_initialize(JPBootContext *context)
{
	JPBootEnvironment *environment;
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	environment = &context->environment;
	if (!provider_type_is_valid(environment->type) || environment->platform_context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (environment->lifecycle != JP_BOOT_ENVIRONMENT_LIFECYCLE_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
#ifdef JP_BOOT_WITHOUT_BIOS
	if (environment->type == JP_BOOT_ENVIRONMENT_BIOS) {
		environment->state = JP_DETECTION_STATE_FAILED;
		environment->lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED;
		return JP_BOOT_FAILED;
	}
#endif
#ifdef JP_BOOT_WITHOUT_UEFI
	if (environment->type == JP_BOOT_ENVIRONMENT_UEFI) {
		environment->state = JP_DETECTION_STATE_FAILED;
		environment->lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED;
		return JP_BOOT_FAILED;
	}
#endif
	if (environment->type == JP_BOOT_ENVIRONMENT_BIOS &&
	    context->handoff.architecture == JP_ARCH_AARCH64) {
		environment->state = JP_DETECTION_STATE_FAILED;
		environment->lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED;
		return JP_BOOT_FAILED;
	}

	result = provider_initialize(environment);
	if (result != JP_BOOT_SUCCESS) {
		environment->state = JP_DETECTION_STATE_FAILED;
		environment->lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED;
		return result;
	}
	environment->state = JP_DETECTION_STATE_DETECTED;
	environment->lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_INITIALIZED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_get_memory_map(JPBootContext *context)
{
	JPBootResult result = require_usable_provider(context);

	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = provider_get_memory_map(context);
	if (result == JP_BOOT_SUCCESS) {
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE;
	}
	return result;
}

JPBootResult jp_boot_environment_get_framebuffer(JPBootContext *context)
{
	JPBootResult result = require_usable_provider(context);

	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = provider_get_framebuffer(context);
	if (result == JP_BOOT_SUCCESS) {
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE;
	}
	return result;
}

JPBootResult jp_boot_environment_get_kernel_image(JPBootContext *context)
{
	JPBootResult result = require_usable_provider(context);

	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = provider_get_kernel_image(context);
	if (result == JP_BOOT_SUCCESS) {
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE;
	}
	return result;
}

/*
 * After the provider has prepared the handoff it holds the final memory map.
 * The final map, not the early one, is what the kernel receives, because it
 * shows the memory that Stage 1 reserved.
 */
static JPBootResult publish_final_memory_map(JPBootContext *context)
{
	JPBootResult result = provider_get_memory_map(context);

	if (result != JP_BOOT_SUCCESS) {
		context->memory_map.state = JP_DETECTION_STATE_FAILED;
		return result;
	}
	if (context->memory_map.region_count > JP_MAX_MEMORY_REGIONS) {
		context->memory_map.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	context->memory_map.state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_environment_prepare_handoff(JPBootContext *context)
{
	JPBootResult result = require_usable_provider(context);

	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = provider_prepare_handoff(&context->environment);
	if (result == JP_BOOT_SUCCESS) {
		/* The provider now answers get_memory_map with the final map. */
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_HANDOFF_PREPARED;
		result = publish_final_memory_map(context);
	}
	if (result != JP_BOOT_SUCCESS) {
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED;
	}
	return result;
}

JPBootResult jp_boot_environment_reserve_memory(JPBootContext *context, uint64_t physical, uint64_t length)
{
	JPBootResult result;

	if (context == NULL || length == 0) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	result = require_usable_provider(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = provider_reserve_memory(&context->environment, physical, length);
	if (result == JP_BOOT_SUCCESS) {
		context->environment.lifecycle = JP_BOOT_ENVIRONMENT_LIFECYCLE_ACTIVE;
	}
	return result;
}

JPBootResult jp_boot_environment_read_input(JPBootContext *context, JPBootInputEvent *event)
{
	if (context == NULL || event == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!lifecycle_permits_use(context->environment.lifecycle)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	return provider_read_input(&context->environment, event);
}
