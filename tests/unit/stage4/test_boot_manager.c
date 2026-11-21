/*
 * Unit test (level U) of phase B of src/boot_manager.c: jp_kernel_stage2_complete (success, a Stage 2 failure, a
 * Stage 4 failure, recovery asked for through the configuration, a second entry), jp_boot_manager_decide (every rule),
 * and jp_boot_manager_request_recovery (every decision and every reason for rejection). The real boot_config.c,
 * boot_info.c, boot_ui.c, boot_status.c, boot_log.c, error.c, boot_report.c, and recovery.c are used, so the whole
 * Stage 4 to Stage 3 flow runs. The machine access (halt), the panic screen, the text output, and kernel_main
 * are fakes that stop the run with a long jump, so that the test can look at the state in which the flow ended.
 */

#include <assert.h>
#include <setjmp.h>
#include <string.h>

#include "diagnostic_hw.h"
#include "justos/boot/boot_config.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/error.h"
#include "justos/boot/panic.h"
#include "justos/boot/recovery.h"
#include "justos/kernel/init.h"

#define MAX_LINES 400U
#define LINE_LENGTH 128U

enum { END_NONE = 0, END_HALT = 1, END_PANIC = 2, END_KERNEL = 3 };

static JPBootContext context;

/* ---- The fakes ---- */

static jmp_buf escape;
static unsigned int halts;
static unsigned int panics;
static unsigned int kernel_mains;
static JPBootContext *kernel_main_context;
static JPBootError panic_error;

static char lines[MAX_LINES][LINE_LENGTH];
static size_t line_count;
static JPBootResult output_result;               /* what the text output answers */
static void (*output_hook)(void);                /* called at the first line; the test can change the context */
static bool hook_done;

_Noreturn void jp_diagnostic_halt(void)
{
	halts++;
	longjmp(escape, END_HALT);
}

_Noreturn void jp_boot_panic(JPBootContext *ctx, const JPBootError *error)
{
	(void)ctx;
	panics++;
	panic_error = *error;
	longjmp(escape, END_PANIC);
}

_Noreturn void kernel_main(JPBootContext *ctx)
{
	kernel_mains++;
	kernel_main_context = ctx;
	longjmp(escape, END_KERNEL);
}

JPBootResult jp_boot_diagnostic_output(JPBootContext *ctx, const JPBootDiagnosticMessage *message)
{
	assert(ctx == &context && message != NULL);
	assert(message->stage == JP_DIAGNOSTIC_STAGE_STAGE4 && message->source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	if (line_count < MAX_LINES && message->length < LINE_LENGTH) {
		memcpy(lines[line_count], message->text, message->length);
		lines[line_count][message->length] = '\0';
		line_count++;
	}
	if (output_hook != NULL && !hook_done) {
		hook_done = true;
		output_hook();
	}
	return output_result;
}

/* ---- Setup and helpers ---- */

static void set_up(JPBootResult stage2_result, JPBootManagerState state)
{
	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = JP_ARCH_AARCH64;
	context.stage2_result = stage2_result;
	if (state == JP_BOOT_MANAGER_PREPARED || state == JP_BOOT_MANAGER_SELECTED) {
		assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
		context.stage4.selection_outcome = state == JP_BOOT_MANAGER_SELECTED ? JP_BOOT_SELECTION_APPLIED : JP_BOOT_SELECTION_NOT_RUN;
	}
	context.stage4.state = state;
	halts = panics = kernel_mains = 0;
	kernel_main_context = NULL;
	line_count = 0;
	output_result = JP_BOOT_SUCCESS;
	output_hook = NULL;
	hook_done = false;
}

static void set_config(bool boot_logging, bool verbose, bool recovery_requested)
{
	JPBootConfig c;

	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	c.boot_logging = boot_logging;
	c.verbose_output = verbose;
	c.recovery_requested = recovery_requested;
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);
	context.stage4.config.source = JP_BOOT_CONFIG_SOURCE_DEFAULT;      /* the menu was not used */
}

/* Runs jp_kernel_stage2_complete until something ends it; returns how it ended. */
static int complete(void)
{
	int ended = setjmp(escape);

	if (ended == 0) {
		jp_kernel_stage2_complete(&context);
	}
	return ended;
}

