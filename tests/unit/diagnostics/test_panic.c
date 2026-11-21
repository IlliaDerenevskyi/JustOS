/*
 * Unit test (level U) of src/boot/panic.c. The output function and the halt
 * are fakes (the output records the lines; the halt jumps back into the test);
 * the real boot_log.c takes the events.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "diagnostic_hw.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/panic.h"

/* ---- The fakes ---- */

#define MAX_LINES 32

static char lines[MAX_LINES][128];
static size_t line_count;
static JPBootDiagnosticMessage seen[MAX_LINES];
static JPBootResult output_result;
static unsigned int halts;
static jmp_buf escape;
static JPBootPanicState state_seen_by_output[MAX_LINES];
static JPBootContext *last_context;

JPBootResult jp_boot_diagnostic_output(JPBootContext *context, const JPBootDiagnosticMessage *message)
{
	assert(line_count < MAX_LINES);
	assert(message->length < sizeof lines[0]);
	memcpy(lines[line_count], message->text, message->length);
	lines[line_count][message->length] = '\0';
	seen[line_count] = *message;
	state_seen_by_output[line_count] = context->stage3.panic;
	last_context = context;
	line_count++;
	return output_result;
}

_Noreturn void jp_diagnostic_halt(void)
{
	halts++;
	longjmp(escape, 1);
}

/* ---- Setup ---- */

static JPBootContext context;

static void setup(void)
{
	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	line_count = 0;
	halts = 0;
	output_result = JP_BOOT_SUCCESS;
}

static JPBootError make_error(void)
{
	JPBootError e;

	memset(&e, 0, sizeof e);
	e.boot_attempt_id = 1;
	e.original_result = JP_BOOT_FAILED;
	e.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	e.severity = JP_BOOT_SEVERITY_ERROR;
	e.recoverability = JP_BOOT_RECOVERABILITY_UNKNOWN;
	e.architecture = JP_ARCH_X86_64;
	e.source_component = JP_BOOT_SOURCE_STAGE2;
	e.diagnostic_context = 0x1234;
	return e;
}

/* Runs the panic; returns when the fake halt jumps back. */
static void run_panic(JPBootContext *target, const JPBootError *error)
{
	if (setjmp(escape) == 0) {
		jp_boot_panic(target, error);
		assert(0);                      /* a panic never returns */
	}
}

static size_t events(void)
{
	JPBootLogStatus s;

	assert(jp_boot_log_get_status(&context, &s) == JP_BOOT_SUCCESS);
	return s.event_count;
}

/* ---- The first panic ---- */

static void test_first_panic(void)
{
	JPBootError e = make_error();
	JPBootEvent ev;
	size_t i;

	setup();
	run_panic(&context, &e);
	assert(halts == 1);
	assert(line_count == 8);
	assert(strcmp(lines[0], "JustOS boot failure") == 0);
	assert(strcmp(lines[1], "attempt: 1") == 0);
	assert(strcmp(lines[2], "stage: STAGE2") == 0);
	assert(strcmp(lines[3], "result: JP_BOOT_FAILED") == 0);
	assert(strcmp(lines[4], "severity: ERROR") == 0);
	assert(strcmp(lines[5], "recoverability: UNKNOWN") == 0);
	assert(strcmp(lines[6], "source: STAGE2") == 0);
	assert(strcmp(lines[7], "context: 0x00001234") == 0);
	for (i = 0; i < 8; i++) {
		assert(seen[i].stage == JP_DIAGNOSTIC_STAGE_STAGE3 && seen[i].source_component == JP_BOOT_SOURCE_PANIC);
		assert(seen[i].length == strlen(lines[i]) && seen[i].diagnostic_context == 0);
		assert(state_seen_by_output[i] == JP_BOOT_PANIC_ACTIVE);          /* shown while ACTIVE */
		assert(last_context == &context);
	}
	assert(context.stage3.panic == JP_BOOT_PANIC_TERMINAL);
	assert(context.stage3.first_panic_recorded && memcmp(&context.stage3.first_panic, &e, sizeof e) == 0);
	/* one FATAL event for the panic */
	assert(events() == 2);
	assert(jp_boot_log_get(&context, 1, &ev) == JP_BOOT_SUCCESS);
	assert(ev.severity == JP_BOOT_SEVERITY_FATAL && ev.source_component == JP_BOOT_SOURCE_PANIC);
	assert(ev.stage == JP_DIAGNOSTIC_STAGE_STAGE3 && ev.result == JP_BOOT_FAILED && ev.diagnostic_context == 0x1234);
	assert(ev.architecture == JP_ARCH_X86_64);
}

