/*
 * error.c - error classification of Stage 3.
 *
 * Purpose:
 *   Turns a JPBootResult into one immutable error record. The rules are in
 *   milestone0.md ("3 Error Classification Contract" and "3 Decided rules").
 *
 * Responsibilities:
 *   - default recoverability by result, default severity (FATAL when not
 *     recoverable, ERROR otherwise);
 *   - overrides that never weaken a FATAL or NON_RECOVERABLE result;
 *   - at most one error per attempt, kept in the Stage 3 area.
 *
 * Limitations:
 *   Writes only the Stage 3 area. No allocation, no firmware.
 *
 * Depends on:
 *   justos/boot/error.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 4.
 */

#include "justos/boot/error.h"

/* ---- Private helpers ---- */

static bool result_is_valid(JPBootResult result)
{
	return result >= JP_BOOT_SUCCESS && result <= JP_BOOT_ERROR_CORRUPTED_DATA;
}

static bool architecture_is_valid(JPArchitectureType architecture)
{
	return architecture == JP_ARCH_X86_64 || architecture == JP_ARCH_AARCH64;
}

static JPBootRecoverability default_recoverability(JPBootResult result)
{
	switch (result) {
	case JP_BOOT_ERROR_INVALID_PARAM:
	case JP_BOOT_ERROR_CORRUPTED_DATA:
		return JP_BOOT_RECOVERABILITY_NON_RECOVERABLE;
	case JP_BOOT_NOT_INITIALIZED:
	case JP_BOOT_NOT_DETECTED:
		return JP_BOOT_RECOVERABILITY_RECOVERABLE;
	default:
		return JP_BOOT_RECOVERABILITY_UNKNOWN;
	}
}

static JPBootSeverity default_severity(JPBootRecoverability recoverability)
{
	return recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE ? JP_BOOT_SEVERITY_FATAL : JP_BOOT_SEVERITY_ERROR;
}

static bool input_is_valid(const JPBootErrorInput *input)
{
	return result_is_valid(input->original_result) &&
	       input->original_result != JP_BOOT_SUCCESS &&
	       input->source_component <= JP_BOOT_SOURCE_BOOT_MANAGER &&
	       input->stage <= JP_DIAGNOSTIC_STAGE_STAGE4 &&
	       input->severity_override <= JP_BOOT_SEVERITY_OVERRIDE_FATAL &&
	       input->recoverability_override <= JP_BOOT_RECOVERABILITY_OVERRIDE_UNKNOWN;
}

/* The override as a base severity: JP_BOOT_SEVERITY_OVERRIDE_INFO is JP_BOOT_SEVERITY_INFO and so on. */
static JPBootSeverity severity_of_override(JPBootSeverityOverride override_value)
{
	return (JPBootSeverity)((int)override_value - (int)JP_BOOT_SEVERITY_OVERRIDE_INFO);
}

static JPBootRecoverability recoverability_of_override(JPBootRecoverabilityOverride override_value)
{
	return (JPBootRecoverability)((int)override_value - (int)JP_BOOT_RECOVERABILITY_OVERRIDE_RECOVERABLE);
}

/* ---- Public API ---- */

JPBootResult jp_boot_error_create(JPBootContext *context, const JPBootErrorInput *input)
{
	JPBootStage3Area *area;
	JPBootRecoverability recoverability;
	JPBootSeverity severity;

	if (context == NULL || input == NULL || !input_is_valid(input)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized || !area->status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (area->report.finalized || area->error_recorded || !architecture_is_valid(context->handoff.architecture)) {
		return JP_BOOT_FAILED;
	}

	recoverability = default_recoverability(input->original_result);
	severity = default_severity(recoverability);
	if (input->recoverability_override != JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED) {
		JPBootRecoverability wanted = recoverability_of_override(input->recoverability_override);

		if (recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE && wanted != JP_BOOT_RECOVERABILITY_NON_RECOVERABLE) {
			return JP_BOOT_ERROR_INVALID_PARAM;
		}
		recoverability = wanted;
	}
	if (input->severity_override != JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED) {
		JPBootSeverity wanted = severity_of_override(input->severity_override);

		if (severity == JP_BOOT_SEVERITY_FATAL && wanted != JP_BOOT_SEVERITY_FATAL) {
			return JP_BOOT_ERROR_INVALID_PARAM;
		}
		severity = wanted;
	}

	area->error.boot_attempt_id = area->attempt.boot_attempt_id;
	area->error.original_result = input->original_result;
	area->error.stage = input->stage;
	area->error.severity = severity;
	area->error.recoverability = recoverability;
	area->error.architecture = context->handoff.architecture;
	area->error.source_component = input->source_component;
	area->error.diagnostic_context = input->diagnostic_context;
	area->error_recorded = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_error_get(const JPBootContext *context, JPBootError *error)
{
	if (context == NULL || error == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized || !context->stage3.status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!context->stage3.error_recorded) {
		return JP_BOOT_NOT_DETECTED;
	}
	*error = context->stage3.error;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_error_clear(JPBootContext *context)
{
	JPBootStage3Area *area;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized || !area->status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (area->report.finalized || area->status.status != JP_BOOT_STATUS_BOOTING) {
		return JP_BOOT_FAILED;
	}
	area->error_recorded = false;
	return JP_BOOT_SUCCESS;
}
