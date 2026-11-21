/*
 * Unit test (level U) of src/boot/boot_info.c: every field of the summary equals the value of its owner, every
 * "not available" state is carried over unchanged, the status is available only when Stage 3 reported it, and
 * nothing in the context is written. The real Stage 3 status and log modules are used.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_info.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_status.h"

static JPBootContext context;

static void fresh(void)
{
	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_X86_64;
}

static void test_arguments(void)
{
	JPBootSummary summary;

	fresh();
	assert(jp_boot_info_summarize(NULL, &summary) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_info_summarize(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

/* Phase A: nothing but Stage 1 exists. A zero context says "unknown" and "unset" everywhere (nothing is invented). */
static void test_zero_context(void)
{
	JPBootSummary s;

	memset(&context, 0, sizeof context);
	memset(&s, 0xA5, sizeof s);                       /* the function must clear what it does not set */
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(s.architecture == JP_ARCH_UNKNOWN);
	assert(s.environment == JP_BOOT_ENVIRONMENT_UNKNOWN);
	assert(s.environment_state == JP_DETECTION_STATE_UNKNOWN);
	assert(s.hardware_state == JP_DETECTION_STATE_UNKNOWN);
	assert(s.memory_map_state == JP_DETECTION_STATE_UNKNOWN);
	assert(s.framebuffer_state == JP_DETECTION_STATE_UNKNOWN);
	assert(s.kernel_validation_state == context.kernel.validation_state);
	assert(s.kernel_load_state == context.kernel.load_state);
	assert(!s.status_available);
	assert((int)s.status.status == 0 && s.status.stage == 0);       /* cleared */
	assert(s.config_state == JP_BOOT_CONFIG_STATE_UNSET);
	assert(s.config_source == JP_BOOT_CONFIG_SOURCE_UNKNOWN);
	assert(s.selection_outcome == JP_BOOT_SELECTION_NOT_RUN);
}

static void test_values_are_copied(void)
{
	JPBootSummary s;

	fresh();
	context.handoff.architecture = JP_ARCH_AARCH64;
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	context.environment.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	context.hardware.overall_state = JP_DETECTION_STATE_FAILED;
	context.memory_map.state = JP_DETECTION_STATE_NOT_DETECTED;
	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.stage4.config.state = JP_BOOT_CONFIG_STATE_FROZEN;
	context.stage4.config.source = JP_BOOT_CONFIG_SOURCE_INTERACTIVE;
	context.stage4.selection_outcome = JP_BOOT_SELECTION_DISCARDED;

	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(s.architecture == JP_ARCH_AARCH64);
	assert(s.environment == JP_BOOT_ENVIRONMENT_BIOS);
	assert(s.environment_state == JP_DETECTION_STATE_NOT_INITIALIZED);
	assert(s.hardware_state == JP_DETECTION_STATE_FAILED);
	assert(s.memory_map_state == JP_DETECTION_STATE_NOT_DETECTED);
	assert(s.framebuffer_state == JP_DETECTION_STATE_DETECTED);
	assert(s.kernel_validation_state == JP_KERNEL_VALIDATION_VALID);
	assert(s.kernel_load_state == JP_KERNEL_LOAD_LOADED);
	assert(s.config_state == JP_BOOT_CONFIG_STATE_FROZEN);
	assert(s.config_source == JP_BOOT_CONFIG_SOURCE_INTERACTIVE);
	assert(s.selection_outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(!s.status_available);            /* Stage 3 has not started */
}

static void test_status(void)
{
	JPBootSummary s;

	fresh();
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(s.status_available && s.status.status == JP_BOOT_STATUS_BOOTING);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(s.status_available && s.status.status == JP_BOOT_STATUS_INITIALIZING);
}

/* The summary is a copy: nothing in the context is written. */
static void test_context_unchanged(void)
{
	JPBootSummary s;
	static JPBootContext before;

	fresh();
	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.stage4.selection_outcome = JP_BOOT_SELECTION_APPLIED;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
	memcpy(&before, &context, sizeof context);
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &context, sizeof context) == 0);
	/* Changing the summary later does not change the context (it holds no pointer into it). */
	s.config_state = JP_BOOT_CONFIG_STATE_FROZEN;
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
}

static void test_names(void)
{
	assert(strcmp(jp_boot_info_state_name(JP_DETECTION_STATE_UNKNOWN), "UNKNOWN") == 0);
	assert(strcmp(jp_boot_info_state_name(JP_DETECTION_STATE_NOT_DETECTED), "NOT DETECTED") == 0);
	assert(strcmp(jp_boot_info_state_name(JP_DETECTION_STATE_NOT_INITIALIZED), "NOT INITIALIZED") == 0);
	assert(strcmp(jp_boot_info_state_name(JP_DETECTION_STATE_DETECTED), "DETECTED") == 0);
	assert(strcmp(jp_boot_info_state_name(JP_DETECTION_STATE_FAILED), "FAILED") == 0);
	assert(strcmp(jp_boot_info_state_name((JPDetectionState)(JP_DETECTION_STATE_FAILED + 1)), "UNKNOWN") == 0);   /* the first value past the table */
	assert(strcmp(jp_boot_info_state_name((JPDetectionState)99), "UNKNOWN") == 0);
	assert(strcmp(jp_boot_info_state_name((JPDetectionState)-1), "UNKNOWN") == 0);
}

int main(void)
{
	test_arguments();
	test_zero_context();
	test_values_are_copied();
	test_status();
	test_context_unchanged();
	test_names();
	return 0;
}