static bool has_line(const char *text)
{
	for (size_t i = 0; i < line_count; i++) {
		if (strcmp(lines[i], text) == 0) {
			return true;
		}
	}
	return false;
}

static JPBootEvent event(size_t index)
{
	JPBootEvent e;

	assert(jp_boot_log_get(&context, index, &e) == JP_BOOT_SUCCESS);
	return e;
}

static size_t event_count(void)
{
	JPBootLogStatus s;

	assert(jp_boot_log_get_status(&context, &s) == JP_BOOT_SUCCESS);
	return s.event_count;
}

static JPBootStatus status_now(void)
{
	JPBootStatusState s;

	assert(jp_boot_status_get(&context, &s) == JP_BOOT_SUCCESS);
	return s.status;
}

static JPBootError error_now(void)
{
	JPBootError e;

	assert(jp_boot_error_get(&context, &e) == JP_BOOT_SUCCESS);
	return e;
}

static bool error_recorded(void)
{
	JPBootError e;

	return jp_boot_error_get(&context, &e) == JP_BOOT_SUCCESS;
}

static bool report_is_final(void)
{
	JPBootReport r;

	return jp_boot_report_get(&context, &r) == JP_BOOT_SUCCESS && r.finalized;
}

/* ---- jp_kernel_stage2_complete: success ---- */

static void test_success(void)
{
	JPBootEvent e;

	for (int variant = 0; variant < 2; variant++) {
		JPBootManagerState from = variant == 0 ? JP_BOOT_MANAGER_SELECTED : JP_BOOT_MANAGER_PREPARED;

		set_up(JP_BOOT_SUCCESS, from);
		assert(complete() == END_KERNEL);
		assert(kernel_mains == 1 && kernel_main_context == &context && halts == 0 && panics == 0);
		assert(context.stage4.decision == JP_BOOT_DECISION_CONTINUE_TO_KERNEL);
		assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED);
		assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_FROZEN);
		assert(report_is_final() && status_now() == JP_BOOT_STATUS_RUNNING && !error_recorded());
		/* The events: the one that the log writes itself, the Stage 2 result, the menu outcome, the configuration. */
		assert(event_count() == 4);
		e = event(1);
		assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE2 && e.source_component == JP_BOOT_SOURCE_STAGE2);
		assert(e.severity == JP_BOOT_SEVERITY_INFO && e.result == JP_BOOT_SUCCESS && e.diagnostic_context == 0);
		e = event(2);
		assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE4 && e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
		assert(e.severity == JP_BOOT_SEVERITY_INFO && e.result == JP_BOOT_SUCCESS);
		assert(e.diagnostic_context == (uint32_t)(variant == 0 ? JP_BOOT_SELECTION_APPLIED : JP_BOOT_SELECTION_NOT_RUN));
		e = event(3);
		assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE4 && e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
		assert(e.severity == JP_BOOT_SEVERITY_INFO && e.result == JP_BOOT_SUCCESS);
		assert(e.diagnostic_context == (((uint32_t)JP_BOOT_CONFIG_STATE_VALIDATED << 8) | (uint32_t)JP_BOOT_CONFIG_SOURCE_DEFAULT));
		/* boot_logging is on by default: the summary was shown (11 lines), and not the events. */
		assert(line_count == 11);
		assert(has_line("Boot status: RUNNING") && has_line("Configuration: VALIDATED (DEFAULT)"));
		assert(has_line(variant == 0 ? "Menu: APPLIED" : "Menu: NOT_RUN"));
	}
}

static void test_success_configuration_source_in_event(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	{
		JPBootConfig c;

		assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
		assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);        /* source INTERACTIVE */
	}
	assert(complete() == END_KERNEL);
	assert(event(3).diagnostic_context == (((uint32_t)JP_BOOT_CONFIG_STATE_VALIDATED << 8) | (uint32_t)JP_BOOT_CONFIG_SOURCE_INTERACTIVE));
	assert(has_line("Configuration: VALIDATED (INTERACTIVE)"));
}

