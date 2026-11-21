/*
 * Unit test (level U) of src/boot/recovery.c: the decision rules, entering
 * recovery, the retry (reset and preserve, and the undo), and the terminal
 * path. The real log, status, error, and report modules keep the records. The
 * Stage 4 function, the panic, and the halt are fakes.
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>

#include "diagnostic_hw.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/error.h"
#include "justos/boot/panic.h"
#include "justos/boot/recovery.h"

/* ---- The fakes ---- */

static JPBootTransitionResult transition_result;
static unsigned int transition_calls;
static JPBootRecoveryState transition_state;
static JPBootStatus status_at_transition;
static JPBootContext *transition_context;
static unsigned int halts;
static unsigned int panics;
static JPBootError panic_error;
static JPBootStatus status_at_panic;
static bool report_final_at_panic;
static jmp_buf escape;

JPBootTransitionResult jp_boot_manager_request_recovery(JPBootContext *context, const JPBootRecoveryState *recovery)
{
	transition_calls++;
	transition_context = context;
	transition_state = *recovery;
	status_at_transition = context->stage3.status.status;
	return transition_result;
}

_Noreturn void jp_diagnostic_halt(void)
{
	halts++;
	longjmp(escape, 1);
}

_Noreturn void jp_boot_panic(JPBootContext *context, const JPBootError *error)
{
	panics++;
	panic_error = *error;
	status_at_panic = context->stage3.status.status;
	report_final_at_panic = context->stage3.report.finalized;
	longjmp(escape, 2);
}

/* ---- Setup ---- */

static JPBootContext context;

static void setup(JPArchitectureType architecture)
{
	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = architecture;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
	transition_result = JP_BOOT_TRANSITION_ACCEPTED;
	transition_calls = halts = panics = 0;
}

static void set_limit(uint32_t maximum)
{
	context.stage3.attempt.maximum_retry_count = maximum;
	context.stage3.recovery.maximum_retry_count = maximum;
}

static void add_error(JPBootResult result)
{
	JPBootErrorInput in;

	memset(&in, 0, sizeof in);
	in.original_result = result;
	in.source_component = JP_BOOT_SOURCE_STAGE2;
	in.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	in.diagnostic_context = 0x77;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
}

/* A failed attempt: INITIALIZING then FAILED with the given result. */
static void fail_with(JPBootResult result)
{
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(result);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
}

static void running(void)
{
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
}

static JPRecoveryDecision decide(void)
{
	JPRecoveryDecision d = (JPRecoveryDecision)99;

	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_SUCCESS);
	return d;
}

/* Runs recovery_enter; returns how it ended: 1 halt, 2 panic. */
static int run_enter(JPBootContext *target, JPRecoveryDecision decision)
{
	int outcome = setjmp(escape);

	if (outcome == 0) {
		jp_boot_recovery_enter(target, decision);
		assert(0);                         /* it never returns */
	}
	return outcome;
}

/* ---- decide ---- */

static void test_decide_by_result(void)
{
	static const struct {
		JPBootResult result;
		JPRecoveryDecision decision;
	} table[] = {
		{ JP_BOOT_ERROR_INVALID_PARAM, JP_RECOVERY_TERMINAL_HALT },
		{ JP_BOOT_ERROR_CORRUPTED_DATA, JP_RECOVERY_TERMINAL_HALT },
		{ JP_BOOT_FAILED, JP_RECOVERY_ENTER },
		{ JP_BOOT_NOT_INITIALIZED, JP_RECOVERY_ENTER },
		{ JP_BOOT_NOT_DETECTED, JP_RECOVERY_ENTER },
		{ JP_BOOT_UNKNOWN, JP_RECOVERY_ENTER }
	};
	size_t i;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		for (i = 0; i < sizeof table / sizeof table[0]; i++) {
			setup(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64);
			fail_with(table[i].result);
			assert(decide() == table[i].decision);
		}
	}
}

