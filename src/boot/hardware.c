/*
 * hardware.c - hardware discovery.
 *
 * Purpose:
 *   Discovers the CPU, the firmware, and the boot device, and stores the
 *   normalized result in context->hardware.
 *
 * Responsibilities:
 *   - CPU: through the architecture operation of handoff.architecture;
 *   - firmware and boot device: copied from the state of the boot
 *     environment, which the provider reported;
 *   - aggregate the three states and turn the aggregate into a result.
 *
 * Limitations:
 *   The caller sets handoff.architecture before discovery. This module never
 *   selects a firmware and contains no UEFI or BIOS logic.
 *
 * Depends on:
 *   justos/boot/hardware.h and the architecture headers.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/hardware.h"

#include "justos/boot/architecture/aarch64.h"
#include "justos/boot/architecture/x86_64.h"
#include "arch_dispatch.h"

/* ---- Public API ---- */

JPBootResult jp_boot_discover_hardware(JPBootContext *context);

/* ---- Private helpers ---- */

/* Precedence of the aggregation: the higher rank wins. */
enum {
	JP_STATE_RANK_DETECTED = 0,
	JP_STATE_RANK_NOT_DETECTED,
	JP_STATE_RANK_NOT_INITIALIZED,
	JP_STATE_RANK_UNKNOWN,
	JP_STATE_RANK_FAILED
};

static int state_rank(JPDetectionState state)
{
	switch (state) {
	case JP_DETECTION_STATE_FAILED: return JP_STATE_RANK_FAILED;
	case JP_DETECTION_STATE_UNKNOWN: return JP_STATE_RANK_UNKNOWN;
	case JP_DETECTION_STATE_NOT_INITIALIZED: return JP_STATE_RANK_NOT_INITIALIZED;
	case JP_DETECTION_STATE_NOT_DETECTED: return JP_STATE_RANK_NOT_DETECTED;
	case JP_DETECTION_STATE_DETECTED: break;
	}
	return JP_STATE_RANK_DETECTED;
}

static JPDetectionState worse_of(JPDetectionState a, JPDetectionState b)
{
	return state_rank(b) > state_rank(a) ? b : a;
}

static JPDetectionState aggregate_hardware_state(const JPHardwareInfo *hardware)
{
	JPDetectionState state = JP_DETECTION_STATE_DETECTED;

	state = worse_of(state, hardware->cpu.state);
	state = worse_of(state, hardware->firmware.state);
	state = worse_of(state, hardware->boot_device.state);
	return state;
}

static JPBootResult result_of_state(JPDetectionState state)
{
	switch (state) {
	case JP_DETECTION_STATE_DETECTED: return JP_BOOT_SUCCESS;
	case JP_DETECTION_STATE_NOT_DETECTED: return JP_BOOT_NOT_DETECTED;
	case JP_DETECTION_STATE_NOT_INITIALIZED: return JP_BOOT_NOT_INITIALIZED;
	case JP_DETECTION_STATE_UNKNOWN: return JP_BOOT_UNKNOWN;
	case JP_DETECTION_STATE_FAILED: break;
	}
	return JP_BOOT_FAILED;
}

/* The state a component gets when it is known only through the environment. */
static JPDetectionState state_from_environment(const JPBootEnvironment *environment)
{
	return environment->state == JP_DETECTION_STATE_DETECTED
		? JP_DETECTION_STATE_DETECTED
		: JP_DETECTION_STATE_NOT_INITIALIZED;
}

static JPBootResult discover_cpu(JPBootContext *context)
{
	JPCpuInfo cpu;
	JPBootResult result;

	cpu.state = JP_DETECTION_STATE_UNKNOWN;
	cpu.logical_cores = 0;
	result = JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_discover_cpu(&cpu), jp_aarch64_discover_cpu(&cpu));
	if (result != JP_BOOT_SUCCESS) {
		context->hardware.cpu.state = JP_DETECTION_STATE_FAILED;
		context->hardware.cpu.logical_cores = 0;
		return result;
	}
	context->hardware.cpu = cpu;
	return JP_BOOT_SUCCESS;
}

static void discover_firmware(JPBootContext *context)
{
	context->hardware.firmware.type = context->environment.type;
	context->hardware.firmware.state = state_from_environment(&context->environment);
}

static void discover_boot_device(JPBootContext *context)
{
	context->hardware.boot_device.state = state_from_environment(&context->environment);
}

/* ---- Implementation ---- */

JPBootResult jp_boot_discover_hardware(JPBootContext *context)
{
	JPBootResult cpu_result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	context->current_stage = JP_BOOT_STAGE_HARDWARE_DISCOVERY;
	if (!JP_ARCH_IS_SUPPORTED(context->handoff.architecture)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}

	cpu_result = discover_cpu(context);
	discover_firmware(context);
	discover_boot_device(context);
	context->hardware.overall_state = aggregate_hardware_state(&context->hardware);

	if (cpu_result != JP_BOOT_SUCCESS) {
		return cpu_result;
	}
	return result_of_state(context->hardware.overall_state);
}