static void test_success_logging_switches(void)
{
	/* boot_logging off: nothing is shown. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	set_config(false, false, false);
	assert(complete() == END_KERNEL && line_count == 0);
	assert(report_is_final() && context.stage4.config.state == JP_BOOT_CONFIG_STATE_FROZEN);

	/* verbose: the events follow the summary (the four events of the log). */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	set_config(true, true, false);
	assert(complete() == END_KERNEL);
	assert(line_count == 11 + 4);
	assert(has_line("Event 0: STAGE1 INFO JP_BOOT_SUCCESS STAGE1"));
	assert(has_line("Event 3: STAGE4 INFO JP_BOOT_SUCCESS BOOT_MANAGER"));
	assert(!has_line("Event 4: STAGE4 INFO JP_BOOT_SUCCESS BOOT_MANAGER"));
}

/* The summary cannot be shown: the failure is recorded, and the kernel is still started. */
static void test_success_summary_failure(void)
{
	JPBootEvent e;

	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	output_result = JP_BOOT_FAILED;
	assert(complete() == END_KERNEL);
	assert(context.stage4.decision == JP_BOOT_DECISION_CONTINUE_TO_KERNEL && report_is_final());
	assert(event_count() == 5);
	e = event(4);
	assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE4 && e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(e.severity == JP_BOOT_SEVERITY_WARNING && e.result == JP_BOOT_FAILED);
	assert(line_count == 11);                    /* every line was tried */
}

/* The report cannot be finalized: the failure while the kernel is being started ("4 Decided rules"). */
static void corrupt_status(void)
{
	context.stage3.status.status = JP_BOOT_STATUS_FAILED;          /* FAILED without an error: the report refuses */
}

static void test_failure_while_starting(void)
{
	JPBootError e;

	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	output_hook = corrupt_status;
	/* The finalization says JP_BOOT_ERROR_CORRUPTED_DATA, which Stage 3 classifies as not recoverable: panic. */
	assert(complete() == END_PANIC);
	assert(kernel_mains == 0 && panics == 1);
	/* The error says what went wrong: Stage 4, source BOOT_MANAGER; the original result of the finalization. */
	e = error_now();
	assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE4 && e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(e.original_result == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(panic_error.original_result == JP_BOOT_ERROR_CORRUPTED_DATA && panic_error.stage == JP_DIAGNOSTIC_STAGE_STAGE4);
	/* The stored decision is replaced once: the kernel was not started, so CONTINUE would no longer be true. */
	assert(context.stage4.decision == JP_BOOT_DECISION_TERMINAL_HALT);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED);
	assert(status_now() == JP_BOOT_STATUS_HALTED);
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_VALIDATED);      /* never frozen after a failure */
}

/* Stage 3 loses its log while the kernel is being started: the finalization says JP_BOOT_NOT_INITIALIZED, and Stage 3
 * cannot decide anything any more. The safe answer is a terminal halt, and the stored decision says so. */
static void lose_log(void)
{
	context.stage3.log_initialized = false;
}

static void test_failure_while_starting_stage3_lost(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	output_hook = lose_log;
	assert(complete() == END_HALT);
	assert(kernel_mains == 0 && panics == 0 && halts == 1);
	assert(context.stage4.decision == JP_BOOT_DECISION_TERMINAL_HALT);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED);
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_VALIDATED);
}

/* ---- jp_kernel_stage2_complete: a Stage 2 failure ---- */

static void test_stage2_failure_enters_recovery(void)
{
	JPBootEvent e;
	JPBootError err;

	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_SELECTED);
	assert(complete() == END_HALT);
	assert(kernel_mains == 0 && panics == 0 && halts == 1);
	assert(context.stage4.decision == JP_BOOT_DECISION_ENTER_RECOVERY);
	assert(context.stage4.state == JP_BOOT_MANAGER_RECOVERY_REQUESTED);
	err = error_now();
	assert(err.original_result == JP_BOOT_FAILED && err.stage == JP_DIAGNOSTIC_STAGE_STAGE2);
	assert(err.source_component == JP_BOOT_SOURCE_STAGE2);
	assert(status_now() == JP_BOOT_STATUS_RECOVERY && report_is_final());
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_FROZEN);
	e = event(1);
	assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE2 && e.severity == JP_BOOT_SEVERITY_ERROR && e.result == JP_BOOT_FAILED);
	/* The failure screen: the status, the error, and the events (the summary is not shown). */
	assert(has_line("JustOS boot diagnostics") && has_line("Status: RECOVERY"));
	assert(has_line("Error: JP_BOOT_FAILED in STAGE2"));
	assert(!has_line("Architecture: AArch64"));
	assert(has_line("Events: 4"));
}

