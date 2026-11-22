/*
 * recovery.c - recovery decision and entry of Stage 3.
 *
 * Purpose:
 *   Decides what to do after a failure and carries it out (milestone0.md,
 *   "3 Decided rules": "What jp_boot_recovery_decide() returns in Milestone 0"
 *   and "Recovery, in detail").
 *
 * Responsibilities:
 *   - jp_boot_recovery_decide: ENTER, RETRY, or TERMINAL_HALT;
 *   - jp_boot_recovery_enter: stores the decision, asks the Stage 4 Boot
 *     Manager (jp_boot_manager_request_recovery), prepares a retry with the
 *     reset and preserve table (and undoes it if Stage 4 refuses), or takes the
 *     terminal path; never returns.
 *
 * Limitations:
 *   Writes only the Stage 3 area. Never enables interrupts. No allocation.
 *   JP_BOOT_MAXIMUM_RETRY_COUNT is 0, so a retry never happens in
 *   Milestone 0; the code is exercised by the unit tests with a larger limit.
 *
 * Depends on:
 *   justos/boot/recovery.h, boot_status.h, boot_log.h, boot_report.h, panic.h,
 *   boot_manager.h (the one function jp_boot_manager_request_recovery),
 *   diagnostic_hw.h.
 *
 * Must not depend on:
 *   firmware, heap, the rest of Stage 4.
 */

#include "justos/boot/recovery.h"

#include "diagnostic_hw.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/panic.h"

/* ---- Private helpers ---- */

static bool decision_is_valid(JPRecoveryDecision decision)
{
	return decision == JP_RECOVERY_ENTER || decision == JP_RECOVERY_RETRY || decision == JP_RECOVERY_TERMINAL_HALT;
}

/* True when retry_count > 0 and retry_count >= maximum. */
static bool is_exhausted(const JPBootRecoveryState *recovery)
{
	return recovery->retry_count > 0 && recovery->retry_count >= recovery->maximum_retry_count;
}

/* The terminal path: HALTED when the graph allows it, report final, the panic screen if there is an error, stop. */
static _Noreturn void terminal_path(JPBootContext *context)
{
	JPBootStage3Area *area;

	if (context == NULL || !context->stage3.log_initialized || !context->stage3.status_initialized) {
		jp_diagnostic_halt();
	}
	area = &context->stage3;
	if (area->status.status == JP_BOOT_STATUS_FAILED || area->status.status == JP_BOOT_STATUS_RECOVERY) {
		(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED);
	}
	(void)jp_boot_report_finalize(context);
	if (area->error_recorded) {
		JPBootError error = area->error;

		jp_boot_panic(context, &error);
	}
	jp_diagnostic_halt();
}

/* The small parts of the Stage 3 area that a retry changes, for the undo. */
typedef struct JPRetrySnapshot {
	JPBootAttemptState attempt;
	JPBootRecoveryState recovery;
	JPBootStatusState status;
	JPBootError error;
	bool error_recorded;
	JPBootLogStatus log_status;
	uint64_t next_event_sequence;
	JPBootEvent slot;               /* the log slot that the history event may use */
	size_t slot_index;
} JPRetrySnapshot;

static void snapshot_take(const JPBootStage3Area *area, JPRetrySnapshot *snapshot)
{
	snapshot->attempt = area->attempt;
	snapshot->recovery = area->recovery;
	snapshot->status = area->status;
	snapshot->error = area->error;
	snapshot->error_recorded = area->error_recorded;
	snapshot->log_status = area->log.status;
	snapshot->next_event_sequence = area->log.next_event_sequence;
	snapshot->slot_index = area->log.status.event_count < JP_BOOT_LOG_CAPACITY ? area->log.status.event_count : 0;
	snapshot->slot = area->log.events[snapshot->slot_index];
}

static void snapshot_restore(JPBootStage3Area *area, const JPRetrySnapshot *snapshot)
{
	area->attempt = snapshot->attempt;
	area->recovery = snapshot->recovery;
	area->status = snapshot->status;
	area->error = snapshot->error;
	area->error_recorded = snapshot->error_recorded;
	area->log.status = snapshot->log_status;
	area->log.next_event_sequence = snapshot->next_event_sequence;
	area->log.events[snapshot->slot_index] = snapshot->slot;
}

