/*
 * boot_manager.c - the Stage 4 Boot Manager.
 *
 * Purpose:
 *   Implements the preparation and the menu of phase A, the completion after
 *   Stage 2, the decision, and the recovery transition (milestone0.md, "4
 *   Stage 4 Algorithms and State Rules", "4 Recovery Transition Owner
 *   Contract", "4 Decided rules", and file specifications 7 and 8).
 *
 * Responsibilities:
 *   Phase A (the boot program, JP_BOOT_STAGE4_PHASE_A): jp_boot_manager_prepare
 *   and jp_boot_manager_select.
 *   Phase B (the kernel image, JP_BOOT_STAGE4_PHASE_B): jp_boot_manager_decide,
 *   jp_boot_manager_request_recovery, and jp_kernel_stage2_complete.
 *
 * Limitations:
 *   Writes only the stage4 area, and the Stage 3 data through the Stage 3
 *   functions. Never turns interrupts on. No heap, no global data that
 *   changes. In phase B there is no firmware, keyboard, or timer.
 *
 * Depends on:
 *   justos/boot/boot_manager.h, boot_config.h, boot_info.h, boot_ui.h,
 *   handoff.h and boot_environment.h (phase A), the Stage 3 headers and
 *   justos/kernel/init.h (phase B).
 *
 * Must not depend on:
 *   Stage 1 firmware layers in phase B; Stage 3 code in phase A.
 */

#include "justos/boot/boot_manager.h"

#if defined(JP_BOOT_STAGE4_PHASE_A) == defined(JP_BOOT_STAGE4_PHASE_B)
#error "define exactly one of JP_BOOT_STAGE4_PHASE_A and JP_BOOT_STAGE4_PHASE_B"
#endif

#include "justos/boot/boot_config.h"
#include "justos/boot/boot_ui.h"

#if defined(JP_BOOT_STAGE4_PHASE_A)
#include "justos/boot/boot_environment.h"
#include "justos/boot/handoff.h"
#else
#include "justos/boot/boot_info.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/error.h"
#include "justos/boot/recovery.h"
#include "justos/kernel/init.h"
#endif

#if defined(JP_BOOT_STAGE4_PHASE_A)

/* ================= Phase A ================= */

#define JP_MANAGER_PIXEL_BYTES 4U

/* The same rule as the menu's: a usable framebuffer ("4 Decided rules"). */
static bool framebuffer_is_usable(const JPFramebufferInfo *fb)
{
	return fb->state == JP_DETECTION_STATE_DETECTED &&
	       fb->base_address != 0 &&
	       fb->width != 0 && fb->height != 0 && fb->pitch != 0 &&
	       (fb->format == JP_FRAMEBUFFER_FORMAT_RGB_888 || fb->format == JP_FRAMEBUFFER_FORMAT_BGR_888) &&
	       (uint64_t)fb->pitch >= (uint64_t)fb->width * JP_MANAGER_PIXEL_BYTES &&
	       fb->pitch % JP_MANAGER_PIXEL_BYTES == 0;
}

/* The state moves to SELECTED and the outcome is stored; the menu function ends here. */
static JPBootResult skip(JPBootContext *context, JPBootSelectionOutcome outcome)
{
	context->stage4.selection_outcome = outcome;
	context->stage4.state = JP_BOOT_MANAGER_SELECTED;
	return JP_BOOT_SUCCESS;
}

/* ---- Public API (phase A) ---- */

