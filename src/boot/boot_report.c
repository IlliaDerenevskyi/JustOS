/*
 * boot_report.c - the Boot Report and the finalization of Stage 3.
 *
 * Purpose:
 *   Freezes the diagnostic state and publishes a read-only report (milestone0.md,
 *   "3 Decided rules": "The Boot Report, in detail").
 *
 * Responsibilities:
 *   - jp_boot_report_finalize: checks that the records agree, closes the log,
 *     and publishes the report; a second call changes nothing;
 *   - jp_boot_report_get: a copy of the report, once it is final.
 *
 * Limitations:
 *   Writes only the Stage 3 area, and nothing at all when a check fails.
 *   No allocation.
 *
 * Depends on:
 *   justos/boot/boot_report.h, boot_log.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, heap, Stage 4.
 */

#include "justos/boot/boot_report.h"

#include "justos/boot/boot_log.h"

/* ---- Private helpers ---- */

/* True when retry_count > 0 and retry_count >= maximum: the budget was used up by real retries. */
static bool retries_exhausted(uint32_t retry_count, uint32_t maximum)
{
	return retry_count > 0 && retry_count >= maximum;
}

static bool status_has_failure(JPBootStatus status)
{
	return status == JP_BOOT_STATUS_FAILED || status == JP_BOOT_STATUS_RECOVERY || status == JP_BOOT_STATUS_HALTED;
}

/* The records must belong together (see the rule). */
static bool records_are_consistent(const JPBootStage3Area *area)
{
	if (area->recovery.current_attempt_id != area->attempt.boot_attempt_id) {
		return false;
	}
	if (area->recovery.retry_count > area->recovery.maximum_retry_count ||
	    area->attempt.retry_count > area->attempt.maximum_retry_count) {
		return false;
	}
	if (area->log.status.event_count > JP_BOOT_LOG_CAPACITY) {
		return false;
	}
	if (area->status.status == JP_BOOT_STATUS_FAILED && !area->error_recorded) {
		return false;
	}
	if (area->error_recorded) {
		if (area->error.boot_attempt_id != area->attempt.boot_attempt_id) {
			return false;
		}
		if (status_has_failure(area->status.status) && area->status.original_result != area->error.original_result) {
			return false;
		}
	}
	return true;
}

/* ---- Public API ---- */

/*
 * Copies the report field by field. A single assignment of the whole report
 * would make the compiler call memcpy, which the kernel does not have.
 */
static void copy_report(JPBootReport *to, const JPBootReport *from)
{
	to->boot_attempt_id = from->boot_attempt_id;
	to->status = from->status;
	to->error = from->error;
	to->recovery = from->recovery;
	to->log_status = from->log_status;
	to->architecture = from->architecture;
	to->finalized = from->finalized;
}

JPBootResult jp_boot_report_finalize(JPBootContext *context)
{
	JPBootStage3Area *area;
	JPBootRecoveryState recovery;
	JPBootReport *report;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized || !area->status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (area->report.finalized) {
		return JP_BOOT_SUCCESS;     /* idempotent: the report stays as it was */
	}
	if (!records_are_consistent(area)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	if (jp_boot_log_finalize(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}

	recovery = area->recovery;
	recovery.exhausted = retries_exhausted(recovery.retry_count, recovery.maximum_retry_count);
	area->recovery = recovery;

	/* Every check has passed: publish. `finalized` is set last. */
	report = &area->report;
	report->finalized = false;
	report->boot_attempt_id = area->attempt.boot_attempt_id;
	report->status = area->status;
	if (area->error_recorded) {
		report->error = area->error;
	} else {
		report->error.boot_attempt_id = 0;       /* none: a real error never has the result JP_BOOT_SUCCESS */
		report->error.original_result = JP_BOOT_SUCCESS;
		report->error.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
		report->error.severity = JP_BOOT_SEVERITY_INFO;
		report->error.recoverability = JP_BOOT_RECOVERABILITY_RECOVERABLE;
		report->error.architecture = JP_ARCH_UNKNOWN;
		report->error.source_component = JP_BOOT_SOURCE_UNKNOWN;
		report->error.diagnostic_context = 0;
	}
	report->recovery = recovery;
	report->log_status = area->log.status;
	report->architecture = area->status.architecture;
	report->finalized = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_report_get(const JPBootContext *context, JPBootReport *report)
{
	if (context == NULL || report == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized || !context->stage3.status_initialized || !context->stage3.report.finalized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	copy_report(report, &context->stage3.report);
	return JP_BOOT_SUCCESS;
}