static void test_names_and_numbers(void)
{
	static const char *const results[] = {
		"JP_BOOT_SUCCESS", "JP_BOOT_NOT_INITIALIZED", "JP_BOOT_NOT_DETECTED", "JP_BOOT_UNKNOWN",
		"JP_BOOT_FAILED", "JP_BOOT_ERROR_INVALID_PARAM", "JP_BOOT_ERROR_CORRUPTED_DATA"
	};
	static const char *const sources[] = {
		"UNKNOWN", "STAGE1", "STAGE2", "BOOT_LOG", "BOOT_STATUS", "ERROR", "DIAGNOSTIC_OUTPUT",
		"BOOT_REPORT", "PANIC", "RECOVERY", "BOOT_CONFIG", "BOOT_INFO", "BOOT_UI", "BOOT_MANAGER"
	};
	static const char *const stages[] = { "UNKNOWN", "STAGE1", "STAGE2", "STAGE3", "STAGE4" };
	static const char *const severities[] = { "INFO", "WARNING", "ERROR", "FATAL" };
	static const char *const recoverabilities[] = { "RECOVERABLE", "NON_RECOVERABLE", "UNKNOWN" };
	JPBootError e;
	size_t i;
	char expected[128];

	for (i = 0; i < 7; i++) {
		setup();
		e = make_error();
		e.original_result = (JPBootResult)i;
		run_panic(&context, &e);
		snprintf(expected, sizeof expected, "result: %s", results[i]);
		assert(strcmp(lines[3], expected) == 0);
	}
	for (i = 0; i < 14; i++) {
		setup();
		e = make_error();
		e.source_component = (JPBootSourceComponent)i;
		run_panic(&context, &e);
		snprintf(expected, sizeof expected, "source: %s", sources[i]);
		assert(strcmp(lines[6], expected) == 0);
	}
	for (i = 0; i < 5; i++) {
		setup();
		e = make_error();
		e.stage = (JPDiagnosticStage)i;
		run_panic(&context, &e);
		snprintf(expected, sizeof expected, "stage: %s", stages[i]);
		assert(strcmp(lines[2], expected) == 0);
	}
	for (i = 0; i < 4; i++) {
		setup();
		e = make_error();
		e.severity = (JPBootSeverity)i;
		run_panic(&context, &e);
		snprintf(expected, sizeof expected, "severity: %s", severities[i]);
		assert(strcmp(lines[4], expected) == 0);
	}
	for (i = 0; i < 3; i++) {
		setup();
		e = make_error();
		e.recoverability = (JPBootRecoverability)i;
		run_panic(&context, &e);
		snprintf(expected, sizeof expected, "recoverability: %s", recoverabilities[i]);
		assert(strcmp(lines[5], expected) == 0);
	}
	/* a value that is not in the vocabulary is UNKNOWN */
	setup();
	e = make_error();
	e.original_result = (JPBootResult)9;
	e.source_component = (JPBootSourceComponent)99;
	e.stage = (JPDiagnosticStage)9;
	e.severity = (JPBootSeverity)9;
	e.recoverability = (JPBootRecoverability)9;
	run_panic(&context, &e);
	assert(strcmp(lines[2], "stage: UNKNOWN") == 0 && strcmp(lines[3], "result: UNKNOWN") == 0);
	assert(strcmp(lines[4], "severity: UNKNOWN") == 0 && strcmp(lines[5], "recoverability: UNKNOWN") == 0);
	assert(strcmp(lines[6], "source: UNKNOWN") == 0);
}

static void test_numbers(void)
{
	JPBootError e;

	setup();
	e = make_error();
	e.boot_attempt_id = 0;
	e.diagnostic_context = 0;
	run_panic(&context, &e);
	assert(strcmp(lines[1], "attempt: 0") == 0 && strcmp(lines[7], "context: 0x00000000") == 0);

	setup();
	e = make_error();
	e.boot_attempt_id = UINT64_MAX;
	e.diagnostic_context = 0xDEADBEEFu;
	run_panic(&context, &e);
	assert(strcmp(lines[1], "attempt: 18446744073709551615") == 0 && strcmp(lines[7], "context: 0xDEADBEEF") == 0);

	setup();
	e = make_error();
	e.boot_attempt_id = 1234567890;
	e.diagnostic_context = 0x0A0B0C0Du;
	run_panic(&context, &e);
	assert(strcmp(lines[1], "attempt: 1234567890") == 0 && strcmp(lines[7], "context: 0x0A0B0C0D") == 0);

	setup();
	e = make_error();
	e.boot_attempt_id = 10;
	run_panic(&context, &e);
	assert(strcmp(lines[1], "attempt: 10") == 0);
}

/* ---- Which error is shown ---- */