static void test_stage2_failure_not_recoverable_halts(void)
{
	JPBootResult results[] = { JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_ERROR_CORRUPTED_DATA };

	for (size_t i = 0; i < 2; i++) {
		set_up(results[i], JP_BOOT_MANAGER_PREPARED);
		assert(complete() == END_PANIC);
		assert(context.stage4.decision == JP_BOOT_DECISION_TERMINAL_HALT);
		assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED);       /* the recovery request was never made */
		assert(panic_error.original_result == results[i] && panic_error.stage == JP_DIAGNOSTIC_STAGE_STAGE2);
		assert(status_now() == JP_BOOT_STATUS_HALTED && kernel_mains == 0);
		assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_VALIDATED);     /* not frozen: nothing continues */
	}
}

static void test_stage2_failure_without_prepare(void)
{
	/* Stage 4 was never prepared: the configuration is UNSET, and it stays so (freezing is not done). */
	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_UNINITIALIZED);
	assert(complete() == END_HALT);
	assert(context.stage4.decision == JP_BOOT_DECISION_ENTER_RECOVERY);
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
	assert(error_now().original_result == JP_BOOT_FAILED && error_now().source_component == JP_BOOT_SOURCE_STAGE2);
	assert(context.stage4.state == JP_BOOT_MANAGER_RECOVERY_REQUESTED && report_is_final());
}

/* ---- jp_kernel_stage2_complete: a Stage 4 failure ---- */

static void test_stage4_failure(void)
{
	JPBootError e;
	JPBootEvent ev;

	/* Stage 2 succeeded, but Stage 4 was not prepared (state UNINITIALIZED, configuration UNSET). */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_UNINITIALIZED);
	assert(complete() == END_HALT);
	assert(kernel_mains == 0);
	e = error_now();
	assert(e.original_result == JP_BOOT_NOT_INITIALIZED && e.stage == JP_DIAGNOSTIC_STAGE_STAGE4);
	assert(e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(context.stage4.decision == JP_BOOT_DECISION_ENTER_RECOVERY);     /* NOT_INITIALIZED is recoverable */
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET && report_is_final());
	/* The event of the Stage 2 result is still INFO: Stage 2 did succeed. */
	ev = event(1);
	assert(ev.stage == JP_DIAGNOSTIC_STAGE_STAGE2 && ev.severity == JP_BOOT_SEVERITY_INFO && ev.result == JP_BOOT_SUCCESS);
	/* The configuration event reports UNSET and UNKNOWN. */
	assert(event(3).diagnostic_context == 0);
	assert(event(2).diagnostic_context == (uint32_t)JP_BOOT_SELECTION_NOT_RUN);
}

/* The state says PREPARED but the configuration is UNSET: also a Stage 4 failure. */
static void test_stage4_failure_config_unset(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_UNINITIALIZED);
	context.stage4.state = JP_BOOT_MANAGER_PREPARED;
	assert(complete() == END_HALT);
	assert(error_now().source_component == JP_BOOT_SOURCE_BOOT_MANAGER && error_now().original_result == JP_BOOT_NOT_INITIALIZED);
	assert(kernel_mains == 0);
}

/* The configuration is set but the manager was never prepared (UNINITIALIZED): also a Stage 4 failure. */
static void test_stage4_failure_state_uninitialized(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_PREPARED);
	context.stage4.state = JP_BOOT_MANAGER_UNINITIALIZED;
	assert(complete() == END_HALT);
	assert(error_now().source_component == JP_BOOT_SOURCE_BOOT_MANAGER && kernel_mains == 0);
}

/* ---- jp_kernel_stage2_complete: recovery asked for through the configuration ---- */

static void test_recovery_requested(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	set_config(true, false, true);
	assert(complete() == END_HALT);
	assert(kernel_mains == 0 && panics == 0);
	assert(context.stage4.decision == JP_BOOT_DECISION_ENTER_RECOVERY);
	assert(context.stage4.state == JP_BOOT_MANAGER_RECOVERY_REQUESTED);
	assert(status_now() == JP_BOOT_STATUS_RECOVERY && !error_recorded() && report_is_final());
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_FROZEN);
	assert(has_line("Status: RECOVERY") && has_line("Error: none recorded"));
	assert(!has_line("Architecture: AArch64"));          /* the summary is not part of the recovery screen */
}

