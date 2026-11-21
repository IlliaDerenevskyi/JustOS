/*
 * fake_stage3_lines.c - the lines that the Stage 4 stand-ins send about Stage 3. Test code only.
 */

#include <stddef.h>

#include "fake_stage3_lines.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/error.h"

#define LINE_MAX 96

typedef struct Line {
	char text[LINE_MAX];
	size_t length;
} Line;

static void put(Line *line, const char *text)
{
	while (*text != '\0' && line->length < LINE_MAX) {
		line->text[line->length++] = *text++;
	}
}

static void put_number(Line *line, uint64_t value)
{
	char digits[20];
	size_t count = 0;

	do {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	} while (value != 0);
	while (count > 0 && line->length < LINE_MAX) {
		line->text[line->length++] = digits[--count];
	}
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

static void send(JPBootContext *context, const Line *line)
{
	JPBootDiagnosticMessage message;

	message.stage = JP_DIAGNOSTIC_STAGE_STAGE3;
	message.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
	message.diagnostic_context = 0;
	message.text = line->text;
	message.length = line->length;
	(void)jp_boot_diagnostic_output(context, &message);
}

void jp_test_stage3_line(JPBootContext *context, const char *text)
{
	Line line;

	line.length = 0;
	put(&line, text);
	send(context, &line);
}

void jp_test_stage3_report(JPBootContext *context)
{
	JPBootStatusState status;
	JPBootLogStatus log;
	JPBootError error;
	JPBootReport report;
	Line line;

	if (jp_boot_status_get(context, &status) == JP_BOOT_SUCCESS) {
		line.length = 0;
		put(&line, "JP|stage3.status=");
		put(&line, status_name(status.status));
		send(context, &line);
	}
	line.length = 0;
	put(&line, "JP|stage3.attempt=");
	put_number(&line, context->stage3.attempt.boot_attempt_id);
	send(context, &line);
	if (jp_boot_log_get_status(context, &log) == JP_BOOT_SUCCESS) {
		line.length = 0;
		put(&line, "JP|stage3.events=");
		put_number(&line, log.event_count);
		send(context, &line);
	}
	line.length = 0;
	put(&line, "JP|stage3.error=");
	put(&line, jp_boot_error_get(context, &error) == JP_BOOT_SUCCESS ? result_name(error.original_result) : "NONE");
	send(context, &line);
	line.length = 0;
	put(&line, "JP|stage3.report=");
	put(&line, jp_boot_report_get(context, &report) == JP_BOOT_SUCCESS && report.finalized ? "FINAL" : "NOT_FINAL");
	send(context, &line);
}