static void test_decide_without_error(void)
{
	JPRecoveryDecision d;

	setup(JP_ARCH_X86_64);
	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_FAILED);                /* BOOTING: no error, no recovery */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_FAILED);                /* INITIALIZING too */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
	assert(decide() == JP_RECOVERY_ENTER);                                          /* RUNNING: asked for through the configuration */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_FAILED);                /* RECOVERY without an error */
}

static void test_decide_terminal_states(void)
{
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	set_limit(5);
	assert(decide() == JP_RECOVERY_RETRY);
	context.stage3.panic = JP_BOOT_PANIC_TERMINAL;                                   /* a panic-terminal attempt cannot be retried */
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);

	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	set_limit(5);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);

	/* a panic that is only active or requested does not forbid the decision, but a retry needs NORMAL */
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	set_limit(5);
	context.stage3.panic = JP_BOOT_PANIC_ACTIVE;
	assert(decide() == JP_RECOVERY_ENTER);
}

static void test_decide_retry_rules(void)
{
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(decide() == JP_RECOVERY_ENTER);              /* the maximum is 0: never a retry */

	set_limit(1);
	assert(decide() == JP_RECOVERY_RETRY);              /* 0 < 1 */
	context.stage3.recovery.retry_count = 1;
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);      /* 1 > 0 and 1 >= 1: exhausted */
	set_limit(2);
	context.stage3.recovery.retry_count = 1;
	assert(decide() == JP_RECOVERY_RETRY);              /* 1 < 2 */
	context.stage3.recovery.retry_count = 2;
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);
	context.stage3.recovery.retry_count = 7;            /* above the limit: no wrap, still terminal */
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);

	/* RETRY needs FAILED or RECOVERY: from RUNNING with an error it is ENTER */
	setup(JP_ARCH_X86_64);
	set_limit(3);
	running();
	add_error(JP_BOOT_FAILED);
	assert(decide() == JP_RECOVERY_ENTER);
}

static void test_decide_refusals(void)
{
	JPRecoveryDecision d;

	assert(jp_boot_recovery_decide(NULL, &d) == JP_BOOT_ERROR_INVALID_PARAM);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_recovery_decide(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	memset(&context, 0, sizeof context);
	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_NOT_INITIALIZED);
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_recovery_decide(&context, &d) == JP_BOOT_NOT_INITIALIZED);       /* no status yet */
}

static void test_decide_changes_nothing(void)
{
	static JPBootContext copy;

	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	memcpy(&copy, &context, sizeof copy);
	(void)decide();
	assert(memcmp(&copy, &context, sizeof copy) == 0);
}

/* ---- enter: ENTER_RECOVERY ---- */

static void test_enter_recovery(void)
{
	JPBootStatusState s;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		setup(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64);
		fail_with(JP_BOOT_FAILED);
		assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);               /* ACCEPTED: the processor is stopped */
		assert(transition_calls == 1 && transition_context == &context);
		assert(transition_state.decision == JP_RECOVERY_ENTER);
		assert(status_at_transition == JP_BOOT_STATUS_RECOVERY);             /* RECOVERY before the request */
		assert(jp_boot_status_get(&context, &s) == JP_BOOT_SUCCESS && s.status == JP_BOOT_STATUS_RECOVERY);
		assert(halts == 1 && panics == 0);
		assert(context.stage3.recovery.decision == JP_RECOVERY_ENTER);
		assert(!context.stage3.report.finalized);                          /* nothing was made final */
	}
}

static void test_enter_recovery_without_error(void)
{
	setup(JP_ARCH_X86_64);
	running();
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);
	assert(context.stage3.status.status == JP_BOOT_STATUS_RECOVERY && transition_calls == 1);
}

static void test_enter_when_already_in_recovery(void)
{
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);
	assert(transition_calls == 1 && context.stage3.status.status == JP_BOOT_STATUS_RECOVERY);
}

