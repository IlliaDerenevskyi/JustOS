/*
 * Unit test (level U) of src/boot/hardware.c.
 * Rules: see "Decided rules (hardware discovery)" in milestone0.md.
 */

#include <assert.h>
#include <string.h>

#include "fake_architecture.h"
#include "justos/boot/hardware.h"

static void prepare(JPBootContext *context, JPArchitectureType arch)
{
	memset(context, 0, sizeof *context);
	jp_fake_arch_reset();
	context->handoff.architecture = arch;
	context->environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context->environment.state = JP_DETECTION_STATE_DETECTED;
}

static void test_success_on_both_architectures(void)
{
	JPBootContext context;

	prepare(&context, JP_ARCH_X86_64);
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_SUCCESS);
	assert(jp_fake_arch.x86_64_calls == 1 && jp_fake_arch.aarch64_calls == 0);
	assert(context.hardware.cpu.state == JP_DETECTION_STATE_DETECTED);
	assert(context.hardware.cpu.logical_cores == 4);
	assert(context.hardware.firmware.state == JP_DETECTION_STATE_DETECTED);
	assert(context.hardware.firmware.type == JP_BOOT_ENVIRONMENT_UEFI);
	assert(context.hardware.boot_device.state == JP_DETECTION_STATE_DETECTED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_DETECTED);
	assert(context.current_stage == JP_BOOT_STAGE_HARDWARE_DISCOVERY);

	prepare(&context, JP_ARCH_AARCH64);
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_SUCCESS);
	assert(jp_fake_arch.aarch64_calls == 1 && jp_fake_arch.x86_64_calls == 0);
}

static void test_firmware_type_follows_environment(void)
{
	JPBootContext context;

	prepare(&context, JP_ARCH_X86_64);
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_SUCCESS);
	assert(context.hardware.firmware.type == JP_BOOT_ENVIRONMENT_BIOS);
}

static void test_invalid_parameters(void)
{
	JPBootContext context;

	assert(jp_boot_discover_hardware(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context, JP_ARCH_UNKNOWN);
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_fake_arch.discover_cpu_calls == 0);
	assert(context.hardware.overall_state != JP_DETECTION_STATE_DETECTED);
}

static void test_cpu_failure_is_propagated_unchanged(void)
{
	JPBootContext context;

	prepare(&context, JP_ARCH_X86_64);
	jp_fake_arch.discover_cpu_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.hardware.cpu.state == JP_DETECTION_STATE_FAILED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_FAILED);
}

static void test_uninitialized_environment_is_not_reported_as_detected(void)
{
	JPBootContext context;

	prepare(&context, JP_ARCH_X86_64);
	context.environment.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(context.hardware.firmware.state == JP_DETECTION_STATE_NOT_INITIALIZED);
	assert(context.hardware.boot_device.state == JP_DETECTION_STATE_NOT_INITIALIZED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_NOT_INITIALIZED);
}

static void test_aggregation_precedence(void)
{
	JPBootContext context;

	/* NOT_DETECTED cpu loses against NOT_INITIALIZED firmware. */
	prepare(&context, JP_ARCH_X86_64);
	jp_fake_arch.cpu.state = JP_DETECTION_STATE_NOT_DETECTED;
	context.environment.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_NOT_INITIALIZED);

	/* UNKNOWN beats NOT_INITIALIZED. */
	prepare(&context, JP_ARCH_X86_64);
	jp_fake_arch.cpu.state = JP_DETECTION_STATE_UNKNOWN;
	context.environment.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_UNKNOWN);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_UNKNOWN);

	/* Only the CPU missing. */
	prepare(&context, JP_ARCH_X86_64);
	jp_fake_arch.cpu.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_NOT_DETECTED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_NOT_DETECTED);

	/* FAILED beats everything. */
	prepare(&context, JP_ARCH_X86_64);
	jp_fake_arch.cpu.state = JP_DETECTION_STATE_FAILED;
	context.environment.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	assert(jp_boot_discover_hardware(&context) == JP_BOOT_FAILED);
	assert(context.hardware.overall_state == JP_DETECTION_STATE_FAILED);
}

int main(void)
{
	test_success_on_both_architectures();
	test_firmware_type_follows_environment();
	test_invalid_parameters();
	test_cpu_failure_is_propagated_unchanged();
	test_uninitialized_environment_is_not_reported_as_detected();
	test_aggregation_precedence();
	return 0;
}