/* A Stage 2 failure wins over a recovery request: rule 1 comes first. */
static void test_stage2_failure_wins_over_request(void)
{
	set_up(JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_MANAGER_SELECTED);
	set_config(true, false, true);
	assert(complete() == END_PANIC);
	assert(context.stage4.decision == JP_BOOT_DECISION_TERMINAL_HALT);
}

/* ---- jp_kernel_stage2_complete: entry checks ---- */

static void test_entry_checks(void)
{
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	assert(setjmp(escape) == 0 ? (jp_kernel_stage2_complete(NULL), 0) : 1);
	assert(halts == 1 && kernel_mains == 0);

	/* A second entry: the machine is stopped, no state changes, Stage 3 is not started. */
	for (JPBootManagerState s = JP_BOOT_MANAGER_COMPLETING; s <= JP_BOOT_MANAGER_DECIDED; s++) {
		set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
		context.stage4.state = s;
		assert(complete() == END_HALT);
		assert(context.stage4.state == s && !context.stage3.log_initialized && kernel_mains == 0 && line_count == 0);
	}
	/* A value that is not a state: also a second entry. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	context.stage4.state = (JPBootManagerState)77;
	assert(complete() == END_HALT && !context.stage3.log_initialized);
}

static void test_stage3_cannot_start(void)
{
	/* An architecture that Stage 3 does not know: nothing can be recorded, so the machine is stopped. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(complete() == END_HALT);
	assert(kernel_mains == 0 && context.stage4.state == JP_BOOT_MANAGER_COMPLETING && line_count == 0);
}

/* ---- jp_boot_manager_decide ---- */

static void add_error(JPBootResult result)
{
	JPBootErrorInput in;

	memset(&in, 0, sizeof in);
	in.original_result = result;
	in.source_component = JP_BOOT_SOURCE_STAGE2;
	in.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
}

static void start_stage3(void)
{
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
}

static JPBootDecision decide_ok(void)
{
	JPBootDecision d = (JPBootDecision)0x55;

	assert(jp_boot_manager_decide(&context, &d) == JP_BOOT_SUCCESS);
	return d;
}

static void test_decide(void)
{
	JPBootDecision d = (JPBootDecision)0x55;

	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	assert(jp_boot_manager_decide(NULL, &d) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_manager_decide(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert((unsigned int)d == 0x55);

	/* Rule 3: nothing wrong, no request. Stage 3 may not even have started. */
	assert(decide_ok() == JP_BOOT_DECISION_CONTINUE_TO_KERNEL);
	start_stage3();
	assert(decide_ok() == JP_BOOT_DECISION_CONTINUE_TO_KERNEL);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_CONTINUE_TO_KERNEL);

	/* Rule 2: recovery requested through the configuration (works without Stage 3, and with an UNSET configuration: no). */
	set_config(true, false, true);
	assert(decide_ok() == JP_BOOT_DECISION_ENTER_RECOVERY);
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_UNINITIALIZED);          /* configuration UNSET: no request can be read */
	assert(decide_ok() == JP_BOOT_DECISION_CONTINUE_TO_KERNEL);
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	set_config(true, false, true);
	assert(decide_ok() == JP_BOOT_DECISION_ENTER_RECOVERY);          /* Stage 3 has not started */
}

static void test_decide_rule_one(void)
{
	JPBootDecision d = (JPBootDecision)0x55;

	/* A Stage 2 failure, and Stage 3 cannot say: its result is returned unchanged, nothing is written. */
	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_SELECTED);
	assert(jp_boot_manager_decide(&context, &d) == JP_BOOT_NOT_INITIALIZED);
	assert((unsigned int)d == 0x55);

	/* A failed attempt with a recoverable error: ENTER_RECOVERY; with a non-recoverable one: TERMINAL_HALT. */
	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_NOT_DETECTED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_ENTER_RECOVERY);
	/* The status RECOVERY is also a failure: the decision of Stage 3 (here: enter; with retries allowed: retry). */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_ENTER_RECOVERY);
	context.stage3.recovery.maximum_retry_count = 1;
	assert(decide_ok() == JP_BOOT_DECISION_RETRY_BOOT);
	context.stage3.recovery.maximum_retry_count = 0;
	/* Halted: terminal. */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_TERMINAL_HALT);

	/* Stage 2 succeeded, but Stage 3 says that the attempt failed, is in recovery, or is halted: rule 1 applies. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_NOT_DETECTED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_ENTER_RECOVERY);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	context.stage3.recovery.maximum_retry_count = 1;
	assert(decide_ok() == JP_BOOT_DECISION_RETRY_BOOT);
	context.stage3.recovery.maximum_retry_count = 0;
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_TERMINAL_HALT);
	/* A non-recoverable error. */
	set_up(JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_TERMINAL_HALT);

	/* A failed status without a recorded error: Stage 3 refuses to decide, its result is returned. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	context.stage3.status.status = JP_BOOT_STATUS_FAILED;
	d = (JPBootDecision)0x55;
	assert(jp_boot_manager_decide(&context, &d) == JP_BOOT_FAILED);
	assert((unsigned int)d == 0x55);

	/* The failure rule is checked first: also with a request for recovery. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	set_config(true, false, true);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	assert(decide_ok() == JP_BOOT_DECISION_TERMINAL_HALT);
}

/* ---- jp_boot_manager_request_recovery ---- */