static void test_enter_rejected_or_terminal(void)
{
	JPBootTransitionResult results[2];
	size_t i;

	results[0] = JP_BOOT_TRANSITION_REJECTED;
	results[1] = JP_BOOT_TRANSITION_TERMINAL;
	for (i = 0; i < 2; i++) {
		setup(JP_ARCH_X86_64);
		fail_with(JP_BOOT_FAILED);
		transition_result = results[i];
		assert(run_enter(&context, JP_RECOVERY_ENTER) == 2);               /* the terminal path: the panic screen */
		assert(panics == 1 && halts == 0);
		assert(panic_error.original_result == JP_BOOT_FAILED);
		assert(status_at_panic == JP_BOOT_STATUS_HALTED && report_final_at_panic);
		assert(context.stage3.report.finalized);
	}
	/* an answer that is not in the enumeration is not ACCEPTED */
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	transition_result = (JPBootTransitionResult)9;
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 2);
}

static void test_enter_cannot_reach_recovery(void)
{
	/* the status graph does not allow RECOVERY from INITIALIZING: the terminal path (no error recorded: a plain halt) */
	setup(JP_ARCH_X86_64);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);
	assert(transition_calls == 0 && panics == 0 && halts == 1);
	assert(context.stage3.report.finalized);
}

/* ---- enter: the terminal path ---- */

static void test_terminal_halt(void)
{
	/* with an error: HALTED, the report is final, the panic screen shows the error */
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(run_enter(&context, JP_RECOVERY_TERMINAL_HALT) == 2);
	assert(transition_calls == 0 && panics == 1);
	assert(panic_error.original_result == JP_BOOT_ERROR_CORRUPTED_DATA && panic_error.diagnostic_context == 0x77);
	assert(status_at_panic == JP_BOOT_STATUS_HALTED && report_final_at_panic);
	assert(context.stage3.recovery.decision == JP_RECOVERY_TERMINAL_HALT);

	/* without an error: a plain halt, report final */
	setup(JP_ARCH_X86_64);
	running();
	assert(run_enter(&context, JP_RECOVERY_TERMINAL_HALT) == 1);
	assert(panics == 0 && halts == 1 && context.stage3.report.finalized);
	assert(context.stage3.status.status == JP_BOOT_STATUS_RUNNING);          /* RUNNING cannot go to HALTED: unchanged */

	/* from RECOVERY: HALTED */
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(run_enter(&context, JP_RECOVERY_TERMINAL_HALT) == 2);
	assert(status_at_panic == JP_BOOT_STATUS_HALTED);
}

static void test_invalid_decision_is_terminal(void)
{
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(run_enter(&context, (JPRecoveryDecision)9) == 2);
	assert(transition_calls == 0);
	assert(context.stage3.recovery.decision == JP_RECOVERY_TERMINAL_HALT);
}

static void test_no_context(void)
{
	setup(JP_ARCH_X86_64);
	assert(run_enter(NULL, JP_RECOVERY_TERMINAL_HALT) == 1);
	assert(halts == 1 && transition_calls == 0 && panics == 0);
	assert(run_enter(NULL, JP_RECOVERY_ENTER) == 1);                         /* any decision: it only stops */
	assert(run_enter(NULL, JP_RECOVERY_RETRY) == 1);
	assert(transition_calls == 0);

	memset(&context, 0, sizeof context);                                    /* the log never started */
	halts = 0;
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);
	assert(halts == 1 && transition_calls == 0 && !context.stage3.report.finalized);

	setup(JP_ARCH_X86_64);
	context.stage3.status_initialized = false;                              /* the log started, the status did not */
	assert(run_enter(&context, JP_RECOVERY_ENTER) == 1);
	assert(transition_calls == 0);
}

/* ---- enter: RETRY ---- */

