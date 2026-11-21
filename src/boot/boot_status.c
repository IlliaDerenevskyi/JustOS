/*
 * boot_status.c - the boot status of Stage 3.
 *
 * Purpose:
 *   The one status of the current boot attempt and the graph of its allowed
 *   changes (milestone0.md, "3 Decided rules", "The status graph, exactly").
 *
 * Responsibilities:
 *   - initialize (BOOTING), update (checks the graph and the stage tag),
 *     get (a copy);
 *   - a change to FAILED needs a recorded error and copies its severity,
 *     recoverability, and original result.
 *
 * Limitations:
 *   Writes only the Stage 3 area. A refused change leaves it as it was.
 *
 * Depends on:
 *   justos/boot/boot_status.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 4.
 */

#include "justos/boot/boot_status.h"

/* ---- Private helpers ---- */

static bool architecture_is_valid(JPArchitectureType architecture)
{
	return architecture == JP_ARCH_X86_64 || architecture == JP_ARCH_AARCH64;
}

static bool transition_is_allowed(JPBootStatus from, JPBootStatus to)
{
	switch (from) {
	case JP_BOOT_STATUS_BOOTING:
		return to == JP_BOOT_STATUS_INITIALIZING;
	case JP_BOOT_STATUS_INITIALIZING:
		return to == JP_BOOT_STATUS_RUNNING || to == JP_BOOT_STATUS_FAILED;
	case JP_BOOT_STATUS_RUNNING:
		return to == JP_BOOT_STATUS_FAILED || to == JP_BOOT_STATUS_RECOVERY;
	case JP_BOOT_STATUS_FAILED:
		return to == JP_BOOT_STATUS_RECOVERY || to == JP_BOOT_STATUS_HALTED;
	case JP_BOOT_STATUS_RECOVERY:
		return to == JP_BOOT_STATUS_BOOTING || to == JP_BOOT_STATUS_HALTED;
	default:
		return false;     /* HALTED is final, and an unknown value is not a status */
	}
}

/* The tag must fit the change (see the rule). */
static bool tag_fits(JPDiagnosticStage stage, JPBootStatus to)
{
	switch (to) {
	case JP_BOOT_STATUS_BOOTING:
		return stage == JP_DIAGNOSTIC_STAGE_STAGE1 || stage == JP_DIAGNOSTIC_STAGE_STAGE3;
	case JP_BOOT_STATUS_INITIALIZING:
		return stage == JP_DIAGNOSTIC_STAGE_STAGE1 || stage == JP_DIAGNOSTIC_STAGE_STAGE2;
	case JP_BOOT_STATUS_RUNNING:
	case JP_BOOT_STATUS_RECOVERY:
	case JP_BOOT_STATUS_HALTED:
		return stage == JP_DIAGNOSTIC_STAGE_STAGE3;
	case JP_BOOT_STATUS_FAILED:
		return stage >= JP_DIAGNOSTIC_STAGE_STAGE1 && stage <= JP_DIAGNOSTIC_STAGE_STAGE4;
	default:
		return false;
	}
}

/* ---- Public API ---- */

JPBootResult jp_boot_status_initialize(JPBootContext *context)
{
	JPBootStage3Area *area;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (area->status_initialized || !architecture_is_valid(context->handoff.architecture)) {
		return JP_BOOT_FAILED;
	}
	area->status.stage = JP_DIAGNOSTIC_STAGE_STAGE1;
	area->status.status = JP_BOOT_STATUS_BOOTING;
	area->status.severity = JP_BOOT_SEVERITY_INFO;
	area->status.recoverability = JP_BOOT_RECOVERABILITY_UNKNOWN;
	area->status.original_result = JP_BOOT_SUCCESS;
	area->status.architecture = context->handoff.architecture;
	area->status_initialized = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_status_update(JPBootContext *context, JPDiagnosticStage stage, JPBootStatus status)
{
	JPBootStage3Area *area;
	JPBootStatusState next;

	if (context == NULL || status > JP_BOOT_STATUS_HALTED || stage > JP_DIAGNOSTIC_STAGE_STAGE4) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized || !area->status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!tag_fits(stage, status)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (area->report.finalized || !transition_is_allowed(area->status.status, status)) {
		return JP_BOOT_FAILED;
	}
	if (status == JP_BOOT_STATUS_FAILED && !area->error_recorded) {
		return JP_BOOT_FAILED;     /* a failure is never made up */
	}

	next = area->status;          /* a complete replacement is built first and published at the end */
	next.stage = stage;
	next.status = status;
	if (status == JP_BOOT_STATUS_FAILED) {
		next.severity = area->error.severity;
		next.recoverability = area->error.recoverability;
		next.original_result = area->error.original_result;
	}
	area->status = next;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_status_get(const JPBootContext *context, JPBootStatusState *status)
{
	if (context == NULL || status == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized || !context->stage3.status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	*status = context->stage3.status;
	return JP_BOOT_SUCCESS;
}