/* Prepares the new attempt (the reset and preserve table). False if a step failed; the caller undoes everything. */
static bool prepare_retry(JPBootContext *context)
{
	JPBootStage3Area *area = &context->stage3;
	uint64_t new_id;

	if (area->attempt.boot_attempt_id == UINT64_MAX) {
		return false;     /* an identity is never reused */
	}
	new_id = area->attempt.boot_attempt_id + 1U;

	/* The error of the old attempt stays in the history, as an event, when the log has room. */
	if (area->error_recorded && area->log.status.event_count < JP_BOOT_LOG_CAPACITY && !area->log.status.finalized) {
		JPBootEvent history;

		history.boot_attempt_id = 0;
		history.event_sequence = 0;
		history.stage = area->error.stage;
		history.severity = area->error.severity;
		history.architecture = area->error.architecture;
		history.source_component = area->error.source_component;
		history.result = area->error.original_result;
		history.diagnostic_context = area->error.diagnostic_context;
		if (jp_boot_log_append(context, &history) != JP_BOOT_SUCCESS) {
			return false;
		}
	}

	area->attempt.boot_attempt_id = new_id;
	area->attempt.attempt_sequence++;
	area->attempt.retry_count++;
	area->recovery.previous_attempt_id = area->recovery.current_attempt_id;
	area->recovery.current_attempt_id = new_id;
	area->recovery.retry_count++;
	area->error_recorded = false;
	area->log.next_event_sequence = 0;

	if (area->status.status == JP_BOOT_STATUS_FAILED &&
	    jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) != JP_BOOT_SUCCESS) {
		return false;
	}
	return jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_BOOTING) == JP_BOOT_SUCCESS;
}

/* ---- Public API ---- */

JPBootResult jp_boot_recovery_decide(const JPBootContext *context, JPRecoveryDecision *decision)
{
	const JPBootStage3Area *area;

	if (context == NULL || decision == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized || !area->status_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (area->status.status == JP_BOOT_STATUS_HALTED || area->panic == JP_BOOT_PANIC_TERMINAL) {
		*decision = JP_RECOVERY_TERMINAL_HALT;
		return JP_BOOT_SUCCESS;
	}
	if (!area->error_recorded) {
		if (area->status.status != JP_BOOT_STATUS_RUNNING) {
			return JP_BOOT_FAILED;     /* a recovery after a failure needs the error */
		}
		*decision = JP_RECOVERY_ENTER;  /* asked for through the configuration */
		return JP_BOOT_SUCCESS;
	}
	if (area->error.recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE || is_exhausted(&area->recovery)) {
		*decision = JP_RECOVERY_TERMINAL_HALT;
	} else if ((area->status.status == JP_BOOT_STATUS_FAILED || area->status.status == JP_BOOT_STATUS_RECOVERY) &&
	           area->panic == JP_BOOT_PANIC_NORMAL && area->recovery.retry_count < area->recovery.maximum_retry_count) {
		*decision = JP_RECOVERY_RETRY;
	} else {
		*decision = JP_RECOVERY_ENTER;
	}
	return JP_BOOT_SUCCESS;
}

_Noreturn void jp_boot_recovery_enter(JPBootContext *context, JPRecoveryDecision decision)
{
	JPBootStage3Area *area;
	JPRecoveryDecision allowed;

	if (context == NULL || !context->stage3.log_initialized || !context->stage3.status_initialized) {
		terminal_path(context);
	}
	area = &context->stage3;
	/* The decision is stored; one that is not in the enumeration is a terminal halt. */
	area->recovery.decision = decision_is_valid(decision) ? decision : JP_RECOVERY_TERMINAL_HALT;
	if (area->recovery.decision == JP_RECOVERY_TERMINAL_HALT) {
		terminal_path(context);
	}

	if (decision == JP_RECOVERY_ENTER) {
		if (area->status.status != JP_BOOT_STATUS_RECOVERY &&
		    jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) != JP_BOOT_SUCCESS) {
			terminal_path(context);
		}
		if (jp_boot_manager_request_recovery(context, &area->recovery) == JP_BOOT_TRANSITION_ACCEPTED) {
			jp_diagnostic_halt();     /* the destination has begun: Stage 3 has nothing more to do */
		}
		terminal_path(context);
	}

	/* JP_RECOVERY_RETRY: only when the decision rules give it now. */
	if (jp_boot_recovery_decide(context, &allowed) != JP_BOOT_SUCCESS || allowed != JP_RECOVERY_RETRY) {
		terminal_path(context);
	}
	{
		JPRetrySnapshot snapshot;

		snapshot_take(area, &snapshot);
		if (prepare_retry(context) &&
		    jp_boot_manager_request_recovery(context, &area->recovery) == JP_BOOT_TRANSITION_ACCEPTED) {
			jp_diagnostic_halt();     /* the new attempt has begun */
		}
		snapshot_restore(area, &snapshot);     /* refused: the old attempt stays current, nothing is half done */
		area->recovery.decision = JP_RECOVERY_TERMINAL_HALT;
	}
	terminal_path(context);
}