static void test_retry(void)
{
	JPBootEvent ev;
	JPBootLogStatus log;
	size_t events_before;

	setup(JP_ARCH_X86_64);
	set_limit(2);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_log_get_status(&context, &log) == JP_BOOT_SUCCESS);
	events_before = log.event_count;
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 1);                     /* ACCEPTED: stopped; the new attempt has begun */
	assert(transition_calls == 1 && transition_state.decision == JP_RECOVERY_RETRY);

	/* the reset and preserve table */
	assert(context.stage3.attempt.boot_attempt_id == 2 && context.stage3.attempt.attempt_sequence == 1);
	assert(context.stage3.attempt.retry_count == 1 && context.stage3.attempt.maximum_retry_count == 2);
	assert(context.stage3.recovery.current_attempt_id == 2 && context.stage3.recovery.previous_attempt_id == 1);
	assert(context.stage3.recovery.retry_count == 1 && context.stage3.recovery.maximum_retry_count == 2);
	assert(context.stage3.recovery.decision == JP_RECOVERY_RETRY);
	assert(!context.stage3.error_recorded);                                  /* a new attempt starts without a current error */
	assert(context.stage3.status.status == JP_BOOT_STATUS_BOOTING);
	assert(context.stage3.panic == JP_BOOT_PANIC_NORMAL);
	assert(context.stage3.log.next_event_sequence == 0);                      /* the event sequence restarts */
	assert(!context.stage3.report.finalized);
	/* the log keeps its events and holds the old error as history */
	assert(jp_boot_log_get_status(&context, &log) == JP_BOOT_SUCCESS && log.event_count == events_before + 1);
	assert(jp_boot_log_get(&context, events_before, &ev) == JP_BOOT_SUCCESS);
	assert(ev.result == JP_BOOT_FAILED && ev.diagnostic_context == 0x77 && ev.source_component == JP_BOOT_SOURCE_STAGE2);
	assert(ev.boot_attempt_id == 1);                                         /* it belongs to the old attempt */
	assert(jp_boot_log_get(&context, 0, &ev) == JP_BOOT_SUCCESS && ev.boot_attempt_id == 1);
	/* the first event of the new attempt gets sequence 0 and the new identity */
	{
		JPBootEvent e;

		memset(&e, 0, sizeof e);
		e.stage = JP_DIAGNOSTIC_STAGE_STAGE1;
		e.severity = JP_BOOT_SEVERITY_INFO;
		e.architecture = JP_ARCH_X86_64;
		e.source_component = JP_BOOT_SOURCE_STAGE1;
		assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
		assert(jp_boot_log_get(&context, events_before + 1, &ev) == JP_BOOT_SUCCESS);
		assert(ev.event_sequence == 0 && ev.boot_attempt_id == 2);
	}
}

static void test_retry_budget(void)
{
	/* two retries allowed: the second works, then the budget is used up and the decision is terminal */
	setup(JP_ARCH_X86_64);
	set_limit(2);
	fail_with(JP_BOOT_FAILED);
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 1);
	/* the new attempt fails again */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_NOT_DETECTED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide() == JP_RECOVERY_RETRY);
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 1);
	assert(context.stage3.attempt.boot_attempt_id == 3 && context.stage3.attempt.retry_count == 2);
	assert(context.stage3.recovery.previous_attempt_id == 2);
	/* the third failure: exhausted, terminal */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide() == JP_RECOVERY_TERMINAL_HALT);
	transition_calls = 0;
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 2);                     /* a forced RETRY is refused: the terminal path */
	assert(transition_calls == 0 && context.stage3.attempt.retry_count == 2 && context.stage3.attempt.boot_attempt_id == 3);
}