static void test_error_choice(void)
{
	JPBootError e = make_error();

	/* NULL: the recorded error */
	setup();
	e.original_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	context.stage3.error = e;
	context.stage3.error_recorded = true;
	run_panic(&context, NULL);
	assert(strcmp(lines[3], "result: JP_BOOT_ERROR_CORRUPTED_DATA") == 0);

	/* NULL and nothing recorded: a panic never invents a cause */
	setup();
	run_panic(&context, NULL);
	assert(strcmp(lines[1], "attempt: 1") == 0);
	assert(strcmp(lines[2], "stage: UNKNOWN") == 0 && strcmp(lines[3], "result: JP_BOOT_UNKNOWN") == 0);
	assert(strcmp(lines[4], "severity: FATAL") == 0 && strcmp(lines[5], "recoverability: UNKNOWN") == 0);
	assert(strcmp(lines[6], "source: UNKNOWN") == 0 && strcmp(lines[7], "context: 0x00000000") == 0);
	assert(context.stage3.first_panic.original_result == JP_BOOT_UNKNOWN);

	/* the argument wins over the recorded error */
	setup();
	context.stage3.error = e;
	context.stage3.error_recorded = true;
	e.original_result = JP_BOOT_NOT_DETECTED;
	run_panic(&context, &e);
	assert(strcmp(lines[3], "result: JP_BOOT_NOT_DETECTED") == 0);
}

/* ---- Recursive panic ---- */

static void test_recursive_panic(void)
{
	JPBootError first = make_error();
	JPBootError second = make_error();
	JPBootPanicState s;
	JPBootEvent ev;
	size_t before;

	second.original_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	for (s = JP_BOOT_PANIC_REQUESTED; s <= JP_BOOT_PANIC_TERMINAL; s++) {
		setup();
		context.stage3.panic = s;
		context.stage3.first_panic = first;
		context.stage3.first_panic_recorded = true;
		before = events();
		run_panic(&context, &second);
		assert(halts == 1 && line_count == 0);                              /* no lines again */
		assert(memcmp(&context.stage3.first_panic, &first, sizeof first) == 0);   /* the first record stays */
		assert(context.stage3.panic == JP_BOOT_PANIC_TERMINAL);
		assert(events() == before + 1);
		assert(jp_boot_log_get(&context, before, &ev) == JP_BOOT_SUCCESS);
		assert(ev.severity == JP_BOOT_SEVERITY_FATAL && ev.source_component == JP_BOOT_SOURCE_PANIC && ev.diagnostic_context == 1);
	}
	/* a panic inside the panic (the output calls panic again) */
	setup();
	run_panic(&context, &first);
	assert(line_count == 8);
	line_count = 0;
	run_panic(&context, &second);
	assert(line_count == 0 && halts == 2);
	assert(context.stage3.first_panic.original_result == JP_BOOT_FAILED);
}

/* ---- Output failures and the log ---- */

static void test_output_failure_does_not_stop_the_panic(void)
{
	JPBootError e = make_error();

	setup();
	output_result = JP_BOOT_FAILED;
	run_panic(&context, &e);
	assert(line_count == 8 && halts == 1 && context.stage3.panic == JP_BOOT_PANIC_TERMINAL);
}

static void test_log_that_takes_no_event(void)
{
	JPBootError e = make_error();

	setup();
	assert(jp_boot_log_finalize(&context) == JP_BOOT_SUCCESS);               /* the log refuses new events */
	run_panic(&context, &e);
	assert(line_count == 8 && halts == 1 && events() == 1);
	setup();
	assert(jp_boot_log_finalize(&context) == JP_BOOT_SUCCESS);
	context.stage3.panic = JP_BOOT_PANIC_ACTIVE;
	run_panic(&context, &e);
	assert(halts == 1 && events() == 1 && context.stage3.panic == JP_BOOT_PANIC_TERMINAL);
}

/* ---- Nothing to record or show ---- */

static void test_no_context(void)
{
	JPBootError e = make_error();

	setup();
	run_panic(NULL, &e);
	assert(halts == 1 && line_count == 0);

	memset(&context, 0, sizeof context);                         /* the log never started */
	line_count = 0;
	halts = 0;
	run_panic(&context, &e);
	assert(halts == 1 && line_count == 0);
	assert(context.stage3.panic == JP_BOOT_PANIC_NORMAL && !context.stage3.first_panic_recorded);
}

static void test_other_state_untouched(void)
{
	static JPBootContext before;
	JPBootError e = make_error();

	setup();
	memcpy(&before, &context, sizeof before);
	run_panic(&context, &e);
	/* only the Stage 3 area changed */
	memcpy(&context.stage3, &before.stage3, sizeof before.stage3);
	assert(memcmp(&before, &context, sizeof before) == 0);
}

int main(void)
{
	test_first_panic();
	test_names_and_numbers();
	test_numbers();
	test_error_choice();
	test_recursive_panic();
	test_output_failure_does_not_stop_the_panic();
	test_log_that_takes_no_event();
	test_no_context();
	test_other_state_untouched();
	return 0;
}