static JPBootTransitionResult request(JPRecoveryDecision decision)
{
	JPBootRecoveryState state = context.stage3.recovery;

	state.decision = decision;
	return jp_boot_manager_request_recovery(&context, &state);
}

/* A context in which Stage 2 failed and the decision is recorded: what jp_boot_recovery_enter finds. */
static void decided_failure(void)
{
	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	add_error(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	context.stage4.state = JP_BOOT_MANAGER_DECIDED;
	context.stage4.decision = JP_BOOT_DECISION_ENTER_RECOVERY;
}

static void test_request_rejected(void)
{
	JPBootRecoveryState state;

	decided_failure();
	memset(&state, 0, sizeof state);
	assert(jp_boot_manager_request_recovery(NULL, &state) == JP_BOOT_TRANSITION_REJECTED);
	assert(jp_boot_manager_request_recovery(&context, NULL) == JP_BOOT_TRANSITION_REJECTED);
	/* A decision that is not a defined value. */
	assert(request((JPRecoveryDecision)3) == JP_BOOT_TRANSITION_REJECTED);
	assert(request((JPRecoveryDecision)99) == JP_BOOT_TRANSITION_REJECTED);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED && status_now() == JP_BOOT_STATUS_FAILED && !report_is_final());
	/* A manager state other than DECIDED: also for the decisions that would give TERMINAL. */
	for (JPBootManagerState s = JP_BOOT_MANAGER_UNINITIALIZED; s <= JP_BOOT_MANAGER_DECIDED; s++) {
		if (s == JP_BOOT_MANAGER_DECIDED) {
			continue;
		}
		decided_failure();
		context.stage4.state = s;
		assert(request(JP_RECOVERY_ENTER) == JP_BOOT_TRANSITION_REJECTED);
		assert(request(JP_RECOVERY_RETRY) == JP_BOOT_TRANSITION_REJECTED);
		assert(request(JP_RECOVERY_TERMINAL_HALT) == JP_BOOT_TRANSITION_REJECTED);
		assert(context.stage4.state == s && status_now() == JP_BOOT_STATUS_FAILED && !report_is_final() && line_count == 0);
	}
}

static void test_request_terminal(void)
{
	decided_failure();
	assert(request(JP_RECOVERY_RETRY) == JP_BOOT_TRANSITION_TERMINAL);
	assert(request(JP_RECOVERY_TERMINAL_HALT) == JP_BOOT_TRANSITION_TERMINAL);
	/* Nothing was changed. */
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED && status_now() == JP_BOOT_STATUS_FAILED);
	assert(!report_is_final() && context.stage4.config.state == JP_BOOT_CONFIG_STATE_VALIDATED && line_count == 0);
}