static void test_retry_refused_is_undone(void)
{
	static JPBootContext before;
	JPBootTransitionResult results[2];
	size_t i;

	results[0] = JP_BOOT_TRANSITION_REJECTED;
	results[1] = JP_BOOT_TRANSITION_TERMINAL;
	for (i = 0; i < 2; i++) {
		setup(JP_ARCH_X86_64);
		set_limit(2);
		fail_with(JP_BOOT_FAILED);
		memcpy(&before, &context, sizeof before);
		transition_result = results[i];
		assert(run_enter(&context, JP_RECOVERY_RETRY) == 2);                 /* the terminal path */
		/* the old attempt is still current and nothing of the reset is visible */
		assert(context.stage3.attempt.boot_attempt_id == 1 && context.stage3.attempt.retry_count == 0);
		assert(context.stage3.recovery.current_attempt_id == 1 && context.stage3.recovery.previous_attempt_id == 0);
		assert(context.stage3.recovery.retry_count == 0);
		assert(context.stage3.error_recorded && context.stage3.error.original_result == JP_BOOT_FAILED);
		assert(transition_state.current_attempt_id == 2);                    /* Stage 4 was asked with the new state */
		assert(status_at_panic == JP_BOOT_STATUS_HALTED);
		assert(context.stage3.log.next_event_sequence == before.stage3.log.next_event_sequence);
		assert(context.stage3.log.status.event_count == before.stage3.log.status.event_count);
		assert(memcmp(context.stage3.log.events, before.stage3.log.events, sizeof before.stage3.log.events) == 0);
	}
}

static void test_retry_history_when_the_log_is_full(void)
{
	JPBootEvent e;
	size_t i;

	/* a full log has no room for the history event: it is skipped, the retry still happens */
	setup(JP_ARCH_X86_64);
	set_limit(2);
	memset(&e, 0, sizeof e);
	e.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	e.severity = JP_BOOT_SEVERITY_ERROR;
	e.architecture = JP_ARCH_X86_64;
	e.source_component = JP_BOOT_SOURCE_STAGE2;
	e.result = JP_BOOT_FAILED;
	for (i = 1; i < JP_BOOT_LOG_CAPACITY; i++) {
		assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	}
	fail_with(JP_BOOT_FAILED);
	assert(context.stage3.log.status.event_count == JP_BOOT_LOG_CAPACITY);
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 1);
	assert(context.stage3.attempt.boot_attempt_id == 2 && context.stage3.log.status.event_count == JP_BOOT_LOG_CAPACITY);
	assert(context.stage3.log.status.discarded_event_count == 0);
}

static void test_retry_not_allowed(void)
{
	/* the maximum is 0: a RETRY request is refused, the terminal path (an ENTER cannot be made out of it) */
	setup(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 2);
	assert(transition_calls == 0 && context.stage3.attempt.boot_attempt_id == 1);

	/* a non-recoverable error */
	setup(JP_ARCH_X86_64);
	set_limit(3);
	fail_with(JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 2);
	assert(transition_calls == 0);

	/* the identity counter cannot wrap */
	setup(JP_ARCH_X86_64);
	set_limit(3);
	fail_with(JP_BOOT_FAILED);
	context.stage3.attempt.boot_attempt_id = UINT64_MAX;
	context.stage3.recovery.current_attempt_id = UINT64_MAX;
	context.stage3.error.boot_attempt_id = UINT64_MAX;
	assert(run_enter(&context, JP_RECOVERY_RETRY) == 2);
	assert(transition_calls == 0 && context.stage3.attempt.boot_attempt_id == UINT64_MAX);
}

int main(void)
{
	test_decide_by_result();
	test_decide_without_error();
	test_decide_terminal_states();
	test_decide_retry_rules();
	test_decide_refusals();
	test_decide_changes_nothing();
	test_enter_recovery();
	test_enter_recovery_without_error();
	test_enter_when_already_in_recovery();
	test_enter_rejected_or_terminal();
	test_enter_cannot_reach_recovery();
	test_terminal_halt();
	test_invalid_decision_is_terminal();
	test_no_context();
	test_retry();
	test_retry_budget();
	test_retry_refused_is_undone();
	test_retry_history_when_the_log_is_full();
	test_retry_not_allowed();
	return 0;
}
