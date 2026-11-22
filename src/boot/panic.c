/*
 * panic.c - the terminal diagnostic state of Stage 3.
 *
 * Purpose:
 *   Shows a failure and stops the processor (milestone0.md, "3 Decided
 *   rules": "Panic, in detail").
 *
 * Responsibilities:
 *   - jp_boot_panic: the first panic keeps its record, adds one FATAL event,
 *     sends the eight lines through diagnostic_output, and stops; a recursive
 *     panic only adds an event and stops;
 *   - the text of the lines is built here without any library.
 *
 * Limitations:
 *   Never returns, never enables interrupts. Writes only the Stage 3 area.
 *   No allocation: the lines are built in a buffer on the stack.
 *
 * Depends on:
 *   justos/boot/panic.h, boot_log.h, diagnostic_output.h, diagnostic_hw.h.
 *
 * Must not depend on:
 *   firmware, heap, Stage 4.
 */

#include "justos/boot/panic.h"

#include "diagnostic_hw.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/diagnostic_output.h"

/* ---- Private constants ---- */

#define JP_PANIC_LINE_MAX 80U
#define JP_PANIC_RECURSIVE_CONTEXT 1U
#define JP_PANIC_DECIMAL_DIGITS_MAX 20U
#define JP_PANIC_HEX_DIGITS 8U
#define JP_PANIC_UNKNOWN_NAME "UNKNOWN"

typedef struct JPPanicLine {
	char text[JP_PANIC_LINE_MAX];
	size_t length;
} JPPanicLine;

/* ---- Private helpers: text ---- */

static void put_string(JPPanicLine *line, const char *text)
{
	while (*text != '\0' && line->length < JP_PANIC_LINE_MAX) {
		line->text[line->length++] = *text++;
	}
}

static void put_decimal(JPPanicLine *line, uint64_t value)
{
	char digits[JP_PANIC_DECIMAL_DIGITS_MAX];
	size_t count = 0;

	do {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	} while (value != 0);
	while (count > 0 && line->length < JP_PANIC_LINE_MAX) {
		line->text[line->length++] = digits[--count];
	}
}

/* "0x" and eight capital hexadecimal digits. */
static void put_hex32(JPPanicLine *line, uint32_t value)
{
	static const char digit[] = "0123456789ABCDEF";
	unsigned int shift;

	put_string(line, "0x");
	for (shift = JP_PANIC_HEX_DIGITS; shift > 0; shift--) {
		if (line->length < JP_PANIC_LINE_MAX) {
			line->text[line->length++] = digit[(value >> ((shift - 1U) * 4U)) & 0xFU];
		}
	}
}

static const char *stage_name(JPDiagnosticStage stage)
{
	static const char *const names[] = { "UNKNOWN", "STAGE1", "STAGE2", "STAGE3", "STAGE4" };

	return (unsigned int)stage <= JP_DIAGNOSTIC_STAGE_STAGE4 ? names[stage] : JP_PANIC_UNKNOWN_NAME;
}

static const char *result_name(JPBootResult result)
{
	static const char *const names[] = {
		"JP_BOOT_SUCCESS", "JP_BOOT_NOT_INITIALIZED", "JP_BOOT_NOT_DETECTED", "JP_BOOT_UNKNOWN",
		"JP_BOOT_FAILED", "JP_BOOT_ERROR_INVALID_PARAM", "JP_BOOT_ERROR_CORRUPTED_DATA"
	};

	return (unsigned int)result <= JP_BOOT_ERROR_CORRUPTED_DATA ? names[result] : JP_PANIC_UNKNOWN_NAME;
}

static const char *severity_name(JPBootSeverity severity)
{
	static const char *const names[] = { "INFO", "WARNING", "ERROR", "FATAL" };

	return (unsigned int)severity <= JP_BOOT_SEVERITY_FATAL ? names[severity] : JP_PANIC_UNKNOWN_NAME;
}

static const char *recoverability_name(JPBootRecoverability recoverability)
{
	static const char *const names[] = { "RECOVERABLE", "NON_RECOVERABLE", "UNKNOWN" };

	return (unsigned int)recoverability <= JP_BOOT_RECOVERABILITY_UNKNOWN ? names[recoverability] : JP_PANIC_UNKNOWN_NAME;
}