static void test_request_enter(void)
{
	/* From FAILED: the status is asked to become RECOVERY, the report is finalized, the configuration is frozen. */
	decided_failure();
	assert(setjmp(escape) == 0 ? (request(JP_RECOVERY_ENTER), 0) : 1);
	assert(halts == 1);
	assert(context.stage4.state == JP_BOOT_MANAGER_RECOVERY_REQUESTED);
	assert(status_now() == JP_BOOT_STATUS_RECOVERY && report_is_final());
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_FROZEN);
	assert(has_line("Status: RECOVERY") && has_line("Error: JP_BOOT_FAILED in STAGE2"));

	/* The request is used up: a second one is rejected. */
	assert(request(JP_RECOVERY_ENTER) == JP_BOOT_TRANSITION_REJECTED);
	assert(request(JP_RECOVERY_TERMINAL_HALT) == JP_BOOT_TRANSITION_REJECTED);

	/* The status was already RECOVERY (what Stage 3 does before it asks). */
	decided_failure();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(setjmp(escape) == 0 ? (request(JP_RECOVERY_ENTER), 0) : 1);
	assert(status_now() == JP_BOOT_STATUS_RECOVERY && report_is_final());

	/* From RUNNING (recovery asked for through the configuration): no error. */
	set_up(JP_BOOT_SUCCESS, JP_BOOT_MANAGER_SELECTED);
	start_stage3();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
	context.stage4.state = JP_BOOT_MANAGER_DECIDED;
	assert(setjmp(escape) == 0 ? (request(JP_RECOVERY_ENTER), 0) : 1);
	assert(status_now() == JP_BOOT_STATUS_RECOVERY && !error_recorded() && report_is_final());
	assert(has_line("Error: none recorded"));

	/* An UNSET configuration stays UNSET. */
	decided_failure();
	memset(&context.stage4.config, 0, sizeof context.stage4.config);
	assert(setjmp(escape) == 0 ? (request(JP_RECOVERY_ENTER), 0) : 1);
	assert(context.stage4.config.state == JP_BOOT_CONFIG_STATE_UNSET);
	assert(report_is_final());
}

static void test_request_enter_refusals(void)
{
	/* Stage 3 has no status: rejected, nothing changes. */
	set_up(JP_BOOT_FAILED, JP_BOOT_MANAGER_DECIDED);
	assert(request(JP_RECOVERY_ENTER) == JP_BOOT_TRANSITION_REJECTED);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED && line_count == 0);

	/* The status cannot become RECOVERY (it is HALTED): rejected. */
	decided_failure();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(request(JP_RECOVERY_ENTER) == JP_BOOT_TRANSITION_REJECTED);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED && !report_is_final() && line_count == 0);

	/* The report cannot be finalized: rejected, the state stays DECIDED, the configuration is not frozen. */
	decided_failure();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	context.stage3.error.boot_attempt_id = 99;       /* the error belongs to another attempt: the records do not agree */
	assert(request(JP_RECOVERY_ENTER) == JP_BOOT_TRANSITION_REJECTED);
	assert(context.stage4.state == JP_BOOT_MANAGER_DECIDED);
	assert(!report_is_final() && context.stage4.config.state == JP_BOOT_CONFIG_STATE_VALIDATED);
	assert(has_line("JustOS boot diagnostics"));     /* the screen was shown before the finalization */
}

/* The failure screen cannot be shown: a WARNING event is recorded, and the request goes on. */
static void test_request_screen_failure(void)
{
	JPBootEvent e;
	size_t before;

	decided_failure();
	output_result = JP_BOOT_FAILED;
	before = event_count();
	assert(setjmp(escape) == 0 ? (request(JP_RECOVERY_ENTER), 0) : 1);
	assert(context.stage4.state == JP_BOOT_MANAGER_RECOVERY_REQUESTED && report_is_final());
	assert(event_count() == before + 1);
	e = event(before);
	assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE4 && e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(e.severity == JP_BOOT_SEVERITY_WARNING && e.result == JP_BOOT_FAILED);
}

int main(void)
{
	test_success();
	test_success_configuration_source_in_event();
	test_success_logging_switches();
	test_success_summary_failure();
	test_failure_while_starting();
	test_failure_while_starting_stage3_lost();
	test_stage2_failure_enters_recovery();
	test_stage2_failure_not_recoverable_halts();
	test_stage2_failure_without_prepare();
	test_stage4_failure();
	test_stage4_failure_config_unset();
	test_stage4_failure_state_uninitialized();
	test_recovery_requested();
	test_stage2_failure_wins_over_request();
	test_entry_checks();
	test_stage3_cannot_start();
	test_decide();
	test_decide_rule_one();
	test_request_rejected();
	test_request_terminal();
	test_request_enter();
	test_request_enter_refusals();
	test_request_screen_failure();
	return 0;
}