JPBootResult jp_boot_manager_prepare(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.state != JP_BOOT_MANAGER_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
	result = jp_boot_config_initialize(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	context->stage4.selection_outcome = JP_BOOT_SELECTION_NOT_RUN;
	context->stage4.decision = JP_BOOT_DECISION_NONE;
	result = jp_boot_register_interaction(context, jp_boot_manager_select, NULL);
	if (result != JP_BOOT_SUCCESS) {
		return result;     /* the state stays UNINITIALIZED and no function is registered */
	}
	context->stage4.state = JP_BOOT_MANAGER_PREPARED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_manager_select(JPBootContext *context, void *user_data)
{
	JPBootInputEvent event;
	JPBootSelectionOutcome outcome = JP_BOOT_SELECTION_NOT_RUN;
	JPBootResult result;

	(void)user_data;
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.state != JP_BOOT_MANAGER_PREPARED) {
		return JP_BOOT_FAILED;
	}
	if (JP_BOOT_SELECTION_POLICY == JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY) {
		return skip(context, JP_BOOT_SELECTION_SKIPPED_POLICY);
	}
	if (!framebuffer_is_usable(&context->framebuffer)) {
		return skip(context, JP_BOOT_SELECTION_SKIPPED_NO_FRAMEBUFFER);
	}
	/* One call that does not wait: SUCCESS means that a keyboard exists (and perhaps that no key is waiting). */
	result = jp_boot_environment_read_input(context, &event);
	if (result == JP_BOOT_NOT_DETECTED) {
		return skip(context, JP_BOOT_SELECTION_SKIPPED_NO_INPUT);
	}
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_ui_select(context, &outcome);
	if (result != JP_BOOT_SUCCESS) {
		return result;     /* the handoff fails; the state stays PREPARED */
	}
	return skip(context, outcome);
}

#else

/* ================= Phase B ================= */

/* ---- Private helpers ---- */

/* The Stage 3 stopping routine ("3 Decided rules"): turns interrupts off and waits; changes no record. */
static _Noreturn void stop_machine(void)
{
	jp_boot_recovery_enter(NULL, JP_RECOVERY_TERMINAL_HALT);
}

static void add_event(JPBootContext *context, JPDiagnosticStage stage, JPBootSeverity severity,
	JPBootSourceComponent source, JPBootResult result, uint32_t diagnostic_context)
{
	JPBootEvent event;

	event.boot_attempt_id = 0;
	event.event_sequence = 0;
	event.stage = stage;
	event.severity = severity;
	event.architecture = context->handoff.architecture;
	event.source_component = source;
	event.result = result;
	event.diagnostic_context = diagnostic_context;
	(void)jp_boot_log_append(context, &event);     /* an event is a report, not a decision: a refusal is ignored */
}

static void add_manager_event(JPBootContext *context, JPBootSeverity severity, JPBootResult result, uint32_t diagnostic_context)
{
	add_event(context, JP_DIAGNOSTIC_STAGE_STAGE4, severity, JP_BOOT_SOURCE_BOOT_MANAGER, result, diagnostic_context);
}

static void record_stage2_result(JPBootContext *context)
{
	add_event(context, JP_DIAGNOSTIC_STAGE_STAGE2,
		context->stage2_result == JP_BOOT_SUCCESS ? JP_BOOT_SEVERITY_INFO : JP_BOOT_SEVERITY_ERROR,
		JP_BOOT_SOURCE_STAGE2, context->stage2_result, 0);
}

/* The two events about the menu and the configuration ("4 Decided rules"). */
static void record_selection_events(JPBootContext *context)
{
	const JPBootConfig *config = &context->stage4.config;

	add_manager_event(context, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS, (uint32_t)context->stage4.selection_outcome);
	add_manager_event(context, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS,
		((uint32_t)config->state << 8) | (uint32_t)config->source);
}

static void create_error(JPBootContext *context, JPBootResult result, JPBootSourceComponent source, JPDiagnosticStage stage)
{
	JPBootErrorInput input;

	input.original_result = result;
	input.source_component = source;
	input.stage = stage;
	input.severity_override = JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED;
	input.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED;
	input.diagnostic_context = 0;
	(void)jp_boot_error_create(context, &input);     /* Stage 3 keeps only the first error: a refusal is ignored */
}

static JPBootDecision to_decision(JPRecoveryDecision decision)
{
	switch (decision) {
	case JP_RECOVERY_ENTER: return JP_BOOT_DECISION_ENTER_RECOVERY;
	case JP_RECOVERY_RETRY: return JP_BOOT_DECISION_RETRY_BOOT;
	default: return JP_BOOT_DECISION_TERMINAL_HALT;
	}
}

static JPRecoveryDecision to_recovery_decision(JPBootDecision decision)
{
	switch (decision) {
	case JP_BOOT_DECISION_ENTER_RECOVERY: return JP_RECOVERY_ENTER;
	case JP_BOOT_DECISION_RETRY_BOOT: return JP_RECOVERY_RETRY;
	default: return JP_RECOVERY_TERMINAL_HALT;
	}
}

static bool status_is_one_of_the_failed(const JPBootContext *context)
{
	JPBootStatusState status;

	if (jp_boot_status_get(context, &status) != JP_BOOT_SUCCESS) {
		return false;
	}
	return status.status == JP_BOOT_STATUS_FAILED || status.status == JP_BOOT_STATUS_RECOVERY ||
	       status.status == JP_BOOT_STATUS_HALTED;
}

/* A failure while the kernel is being started ("4 Decided rules"). Never returns. */
static _Noreturn void fail_kernel_start(JPBootContext *context, JPBootResult result)
{
	JPRecoveryDecision decision;

	create_error(context, result, JP_BOOT_SOURCE_BOOT_MANAGER, JP_DIAGNOSTIC_STAGE_STAGE4);
	(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE4, JP_BOOT_STATUS_FAILED);
	if (jp_boot_recovery_decide(context, &decision) != JP_BOOT_SUCCESS) {
		decision = JP_RECOVERY_TERMINAL_HALT;
	}
	context->stage4.decision = to_decision(decision);     /* the one time that the stored decision changes */
	jp_boot_recovery_enter(context, decision);
}

/* The Stage 3 records and the configuration agree: RUNNING, no error, a configuration that is VALIDATED. */
static bool records_agree(const JPBootContext *context)
{
	JPBootStatusState status;
	JPBootError error;
	JPBootConfig config;

	return jp_boot_status_get(context, &status) == JP_BOOT_SUCCESS && status.status == JP_BOOT_STATUS_RUNNING &&
	       jp_boot_error_get(context, &error) != JP_BOOT_SUCCESS &&
	       jp_boot_config_get(context, &config) == JP_BOOT_SUCCESS && config.state == JP_BOOT_CONFIG_STATE_VALIDATED;
}

/* The steps of the Freeze and Kernel Handoff Contract. Never returns. */
static _Noreturn void start_kernel(JPBootContext *context)
{
	JPBootConfig config;
	JPBootSummary summary;
	JPBootResult result;

	if (!records_agree(context)) {
		fail_kernel_start(context, JP_BOOT_FAILED);
	}
	if (jp_boot_config_get(context, &config) == JP_BOOT_SUCCESS && config.boot_logging &&
	    jp_boot_info_summarize(context, &summary) == JP_BOOT_SUCCESS) {
		result = jp_boot_ui_present_summary(context, &summary);
		if (result != JP_BOOT_SUCCESS) {
			add_manager_event(context, JP_BOOT_SEVERITY_WARNING, result, 0);     /* recorded; the decision does not change */
		}
	}
	result = jp_boot_report_finalize(context);
	if (result != JP_BOOT_SUCCESS) {
		fail_kernel_start(context, result);
	}
	result = jp_boot_config_freeze(context);
	if (result != JP_BOOT_SUCCESS) {
		fail_kernel_start(context, result);
	}
	kernel_main(context);
}

/* ---- Public API (phase B) ---- */

JPBootResult jp_boot_manager_decide(const JPBootContext *context, JPBootDecision *decision)
{
	JPBootConfig config;
	JPRecoveryDecision recovery;
	JPBootResult result;

	if (context == NULL || decision == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage2_result != JP_BOOT_SUCCESS || status_is_one_of_the_failed(context)) {
		result = jp_boot_recovery_decide(context, &recovery);
		if (result != JP_BOOT_SUCCESS) {
			return result;     /* unchanged; nothing is written */
		}
		*decision = to_decision(recovery);
		return JP_BOOT_SUCCESS;
	}
	if (jp_boot_config_get(context, &config) == JP_BOOT_SUCCESS && config.recovery_requested) {
		*decision = JP_BOOT_DECISION_ENTER_RECOVERY;
		return JP_BOOT_SUCCESS;
	}
	*decision = JP_BOOT_DECISION_CONTINUE_TO_KERNEL;
	return JP_BOOT_SUCCESS;
}

JPBootTransitionResult jp_boot_manager_request_recovery(JPBootContext *context, const JPBootRecoveryState *recovery)
{
	JPBootStatusState status;
	JPBootResult result;

	if (context == NULL || recovery == NULL) {
		return JP_BOOT_TRANSITION_REJECTED;
	}
	if ((unsigned int)recovery->decision > (unsigned int)JP_RECOVERY_TERMINAL_HALT ||
	    context->stage4.state != JP_BOOT_MANAGER_DECIDED) {
		return JP_BOOT_TRANSITION_REJECTED;
	}
	if (recovery->decision != JP_RECOVERY_ENTER) {
		return JP_BOOT_TRANSITION_TERMINAL;     /* RETRY cannot be carried out in Milestone 0; HALT has no destination */
	}

	if (jp_boot_status_get(context, &status) != JP_BOOT_SUCCESS) {
		return JP_BOOT_TRANSITION_REJECTED;
	}
	if (status.status != JP_BOOT_STATUS_RECOVERY &&
	    jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) != JP_BOOT_SUCCESS) {
		return JP_BOOT_TRANSITION_REJECTED;
	}
	result = jp_boot_ui_present_failure(context);
	if (result != JP_BOOT_SUCCESS) {
		add_manager_event(context, JP_BOOT_SEVERITY_WARNING, result, 0);       /* recorded; the request goes on */
	}
	if (jp_boot_report_finalize(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_TRANSITION_REJECTED;
	}
	(void)jp_boot_config_freeze(context);                  /* an UNSET configuration stays as it is */
	context->stage4.state = JP_BOOT_MANAGER_RECOVERY_REQUESTED;
	stop_machine();                                        /* the recovery diagnostics state; never returns */
}

_Noreturn void jp_kernel_stage2_complete(JPBootContext *context)
{
	JPBootManagerState entry_state;
	JPBootDecision decision;
	bool stage4_ready;

	if (context == NULL) {
		stop_machine();
	}
	entry_state = context->stage4.state;
	if (entry_state != JP_BOOT_MANAGER_UNINITIALIZED && entry_state != JP_BOOT_MANAGER_PREPARED &&
	    entry_state != JP_BOOT_MANAGER_SELECTED) {
		stop_machine();     /* a second entry */
	}
	context->stage4.state = JP_BOOT_MANAGER_COMPLETING;

	if (jp_boot_log_initialize(context) != JP_BOOT_SUCCESS || jp_boot_status_initialize(context) != JP_BOOT_SUCCESS) {
		stop_machine();     /* nothing can be recorded */
	}
	record_stage2_result(context);
	record_selection_events(context);

	(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING);
	stage4_ready = context->stage4.config.state != JP_BOOT_CONFIG_STATE_UNSET &&
	               (entry_state == JP_BOOT_MANAGER_PREPARED || entry_state == JP_BOOT_MANAGER_SELECTED);
	if (context->stage2_result == JP_BOOT_SUCCESS && stage4_ready) {
		(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING);
	} else if (context->stage2_result != JP_BOOT_SUCCESS) {
		create_error(context, context->stage2_result, JP_BOOT_SOURCE_STAGE2, JP_DIAGNOSTIC_STAGE_STAGE2);
		(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED);
	} else {
		create_error(context, JP_BOOT_NOT_INITIALIZED, JP_BOOT_SOURCE_BOOT_MANAGER, JP_DIAGNOSTIC_STAGE_STAGE4);
		(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE4, JP_BOOT_STATUS_FAILED);
	}

	if (jp_boot_manager_decide(context, &decision) != JP_BOOT_SUCCESS) {
		decision = JP_BOOT_DECISION_TERMINAL_HALT;     /* Stage 3 cannot say: the safe answer */
	}
	context->stage4.decision = decision;
	context->stage4.state = JP_BOOT_MANAGER_DECIDED;

	if (decision == JP_BOOT_DECISION_CONTINUE_TO_KERNEL) {
		start_kernel(context);
	}
	jp_boot_recovery_enter(context, to_recovery_decision(decision));
}

#endif
