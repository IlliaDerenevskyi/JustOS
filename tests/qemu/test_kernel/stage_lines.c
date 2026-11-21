/*
 * stage_lines.c - the lines that the test kernel writes about Stage 3 and Stage 4. Test code only.
 */

#include "stage_lines.h"

#include "justos/boot/boot_config.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/error.h"

#define DECIMAL_DIGITS_MAX 20U

static void put_string(const JPTestSink *sink, const char *text)
{
	while (*text != '\0') {
		sink->put_char(sink->user, *text++);
	}
}

static void put_decimal(const JPTestSink *sink, uint64_t value)
{
	char digits[DECIMAL_DIGITS_MAX];
	size_t count = 0;

	do {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	} while (value != 0);
	while (count > 0) {
		sink->put_char(sink->user, digits[--count]);
	}
}

static void line(const JPTestSink *sink, const char *key, const char *value)
{
	put_string(sink, "JP|");
	put_string(sink, key);
	sink->put_char(sink->user, '=');
	put_string(sink, value);
	sink->put_char(sink->user, '\n');
}

static void line_number(const JPTestSink *sink, const char *key, uint64_t value)
{
	put_string(sink, "JP|");
	put_string(sink, key);
	sink->put_char(sink->user, '=');
	put_decimal(sink, value);
	sink->put_char(sink->user, '\n');
}

static const char *yes_no(bool value)
{
	return value ? "yes" : "no";
}

static const char *status_name(JPBootStatus status)
{
	static const char *const names[] = { "BOOTING", "INITIALIZING", "RUNNING", "FAILED", "RECOVERY", "HALTED" };

	return (unsigned int)status <= JP_BOOT_STATUS_HALTED ? names[status] : "UNKNOWN";
}

static const char *result_name(JPBootResult result)
{
	static const char *const names[] = {
		"SUCCESS", "NOT_INITIALIZED", "NOT_DETECTED", "UNKNOWN", "FAILED", "ERROR_INVALID_PARAM", "ERROR_CORRUPTED_DATA"
	};

	return (unsigned int)result <= JP_BOOT_ERROR_CORRUPTED_DATA ? names[result] : "UNKNOWN";
}

static const char *manager_state_name(JPBootManagerState state)
{
	static const char *const names[] = { "UNINITIALIZED", "PREPARED", "SELECTED", "COMPLETING", "RECOVERY_REQUESTED", "DECIDED" };

	return (unsigned int)state <= JP_BOOT_MANAGER_DECIDED ? names[state] : "UNKNOWN";
}

static const char *decision_name(JPBootDecision decision)
{
	static const char *const names[] = { "NONE", "CONTINUE_TO_KERNEL", "RETRY_BOOT", "ENTER_RECOVERY", "TERMINAL_HALT" };

	return (unsigned int)decision <= JP_BOOT_DECISION_TERMINAL_HALT ? names[decision] : "UNKNOWN";
}

static const char *outcome_name(JPBootSelectionOutcome outcome)
{
	static const char *const names[] = {
		"NOT_RUN", "SKIPPED_POLICY", "SKIPPED_NO_FRAMEBUFFER", "SKIPPED_NO_INPUT", "APPLIED", "DISCARDED"
	};

	return (unsigned int)outcome <= JP_BOOT_SELECTION_DISCARDED ? names[outcome] : "UNKNOWN";
}

static void emit_stage3(const JPBootContext *context, const JPTestSink *sink)
{
	JPBootStatusState status;
	JPBootLogStatus log;
	JPBootError error;
	JPBootReport report;

	line(sink, "stage3.status", jp_boot_status_get(context, &status) == JP_BOOT_SUCCESS ? status_name(status.status) : "NOT_INITIALIZED");
	line_number(sink, "stage3.attempt", context->stage3.attempt.boot_attempt_id);
	if (jp_boot_log_get_status(context, &log) == JP_BOOT_SUCCESS) {
		line_number(sink, "stage3.events", log.event_count);
	} else {
		line(sink, "stage3.events", "NOT_INITIALIZED");
	}
	line(sink, "stage3.error", jp_boot_error_get(context, &error) == JP_BOOT_SUCCESS ? result_name(error.original_result) : "NONE");
	line(sink, "stage3.report", jp_boot_report_get(context, &report) == JP_BOOT_SUCCESS && report.finalized ? "FINAL" : "NOT_FINAL");
}

static void emit_stage4(const JPBootContext *context, const JPTestSink *sink)
{
	JPBootConfig config;

	line(sink, "stage4.manager", manager_state_name(context->stage4.state));
	line(sink, "stage4.decision", decision_name(context->stage4.decision));
	line(sink, "stage4.selection", outcome_name(context->stage4.selection_outcome));
	if (jp_boot_config_get(context, &config) != JP_BOOT_SUCCESS) {
		line(sink, "stage4.config.state", "UNSET");
		return;
	}
	line(sink, "stage4.config.state", config.state == JP_BOOT_CONFIG_STATE_FROZEN ? "FROZEN" : "VALIDATED");
	line(sink, "stage4.config.source", config.source == JP_BOOT_CONFIG_SOURCE_INTERACTIVE ? "INTERACTIVE" : "DEFAULT");
	line(sink, "stage4.config.kernel_parameters", config.kernel_parameters);
	line(sink, "stage4.config.boot_logging", yes_no(config.boot_logging));
	line(sink, "stage4.config.verbose_output", yes_no(config.verbose_output));
	line(sink, "stage4.config.safe_mode", yes_no(config.safe_mode));
	line(sink, "stage4.config.recovery_requested", yes_no(config.recovery_requested));
	line_number(sink, "stage4.config.entry_count", config.entry_count);
}

void jp_test_stage_lines_emit(const JPBootContext *context, const JPTestSink *sink)
{
	emit_stage3(context, sink);
	emit_stage4(context, sink);
}