static const char *source_name(JPBootSourceComponent source)
{
	static const char *const names[] = {
		"UNKNOWN", "STAGE1", "STAGE2", "BOOT_LOG", "BOOT_STATUS", "ERROR", "DIAGNOSTIC_OUTPUT",
		"BOOT_REPORT", "PANIC", "RECOVERY", "BOOT_CONFIG", "BOOT_INFO", "BOOT_UI", "BOOT_MANAGER"
	};

	return (unsigned int)source <= JP_BOOT_SOURCE_BOOT_MANAGER ? names[source] : JP_PANIC_UNKNOWN_NAME;
}

static void send_line(JPBootContext *context, const JPPanicLine *line)
{
	JPBootDiagnosticMessage message;

	message.stage = JP_DIAGNOSTIC_STAGE_STAGE3;
	message.source_component = JP_BOOT_SOURCE_PANIC;
	message.diagnostic_context = 0;
	message.text = line->text;
	message.length = line->length;
	(void)jp_boot_diagnostic_output(context, &message);     /* a refused line never stops the panic */
}

/* The eight lines of the panic screen. */
static void present(JPBootContext *context, const JPBootError *error)
{
	JPPanicLine line;

	line.length = 0;
	put_string(&line, "JustOS boot failure");
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "attempt: ");
	put_decimal(&line, error->boot_attempt_id);
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "stage: ");
	put_string(&line, stage_name(error->stage));
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "result: ");
	put_string(&line, result_name(error->original_result));
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "severity: ");
	put_string(&line, severity_name(error->severity));
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "recoverability: ");
	put_string(&line, recoverability_name(error->recoverability));
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "source: ");
	put_string(&line, source_name(error->source_component));
	send_line(context, &line);

	line.length = 0;
	put_string(&line, "context: ");
	put_hex32(&line, error->diagnostic_context);
	send_line(context, &line);
}

/* ---- Private helpers: records ---- */

/* One FATAL event for the panic; a refusal is ignored. */
static void add_event(JPBootContext *context, JPBootResult result, uint32_t diagnostic_context)
{
	JPBootEvent event;

	event.boot_attempt_id = 0;
	event.event_sequence = 0;
	event.stage = JP_DIAGNOSTIC_STAGE_STAGE3;
	event.severity = JP_BOOT_SEVERITY_FATAL;
	event.architecture = context->handoff.architecture;
	event.source_component = JP_BOOT_SOURCE_PANIC;
	event.result = result;
	event.diagnostic_context = diagnostic_context;
	(void)jp_boot_log_append(context, &event);
}

/* The error that the panic shows: the argument, else the recorded error, else one that says "unknown". */
static JPBootError error_to_show(const JPBootContext *context, const JPBootError *error)
{
	JPBootError unknown;

	if (error != NULL) {
		return *error;
	}
	if (context->stage3.error_recorded) {
		return context->stage3.error;
	}
	unknown.boot_attempt_id = context->stage3.attempt.boot_attempt_id;
	unknown.original_result = JP_BOOT_UNKNOWN;
	unknown.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
	unknown.severity = JP_BOOT_SEVERITY_FATAL;
	unknown.recoverability = JP_BOOT_RECOVERABILITY_UNKNOWN;
	unknown.architecture = context->handoff.architecture;
	unknown.source_component = JP_BOOT_SOURCE_UNKNOWN;
	unknown.diagnostic_context = 0;
	return unknown;
}

/* ---- Public API ---- */

_Noreturn void jp_boot_panic(JPBootContext *context, const JPBootError *error)
{
	JPBootStage3Area *area;
	JPBootError shown;

	if (context == NULL || !context->stage3.log_initialized) {
		jp_diagnostic_halt();     /* nothing can be recorded or shown */
	}
	area = &context->stage3;

	if (area->panic != JP_BOOT_PANIC_NORMAL) {
		/* A recursive panic: no second presentation, the first record stays. */
		add_event(context, JP_BOOT_FAILED, JP_PANIC_RECURSIVE_CONTEXT);
		area->panic = JP_BOOT_PANIC_TERMINAL;
		jp_diagnostic_halt();
	}

	area->panic = JP_BOOT_PANIC_REQUESTED;
	shown = error_to_show(context, error);
	area->first_panic = shown;
	area->first_panic_recorded = true;
	area->panic = JP_BOOT_PANIC_ACTIVE;
	add_event(context, shown.original_result, shown.diagnostic_context);
	present(context, &shown);
	area->panic = JP_BOOT_PANIC_DIAGNOSTIC_PRESENTED;
	area->panic = JP_BOOT_PANIC_TERMINAL;
	jp_diagnostic_halt();
}
