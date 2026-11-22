/*
 * Unit test (level U) of the text screens of src/boot_ui.c (phase B): the summary and the failure screen. Every line
 * goes through jp_boot_diagnostic_output, which is a fake here that records the lines and can refuse. The real
 * boot_config.c, boot_info.c, boot_status.c, boot_log.c, and error.c keep the records that the screens read.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "justos/boot/boot_config.h"
#include "justos/boot/boot_info.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/boot_ui.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/error.h"

#define MAX_LINES 400U
#define LINE_LENGTH 128U

static JPBootContext context;

/* ---- The fake text output ---- */

static char lines[MAX_LINES][LINE_LENGTH];
static size_t line_count;
static JPBootResult refuse_with;                 /* returned for the calls from refuse_from on */
static size_t refuse_from;
static JPBootResult refuse_later_with;          /* if set, the refusals after the first one return this */
static unsigned int refusals;
static bool refuse_only_one;                    /* refuse just the call number refuse_from + 1 */

JPBootResult jp_boot_diagnostic_output(JPBootContext *ctx, const JPBootDiagnosticMessage *message)
{
	assert(ctx == &context && message != NULL && message->text != NULL);
	assert(message->stage == JP_DIAGNOSTIC_STAGE_STAGE4);
	assert(message->source_component == JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(message->diagnostic_context == 0);
	assert(message->length < LINE_LENGTH && message->length <= 80);
	assert(line_count < MAX_LINES);
	memcpy(lines[line_count], message->text, message->length);
	lines[line_count][message->length] = '\0';
	line_count++;
	if (refuse_with != JP_BOOT_SUCCESS && (refuse_only_one ? line_count == refuse_from + 1U : line_count > refuse_from)) {
		return (refusals++ > 0 && refuse_later_with != JP_BOOT_SUCCESS) ? refuse_later_with : refuse_with;
	}
	return JP_BOOT_SUCCESS;
}

static void set_up(void)
{
	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_X86_64;
	context.current_stage = JP_BOOT_STAGE_READY;        /* so that event 0 of the log says INFO */
	context.handoff.state = JP_HANDOFF_STATE_READY;
	assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
	line_count = 0;
	refuse_with = JP_BOOT_SUCCESS;
	refuse_from = 0;
	refuse_only_one = false;
	refuse_later_with = JP_BOOT_SUCCESS;
	refusals = 0;
}

static void start_stage3(void)
{
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
}

static void set_verbose(bool on)
{
	JPBootConfig c;

	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	c.verbose_output = on;
	assert(jp_boot_config_apply(&context, &c) == JP_BOOT_SUCCESS);
}

static void add_event(JPDiagnosticStage stage, JPBootSeverity severity, JPBootResult result, JPBootSourceComponent source)
{
	JPBootEvent event;

	memset(&event, 0, sizeof event);
	event.stage = stage;
	event.severity = severity;
	event.architecture = JP_ARCH_X86_64;
	event.source_component = source;
	event.result = result;
	assert(jp_boot_log_append(&context, &event) == JP_BOOT_SUCCESS);
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

/* ---- The summary ---- */

static void test_summary_arguments(void)
{
	JPBootSummary summary;

	set_up();
	memset(&summary, 0, sizeof summary);
	assert(jp_boot_ui_present_summary(NULL, &summary) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_ui_present_summary(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(line_count == 0);
}

static void test_summary_every_field(void)
{
	JPBootSummary s;

	set_up();
	memset(&s, 0, sizeof s);
	s.architecture = JP_ARCH_AARCH64;
	s.environment = JP_BOOT_ENVIRONMENT_UEFI;
	s.environment_state = JP_DETECTION_STATE_DETECTED;
	s.hardware_state = JP_DETECTION_STATE_NOT_DETECTED;
	s.memory_map_state = JP_DETECTION_STATE_NOT_INITIALIZED;
	s.framebuffer_state = JP_DETECTION_STATE_FAILED;
	s.kernel_validation_state = JP_KERNEL_VALIDATION_INVALID;
	s.kernel_load_state = JP_KERNEL_LOAD_LOADED;
	s.status_available = true;
	s.status.status = JP_BOOT_STATUS_RUNNING;
	s.config_state = JP_BOOT_CONFIG_STATE_FROZEN;
	s.config_source = JP_BOOT_CONFIG_SOURCE_INTERACTIVE;
	s.selection_outcome = JP_BOOT_SELECTION_APPLIED;

	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(line_count == 11);                    /* verbose output is off: no events */
	assert(strcmp(lines[0], "Architecture: AArch64") == 0);
	assert(strcmp(lines[1], "Environment: UEFI") == 0);
	assert(strcmp(lines[2], "Environment state: DETECTED") == 0);
	assert(strcmp(lines[3], "Hardware: NOT DETECTED") == 0);
	assert(strcmp(lines[4], "Memory map: NOT INITIALIZED") == 0);
	assert(strcmp(lines[5], "Framebuffer: FAILED") == 0);
	assert(strcmp(lines[6], "Kernel validation: INVALID") == 0);
	assert(strcmp(lines[7], "Kernel load: LOADED") == 0);
	assert(strcmp(lines[8], "Boot status: RUNNING") == 0);
	assert(strcmp(lines[9], "Configuration: FROZEN (INTERACTIVE)") == 0);
	assert(strcmp(lines[10], "Menu: APPLIED") == 0);

	set_up();
	s.environment = JP_BOOT_ENVIRONMENT_BIOS;
	s.architecture = JP_ARCH_X86_64;
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(has_line("Environment: BIOS") && has_line("Architecture: x86_64"));
}

/* A state that is not available is shown exactly as reported; an unavailable status says NOT INITIALIZED. */
static void test_summary_unavailable(void)
{
	JPBootSummary s;

	set_up();
	memset(&s, 0, sizeof s);
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(strcmp(lines[0], "Architecture: UNKNOWN") == 0);
	assert(strcmp(lines[1], "Environment: UNKNOWN") == 0);
	assert(strcmp(lines[3], "Hardware: UNKNOWN") == 0);
	assert(strcmp(lines[8], "Boot status: NOT INITIALIZED") == 0);
	assert(strcmp(lines[9], "Configuration: UNSET (UNKNOWN)") == 0);
	assert(strcmp(lines[10], "Menu: NOT_RUN") == 0);
}

static void test_summary_names(void)
{
	static const char *const validation[] = { "UNKNOWN", "NOT INITIALIZED", "VALID", "INVALID", "FAILED" };
	static const char *const load[] = { "UNKNOWN", "NOT INITIALIZED", "LOADED", "FAILED" };
	static const char *const status[] = { "BOOTING", "INITIALIZING", "RUNNING", "FAILED", "RECOVERY", "HALTED" };
	static const char *const menu[] = {
		"NOT_RUN", "SKIPPED_POLICY", "SKIPPED_NO_FRAMEBUFFER", "SKIPPED_NO_INPUT", "APPLIED", "DISCARDED"
	};
	JPBootSummary s;
	char text[64];

	memset(&s, 0, sizeof s);
	for (int v = 0; v <= JP_KERNEL_VALIDATION_FAILED; v++) {
		set_up();
		s.kernel_validation_state = (JPKernelValidationState)v;
		assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
		strcpy(text, "Kernel validation: ");
		assert(has_line(strcat(text, validation[v])));
	}
	for (int v = 0; v <= JP_KERNEL_LOAD_FAILED; v++) {
		set_up();
		s.kernel_load_state = (JPKernelLoadState)v;
		assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
		strcpy(text, "Kernel load: ");
		assert(has_line(strcat(text, load[v])));
	}
	s.status_available = true;
	for (int v = 0; v <= JP_BOOT_STATUS_HALTED; v++) {
		set_up();
		s.status.status = (JPBootStatus)v;
		assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
		strcpy(text, "Boot status: ");
		assert(has_line(strcat(text, status[v])));
	}
	for (int v = 0; v <= JP_BOOT_SELECTION_DISCARDED; v++) {
		set_up();
		s.selection_outcome = (JPBootSelectionOutcome)v;
		assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
		strcpy(text, "Menu: ");
		assert(has_line(strcat(text, menu[v])));
	}
	set_up();
	s.architecture = (JPArchitectureType)9;
	s.environment = (JPBootEnvironmentType)9;
	s.kernel_validation_state = (JPKernelValidationState)9;
	s.kernel_load_state = (JPKernelLoadState)9;
	s.status.status = (JPBootStatus)9;
	s.config_state = (JPBootConfigState)9;
	s.config_source = (JPBootConfigSource)9;
	s.selection_outcome = (JPBootSelectionOutcome)9;
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(has_line("Architecture: UNKNOWN") && has_line("Environment: UNKNOWN"));
	assert(has_line("Kernel validation: UNKNOWN") && has_line("Kernel load: UNKNOWN"));
	assert(has_line("Boot status: UNKNOWN") && has_line("Configuration: UNKNOWN (UNKNOWN)") && has_line("Menu: UNKNOWN"));
}

static void test_summary_events_when_verbose(void)
{
	JPBootSummary s;

	set_up();
	start_stage3();
	add_event(JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS, JP_BOOT_SOURCE_STAGE2);
	add_event(JP_DIAGNOSTIC_STAGE_STAGE4, JP_BOOT_SEVERITY_WARNING, JP_BOOT_FAILED, JP_BOOT_SOURCE_BOOT_MANAGER);
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(line_count == 11);                    /* not verbose: no event lines */

	set_verbose(true);
	line_count = 0;
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(line_count == 14);                    /* event 0 is the one that the log writes itself (Stage 1) */
	assert(strcmp(lines[11], "Event 0: STAGE1 INFO JP_BOOT_SUCCESS STAGE1") == 0);
	assert(strcmp(lines[12], "Event 1: STAGE2 INFO JP_BOOT_SUCCESS STAGE2") == 0);
	assert(strcmp(lines[13], "Event 2: STAGE4 WARNING JP_BOOT_FAILED BOOT_MANAGER") == 0);
}

static void test_summary_verbose_without_log(void)
{
	JPBootSummary s;

	set_up();
	set_verbose(true);
	memset(&s, 0, sizeof s);
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(line_count == 12 && strcmp(lines[11], "Events: NOT INITIALIZED") == 0);
}

/* ---- The failure screen ---- */

static void test_failure_arguments(void)
{
	set_up();
	assert(jp_boot_ui_present_failure(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(line_count == 0);
}

static void test_failure_nothing_started(void)
{
	set_up();
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(line_count == 4);
	assert(strcmp(lines[0], "JustOS boot diagnostics") == 0);
	assert(strcmp(lines[1], "Status: NOT INITIALIZED") == 0);
	assert(strcmp(lines[2], "Error: none recorded") == 0);
	assert(strcmp(lines[3], "Events: NOT INITIALIZED") == 0);
}

static void test_failure_with_error(void)
{
	JPBootErrorInput input;

	set_up();
	start_stage3();
	add_event(JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_SEVERITY_ERROR, JP_BOOT_FAILED, JP_BOOT_SOURCE_STAGE2);
	memset(&input, 0, sizeof input);
	input.original_result = JP_BOOT_NOT_DETECTED;
	input.source_component = JP_BOOT_SOURCE_STAGE2;
	input.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	assert(jp_boot_error_create(&context, &input) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);

	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(strcmp(lines[0], "JustOS boot diagnostics") == 0);
	assert(strcmp(lines[1], "Status: FAILED") == 0);
	assert(strcmp(lines[2], "Error: JP_BOOT_NOT_DETECTED in STAGE2") == 0);
	assert(strcmp(lines[3], "Severity: ERROR, RECOVERABLE, source STAGE2") == 0);
	assert(strcmp(lines[4], "Events: 2") == 0);
	assert(line_count == 5);                     /* not verbose */

	set_verbose(true);
	line_count = 0;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(line_count == 7 && strcmp(lines[5], "Event 0: STAGE1 INFO JP_BOOT_SUCCESS STAGE1") == 0);
	assert(strcmp(lines[6], "Event 1: STAGE2 ERROR JP_BOOT_FAILED STAGE2") == 0);
}

static void test_failure_names(void)
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
	char text[96];

	/* The event lines name every value of the four tables, down to the last one of each. */
	set_up();
	start_stage3();
	set_verbose(true);
	for (int r = 0; r <= JP_BOOT_ERROR_CORRUPTED_DATA; r++) {
		add_event(JP_DIAGNOSTIC_STAGE_STAGE4, JP_BOOT_SEVERITY_FATAL, (JPBootResult)r, JP_BOOT_SOURCE_BOOT_MANAGER);
	}
	for (int v = 0; v <= JP_BOOT_SOURCE_BOOT_MANAGER; v++) {
		add_event((JPDiagnosticStage)(v % 5), (JPBootSeverity)(v % 4), JP_BOOT_FAILED, (JPBootSourceComponent)v);
	}
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	for (int r = 0; r <= JP_BOOT_ERROR_CORRUPTED_DATA; r++) {
		snprintf(text, sizeof text, "Event %d: STAGE4 FATAL %s BOOT_MANAGER", r + 1, results[r]);
		assert(has_line(text));
	}
	for (int v = 0; v <= JP_BOOT_SOURCE_BOOT_MANAGER; v++) {
		snprintf(text, sizeof text, "Event %d: %s %s JP_BOOT_FAILED %s", 8 + v, stages[v % 5], severities[v % 4], sources[v]);
		assert(has_line(text));
	}
	/* Values outside the tables say UNKNOWN. */
	set_up();
	start_stage3();
	set_verbose(true);
	{
		JPBootEvent event;

		memset(&event, 0, sizeof event);
		event.stage = JP_DIAGNOSTIC_STAGE_STAGE1;
		event.severity = JP_BOOT_SEVERITY_INFO;
		event.architecture = JP_ARCH_X86_64;
		event.result = JP_BOOT_SUCCESS;
		event.source_component = JP_BOOT_SOURCE_STAGE1;
		assert(jp_boot_log_append(&context, &event) == JP_BOOT_SUCCESS);
		context.stage3.log.events[1].stage = (JPDiagnosticStage)9;
		context.stage3.log.events[1].severity = (JPBootSeverity)9;
		context.stage3.log.events[1].result = (JPBootResult)99;
		context.stage3.log.events[1].source_component = (JPBootSourceComponent)99;
	}
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(has_line("Event 1: UNKNOWN UNKNOWN UNKNOWN UNKNOWN"));
	/* Recoverability names of an error. */
	for (int v = 0; v <= JP_BOOT_RECOVERABILITY_UNKNOWN; v++) {
		static const char *const rec[] = { "RECOVERABLE", "NON_RECOVERABLE", "UNKNOWN" };

		set_up();
		start_stage3();
		context.stage3.error_recorded = true;
		context.stage3.error.original_result = JP_BOOT_FAILED;
		context.stage3.error.severity = (JPBootSeverity)(v + 1);
		context.stage3.error.recoverability = (JPBootRecoverability)v;
		context.stage3.error.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
		context.stage3.error.stage = JP_DIAGNOSTIC_STAGE_STAGE3;
		assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
		snprintf(text, sizeof text, "Severity: %s, %s, source BOOT_MANAGER", severities[v + 1], rec[v]);
		assert(has_line(text));
		assert(has_line("Error: JP_BOOT_FAILED in STAGE3"));
	}
	context.stage3.error.recoverability = (JPBootRecoverability)9;
	context.stage3.error.severity = (JPBootSeverity)9;
	context.stage3.error.source_component = (JPBootSourceComponent)99;
	context.stage3.error.stage = (JPDiagnosticStage)9;
	context.stage3.error.original_result = (JPBootResult)99;
	line_count = 0;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(has_line("Error: UNKNOWN in UNKNOWN") && has_line("Severity: UNKNOWN, UNKNOWN, source UNKNOWN"));
	/* A status that Stage 3 reported is named. */
	for (int v = 0; v <= JP_BOOT_STATUS_HALTED; v++) {
		static const char *const status[] = { "BOOTING", "INITIALIZING", "RUNNING", "FAILED", "RECOVERY", "HALTED" };

		set_up();
		start_stage3();
		context.stage3.status.status = (JPBootStatus)v;
		assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
		strcpy(text, "Status: ");
		assert(has_line(strcat(text, status[v])));
	}
	context.stage3.status.status = (JPBootStatus)9;
	line_count = 0;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(has_line("Status: UNKNOWN"));
}

/* The count of events is a decimal number, also with several digits. */
static void test_failure_event_count_digits(void)
{
	set_up();
	start_stage3();
	for (int i = 0; i < 123; i++) {
		add_event(JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS, JP_BOOT_SOURCE_STAGE2);
	}
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(has_line("Events: 124"));                 /* 123 added, and event 0 of the log */
	set_verbose(true);
	line_count = 0;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(has_line("Event 1: STAGE2 INFO JP_BOOT_SUCCESS STAGE2") && has_line("Event 123: STAGE2 INFO JP_BOOT_SUCCESS STAGE2"));
	assert(line_count == 4 + 124);
}

/* ---- Failures of the text output ---- */

static void test_output_failure(void)
{
	JPBootSummary s;

	set_up();
	memset(&s, 0, sizeof s);
	refuse_with = JP_BOOT_FAILED;
	refuse_later_with = JP_BOOT_NOT_DETECTED;     /* later refusals say something else: the first one is returned */
	refuse_from = 3;                             /* the fourth line and every later one are refused */
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_FAILED);
	assert(line_count == 11);                    /* every line was tried */

	set_up();
	refuse_with = JP_BOOT_NOT_INITIALIZED;
	refuse_from = 1;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(line_count == 4);

	/* The first failure is the one that is returned, not a later one. */
	set_up();
	refuse_with = JP_BOOT_NOT_DETECTED;
	refuse_from = 0;
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_NOT_DETECTED);

	/* One refused line in the middle: the failure stays, later lines that are accepted do not wipe it. */
	set_up();
	memset(&s, 0, sizeof s);
	refuse_with = JP_BOOT_FAILED;
	refuse_from = 2;
	refuse_only_one = true;
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_FAILED);
	assert(line_count == 11);

	/* A failure in the event lines is returned as well. */
	set_up();
	start_stage3();
	add_event(JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS, JP_BOOT_SOURCE_STAGE2);
	set_verbose(true);
	line_count = 0;
	refuse_with = JP_BOOT_FAILED;
	refuse_from = 5;                             /* only the event line is refused */
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_FAILED);
}

/* The screens read the records and change nothing. */
static void test_context_unchanged(void)
{
	static JPBootContext before;
	JPBootSummary s;

	set_up();
	start_stage3();
	add_event(JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_SEVERITY_INFO, JP_BOOT_SUCCESS, JP_BOOT_SOURCE_STAGE2);
	set_verbose(true);
	assert(jp_boot_info_summarize(&context, &s) == JP_BOOT_SUCCESS);
	memcpy(&before, &context, sizeof context);
	assert(jp_boot_ui_present_summary(&context, &s) == JP_BOOT_SUCCESS);
	assert(jp_boot_ui_present_failure(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &context, sizeof context) == 0);
}

int main(void)
{
	test_summary_arguments();
	test_summary_every_field();
	test_summary_unavailable();
	test_summary_names();
	test_summary_events_when_verbose();
	test_summary_verbose_without_log();
	test_failure_arguments();
	test_failure_nothing_started();
	test_failure_with_error();
	test_failure_names();
	test_failure_event_count_digits();
	test_output_failure();
	test_context_unchanged();
	return 0;
}
