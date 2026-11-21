/*
 * boot_ui.c - the boot screens (Stage 4).
 *
 * Purpose:
 *   Implements the Boot UI Contract (milestone0.md, "4 Boot UI Contract",
 *   "4 Decided rules", and file specifications 5 and 6).
 *
 * Responsibilities:
 *   Phase A (the boot program, JP_BOOT_STAGE4_PHASE_A): the menu. It draws
 *   the whole screen into the checked framebuffer, reads one key at a time
 *   through jp_boot_environment_read_input, and ends with F10 (apply) or Esc
 *   (discard).
 *   Phase B (the kernel image, JP_BOOT_STAGE4_PHASE_B): the summary and the
 *   failure screen, as lines of text through jp_boot_diagnostic_output.
 *
 * Limitations:
 *   The text sits on a grid of 8x16 pixel cells, white on black, at most 80
 *   columns; everything outside the framebuffer is clipped. The menu redraws
 *   after a key (and at the start), not on every poll: what can be seen is the
 *   same. No heap, no timer, no global data that changes; the font is a
 *   read-only table.
 *
 * Depends on:
 *   justos/boot/boot_ui.h, boot_config.h, boot_info.h, boot_manager.h,
 *   boot_environment.h and boot_ui_font.h (phase A), diagnostic_output.h,
 *   boot_log.h, boot_status.h, error.h (phase B).
 *
 * Must not depend on:
 *   firmware except the keyboard input that Stage 1 provides, Stage 3 code
 *   other than the text output and the readers.
 */

#include "justos/boot/boot_ui.h"

#if defined(JP_BOOT_STAGE4_PHASE_A) == defined(JP_BOOT_STAGE4_PHASE_B)
#error "define exactly one of JP_BOOT_STAGE4_PHASE_A and JP_BOOT_STAGE4_PHASE_B"
#endif

#include "justos/boot/boot_config.h"
#include "justos/boot/boot_manager.h"

#if defined(JP_BOOT_STAGE4_PHASE_A)
#include "boot_ui_font.h"
#include "justos/boot/boot_environment.h"
#else
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/error.h"
#endif

/* ---- Private constants ---- */

#define JP_UI_LINE_MAX 80U                 /* the most characters of one line */
#define JP_UI_DECIMAL_DIGITS_MAX 20U
#define JP_UI_UNKNOWN_NAME "UNKNOWN"
#define JP_UI_NOT_INITIALIZED_NAME "NOT INITIALIZED"
#define JP_UI_SUMMARY_LINE_COUNT 11U

typedef struct JPUiLine {
	char text[JP_UI_LINE_MAX];
	size_t length;
} JPUiLine;

/* ---- Private helpers: text (both phases) ---- */

static void put(JPUiLine *line, const char *text)
{
	while (*text != '\0' && line->length < JP_UI_LINE_MAX) {
		line->text[line->length++] = *text++;
	}
}

#if defined(JP_BOOT_STAGE4_PHASE_B)
static void put_decimal(JPUiLine *line, uint64_t value)
{
	char digits[JP_UI_DECIMAL_DIGITS_MAX];
	size_t count = 0;

	do {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	} while (value != 0);
	while (count > 0 && line->length < JP_UI_LINE_MAX) {
		line->text[line->length++] = digits[--count];
	}
}
#endif

static const char *architecture_name(JPArchitectureType architecture)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, "x86_64", "AArch64" };

	return (unsigned int)architecture <= JP_ARCH_AARCH64 ? names[architecture] : JP_UI_UNKNOWN_NAME;
}

static const char *environment_name(JPBootEnvironmentType type)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, "UEFI", "BIOS" };

	return (unsigned int)type <= JP_BOOT_ENVIRONMENT_BIOS ? names[type] : JP_UI_UNKNOWN_NAME;
}

static const char *validation_name(JPKernelValidationState state)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, JP_UI_NOT_INITIALIZED_NAME, "VALID", "INVALID", "FAILED" };

	return (unsigned int)state <= JP_KERNEL_VALIDATION_FAILED ? names[state] : JP_UI_UNKNOWN_NAME;
}

static const char *load_name(JPKernelLoadState state)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, JP_UI_NOT_INITIALIZED_NAME, "LOADED", "FAILED" };

	return (unsigned int)state <= JP_KERNEL_LOAD_FAILED ? names[state] : JP_UI_UNKNOWN_NAME;
}

static const char *status_name(JPBootStatus status)
{
	static const char *const names[] = { "BOOTING", "INITIALIZING", "RUNNING", "FAILED", "RECOVERY", "HALTED" };

	return (unsigned int)status <= JP_BOOT_STATUS_HALTED ? names[status] : JP_UI_UNKNOWN_NAME;
}

static const char *config_state_name(JPBootConfigState state)
{
	static const char *const names[] = { "UNSET", "VALIDATED", "FROZEN" };

	return (unsigned int)state <= JP_BOOT_CONFIG_STATE_FROZEN ? names[state] : JP_UI_UNKNOWN_NAME;
}

static const char *config_source_name(JPBootConfigSource source)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, "DEFAULT", "INTERACTIVE" };

	return (unsigned int)source <= JP_BOOT_CONFIG_SOURCE_INTERACTIVE ? names[source] : JP_UI_UNKNOWN_NAME;
}

static const char *outcome_name(JPBootSelectionOutcome outcome)
{
	static const char *const names[] = {
		"NOT_RUN", "SKIPPED_POLICY", "SKIPPED_NO_FRAMEBUFFER", "SKIPPED_NO_INPUT", "APPLIED", "DISCARDED"
	};

	return (unsigned int)outcome <= JP_BOOT_SELECTION_DISCARDED ? names[outcome] : JP_UI_UNKNOWN_NAME;
}

/*
 * Line number index of the summary (0 to JP_UI_SUMMARY_LINE_COUNT - 1). Every value is the one that its owner
 * reported; an unavailable boot status is shown as NOT INITIALIZED, which is what the status owner said.
 */
static void summary_line(const JPBootSummary *summary, size_t index, JPUiLine *line)
{
	line->length = 0;
	switch (index) {
	case 0: put(line, "Architecture: "); put(line, architecture_name(summary->architecture)); break;
	case 1: put(line, "Environment: "); put(line, environment_name(summary->environment)); break;
	case 2: put(line, "Environment state: "); put(line, jp_boot_info_state_name(summary->environment_state)); break;
	case 3: put(line, "Hardware: "); put(line, jp_boot_info_state_name(summary->hardware_state)); break;
	case 4: put(line, "Memory map: "); put(line, jp_boot_info_state_name(summary->memory_map_state)); break;
	case 5: put(line, "Framebuffer: "); put(line, jp_boot_info_state_name(summary->framebuffer_state)); break;
	case 6: put(line, "Kernel validation: "); put(line, validation_name(summary->kernel_validation_state)); break;
	case 7: put(line, "Kernel load: "); put(line, load_name(summary->kernel_load_state)); break;
	case 8:
		put(line, "Boot status: ");
		put(line, summary->status_available ? status_name(summary->status.status) : JP_UI_NOT_INITIALIZED_NAME);
		break;
	case 9:
		put(line, "Configuration: ");
		put(line, config_state_name(summary->config_state));
		put(line, " (");
		put(line, config_source_name(summary->config_source));
		put(line, ")");
		break;
	default: put(line, "Menu: "); put(line, outcome_name(summary->selection_outcome)); break;
	}
}

#if defined(JP_BOOT_STAGE4_PHASE_A)

/* ================= Phase A: the menu ================= */

#define JP_UI_COLOR_WHITE 0x00FFFFFFU
#define JP_UI_COLOR_BLACK 0x00000000U
#define JP_UI_PIXEL_BYTES 4U
#define JP_UI_FIRST_PRINTABLE 0x20U
#define JP_UI_LAST_PRINTABLE 0x7EU

/* Rows (text lines) of the screen. */
#define JP_UI_ROW_TITLE 0U
#define JP_UI_ROW_FIELDS 2U
#define JP_UI_ROW_INFO_TITLE 8U
#define JP_UI_ROW_INFO 9U
#define JP_UI_ROW_MESSAGE 21U
#define JP_UI_ROW_HELP 22U

typedef enum JPUiField {
	JP_UI_FIELD_KERNEL_PARAMETERS = 0,
	JP_UI_FIELD_BOOT_LOGGING,
	JP_UI_FIELD_VERBOSE_OUTPUT,
	JP_UI_FIELD_SAFE_MODE,
	JP_UI_FIELD_RECOVERY_REQUESTED,
	JP_UI_FIELD_COUNT
} JPUiField;

/* What exists only during one call of jp_boot_ui_select. */
typedef struct JPUiState {
	size_t field;                 /* the current field */
	bool editing;                 /* the kernel parameters are being edited */
	bool has_message;             /* a rejection text is shown */
	JPBootConfig candidate;       /* what the user edits; the active configuration is untouched until F10 */
} JPUiState;

static const char *const jp_ui_title = "JustOS boot menu";
static const char *const jp_ui_help = "Up/Down: field  Left/Right/Enter: change  F10: apply  Esc: discard";
static const char *const jp_ui_rejection = "The values are not valid. Nothing was changed.";

static bool framebuffer_is_usable(const JPFramebufferInfo *fb)
{
	return fb->state == JP_DETECTION_STATE_DETECTED &&
	       fb->base_address != 0 &&
	       fb->width != 0 && fb->height != 0 && fb->pitch != 0 &&
	       (fb->format == JP_FRAMEBUFFER_FORMAT_RGB_888 || fb->format == JP_FRAMEBUFFER_FORMAT_BGR_888) &&
	       (uint64_t)fb->pitch >= (uint64_t)fb->width * JP_UI_PIXEL_BYTES &&
	       fb->pitch % JP_UI_PIXEL_BYTES == 0;
}

static void put_pixel(const JPFramebufferInfo *fb, uint32_t x, uint32_t y, uint32_t color)
{
	uint8_t *base = (uint8_t *)(uintptr_t)fb->base_address;
	uint32_t *pixel = (uint32_t *)(void *)(base + (uint64_t)y * fb->pitch + (uint64_t)x * JP_UI_PIXEL_BYTES);

	*pixel = color;
}

static void clear_screen(const JPFramebufferInfo *fb)
{
	for (uint32_t y = 0; y < fb->height; y++) {
		for (uint32_t x = 0; x < fb->width; x++) {
			put_pixel(fb, x, y, JP_UI_COLOR_BLACK);
		}
	}
}

static char printable(char c)
{
	return ((unsigned char)c >= JP_UI_FIRST_PRINTABLE && (unsigned char)c <= JP_UI_LAST_PRINTABLE) ? c : '?';
}

/* One line of text on the cell grid; characters (and lines) that do not fit inside the framebuffer are cut off. */
static void draw_text(const JPFramebufferInfo *fb, uint32_t row, const char *text, size_t length, bool inverse)
{
	uint32_t columns = fb->width / JP_BOOT_UI_FONT_WIDTH;
	uint32_t top = row * JP_BOOT_UI_FONT_HEIGHT;
	uint32_t foreground = inverse ? JP_UI_COLOR_BLACK : JP_UI_COLOR_WHITE;
	uint32_t background = inverse ? JP_UI_COLOR_WHITE : JP_UI_COLOR_BLACK;

	if (columns > JP_UI_LINE_MAX) {
		columns = JP_UI_LINE_MAX;
	}
	if ((uint64_t)top + JP_BOOT_UI_FONT_HEIGHT > fb->height) {
		return;
	}
	for (size_t i = 0; i < length && i < columns; i++) {
		const uint8_t *glyph = jp_boot_ui_font8x16[(unsigned char)printable(text[i]) - JP_BOOT_UI_FONT_FIRST_CHAR];

		for (uint32_t y = 0; y < JP_BOOT_UI_FONT_HEIGHT; y++) {
			for (uint32_t x = 0; x < JP_BOOT_UI_FONT_WIDTH; x++) {
				put_pixel(fb, (uint32_t)i * JP_BOOT_UI_FONT_WIDTH + x, top + y,
					(glyph[y] & (0x80U >> x)) != 0 ? foreground : background);
			}
		}
	}
}

static void draw_line(const JPFramebufferInfo *fb, uint32_t row, const JPUiLine *line, bool inverse)
{
	draw_text(fb, row, line->text, line->length, inverse);
}

static void draw_field(const JPFramebufferInfo *fb, const JPUiState *ui, JPUiField field)
{
	const JPBootConfig *c = &ui->candidate;
	JPUiLine line;
	bool value;

	line.length = 0;
	put(&line, ui->field == (size_t)field ? "> " : "  ");
	switch (field) {
	case JP_UI_FIELD_KERNEL_PARAMETERS:
		put(&line, "Kernel parameters: ");
		put(&line, c->kernel_parameters);
		if (ui->editing) {
			put(&line, "_");
		}
		break;
	case JP_UI_FIELD_BOOT_LOGGING: value = c->boot_logging; put(&line, value ? "[x] Boot logging" : "[ ] Boot logging"); break;
	case JP_UI_FIELD_VERBOSE_OUTPUT: value = c->verbose_output; put(&line, value ? "[x] Verbose output" : "[ ] Verbose output"); break;
	case JP_UI_FIELD_SAFE_MODE: value = c->safe_mode; put(&line, value ? "[x] Safe mode" : "[ ] Safe mode"); break;
	default: value = c->recovery_requested; put(&line, value ? "[x] Recovery requested" : "[ ] Recovery requested"); break;
	}
	draw_line(fb, JP_UI_ROW_FIELDS + (uint32_t)field, &line, ui->field == (size_t)field);
}

/* The whole screen: black background, the title, the five fields, the information panel, the message, the help. */
static void draw_screen(const JPFramebufferInfo *fb, const JPUiState *ui, const JPBootSummary *summary)
{
	JPUiLine line;

	clear_screen(fb);
	draw_text(fb, JP_UI_ROW_TITLE, jp_ui_title, 16U, false);
	for (size_t f = 0; f < JP_UI_FIELD_COUNT; f++) {
		draw_field(fb, ui, (JPUiField)f);
	}
	line.length = 0;
	put(&line, "System information");
	draw_line(fb, JP_UI_ROW_INFO_TITLE, &line, false);
	for (size_t i = 0; i < JP_UI_SUMMARY_LINE_COUNT; i++) {
		summary_line(summary, i, &line);
		draw_line(fb, JP_UI_ROW_INFO + (uint32_t)i, &line, false);
	}
	if (ui->has_message) {
		line.length = 0;
		put(&line, jp_ui_rejection);
		draw_line(fb, JP_UI_ROW_MESSAGE, &line, false);
	}
	line.length = 0;
	put(&line, jp_ui_help);
	draw_line(fb, JP_UI_ROW_HELP, &line, false);
}

/* ---- Private helpers: the keys ---- */

static size_t text_length(const char *text)
{
	size_t n = 0;

	while (text[n] != '\0') {
		n++;
	}
	return n;
}

static void toggle_switch(JPUiState *ui)
{
	JPBootConfig *c = &ui->candidate;

	switch (ui->field) {
	case JP_UI_FIELD_BOOT_LOGGING: c->boot_logging = !c->boot_logging; break;
	case JP_UI_FIELD_VERBOSE_OUTPUT: c->verbose_output = !c->verbose_output; break;
	case JP_UI_FIELD_SAFE_MODE: c->safe_mode = !c->safe_mode; break;
	case JP_UI_FIELD_RECOVERY_REQUESTED: c->recovery_requested = !c->recovery_requested; break;
	default: break;
	}
}

typedef enum JPUiAction {
	JP_UI_ACTION_NONE,           /* keep going */
	JP_UI_ACTION_SUBMIT,         /* F10 */
	JP_UI_ACTION_DISCARD         /* Esc */
} JPUiAction;

/* One event, as the key table of the Boot UI Contract says. */
static JPUiAction handle_event(JPUiState *ui, const JPBootInputEvent *event)
{
	JPBootConfig *c = &ui->candidate;
	size_t length;

	if (event->kind == JP_BOOT_INPUT_CHARACTER) {
		length = text_length(c->kernel_parameters);
		if (ui->editing && event->character >= JP_UI_FIRST_PRINTABLE && event->character <= JP_UI_LAST_PRINTABLE &&
		    length + 1U < JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY) {
			c->kernel_parameters[length] = (char)event->character;
			c->kernel_parameters[length + 1U] = '\0';
		}
		return JP_UI_ACTION_NONE;
	}
	switch (event->key) {
	case JP_BOOT_KEY_F10:
		return JP_UI_ACTION_SUBMIT;
	case JP_BOOT_KEY_ESCAPE:
		return JP_UI_ACTION_DISCARD;
	case JP_BOOT_KEY_UP:
		if (!ui->editing) {
			ui->field = ui->field == 0 ? JP_UI_FIELD_COUNT - 1U : ui->field - 1U;
		}
		break;
	case JP_BOOT_KEY_DOWN:
		if (!ui->editing) {
			ui->field = ui->field + 1U >= JP_UI_FIELD_COUNT ? 0 : ui->field + 1U;
		}
		break;
	case JP_BOOT_KEY_LEFT:
	case JP_BOOT_KEY_RIGHT:
		if (!ui->editing) {
			toggle_switch(ui);
		}
		break;
	case JP_BOOT_KEY_ENTER:
		if (ui->field != JP_UI_FIELD_KERNEL_PARAMETERS) {
			toggle_switch(ui);
		} else {
			ui->editing = !ui->editing;
		}
		break;
	case JP_BOOT_KEY_BACKSPACE:
		length = text_length(c->kernel_parameters);
		if (ui->editing && length > 0) {
			c->kernel_parameters[length - 1U] = '\0';
		}
		break;
	default:
		break;
	}
	return JP_UI_ACTION_NONE;
}

/* ---- Public API (phase A) ---- */

/* The menu loop: draws, reads keys, and ends with one outcome or a failure. The menu has been drawn when it ends. */
static JPBootResult run_menu(JPBootContext *context, JPBootSelectionOutcome *outcome, JPUiState *ui,
			     const JPBootSummary *summary)
{
	JPBootInputEvent event;
	JPBootResult result;
	bool redraw = true;

	for (;;) {
		JPUiAction action;

		if (redraw) {
			draw_screen(&context->framebuffer, ui, summary);
			redraw = false;
		}
		result = jp_boot_environment_read_input(context, &event);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
		if (event.kind == JP_BOOT_INPUT_NONE) {
			continue;
		}
		ui->has_message = false;      /* the next key removes the message and is then handled as usual */
		redraw = true;
		action = handle_event(ui, &event);
		if (action == JP_UI_ACTION_DISCARD) {
			*outcome = JP_BOOT_SELECTION_DISCARDED;
			return JP_BOOT_SUCCESS;
		}
		if (action == JP_UI_ACTION_SUBMIT) {
			result = jp_boot_config_apply(context, &ui->candidate);
			if (result == JP_BOOT_SUCCESS) {
				*outcome = JP_BOOT_SELECTION_APPLIED;
				return JP_BOOT_SUCCESS;
			}
			if (result != JP_BOOT_ERROR_INVALID_PARAM) {
				return result;
			}
			ui->has_message = true;
		}
	}
}

JPBootResult jp_boot_ui_select(JPBootContext *context, JPBootSelectionOutcome *outcome)
{
	JPUiState ui;
	JPBootSummary summary;
	JPBootResult result;

	if (context == NULL || outcome == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage4.state != JP_BOOT_MANAGER_PREPARED) {
		return JP_BOOT_FAILED;
	}
	if (!framebuffer_is_usable(&context->framebuffer)) {
		return JP_BOOT_NOT_DETECTED;
	}
	result = jp_boot_config_get(context, &ui.candidate);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = jp_boot_info_summarize(context, &summary);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	ui.field = JP_UI_FIELD_KERNEL_PARAMETERS;
	ui.editing = false;
	ui.has_message = false;

	/* Whatever the result, the menu was the last user of the screen: leave it black (milestone0.md, small rule 6). */
	result = run_menu(context, outcome, &ui, &summary);
	clear_screen(&context->framebuffer);
	return result;
}

#else

/* ================= Phase B: the text screens ================= */

/* Sends one line; remembers the first failure and goes on, so that every line is tried. */
static void send_line(JPBootContext *context, const JPUiLine *line, JPBootResult *first_failure)
{
	JPBootDiagnosticMessage message;
	JPBootResult result;

	message.stage = JP_DIAGNOSTIC_STAGE_STAGE4;
	message.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
	message.diagnostic_context = 0;
	message.text = line->text;
	message.length = line->length;
	result = jp_boot_diagnostic_output(context, &message);
	if (result != JP_BOOT_SUCCESS && *first_failure == JP_BOOT_SUCCESS) {
		*first_failure = result;
	}
}

static const char *result_name(JPBootResult result)
{
	static const char *const names[] = {
		"JP_BOOT_SUCCESS", "JP_BOOT_NOT_INITIALIZED", "JP_BOOT_NOT_DETECTED", "JP_BOOT_UNKNOWN",
		"JP_BOOT_FAILED", "JP_BOOT_ERROR_INVALID_PARAM", "JP_BOOT_ERROR_CORRUPTED_DATA"
	};

	return (unsigned int)result <= JP_BOOT_ERROR_CORRUPTED_DATA ? names[result] : JP_UI_UNKNOWN_NAME;
}

static const char *stage_name(JPDiagnosticStage stage)
{
	static const char *const names[] = { JP_UI_UNKNOWN_NAME, "STAGE1", "STAGE2", "STAGE3", "STAGE4" };

	return (unsigned int)stage <= JP_DIAGNOSTIC_STAGE_STAGE4 ? names[stage] : JP_UI_UNKNOWN_NAME;
}

static const char *severity_name(JPBootSeverity severity)
{
	static const char *const names[] = { "INFO", "WARNING", "ERROR", "FATAL" };

	return (unsigned int)severity <= JP_BOOT_SEVERITY_FATAL ? names[severity] : JP_UI_UNKNOWN_NAME;
}

static const char *recoverability_name(JPBootRecoverability recoverability)
{
	static const char *const names[] = { "RECOVERABLE", "NON_RECOVERABLE", JP_UI_UNKNOWN_NAME };

	return (unsigned int)recoverability <= JP_BOOT_RECOVERABILITY_UNKNOWN ? names[recoverability] : JP_UI_UNKNOWN_NAME;
}

static const char *source_name(JPBootSourceComponent source)
{
	static const char *const names[] = {
		JP_UI_UNKNOWN_NAME, "STAGE1", "STAGE2", "BOOT_LOG", "BOOT_STATUS", "ERROR", "DIAGNOSTIC_OUTPUT",
		"BOOT_REPORT", "PANIC", "RECOVERY", "BOOT_CONFIG", "BOOT_INFO", "BOOT_UI", "BOOT_MANAGER"
	};

	return (unsigned int)source <= JP_BOOT_SOURCE_BOOT_MANAGER ? names[source] : JP_UI_UNKNOWN_NAME;
}

/* Every recorded event, one line each; used when verbose_output is set. */
static void send_events(JPBootContext *context, JPBootResult *first_failure)
{
	JPBootLogStatus log;
	JPUiLine line;

	if (jp_boot_log_get_status(context, &log) != JP_BOOT_SUCCESS) {
		line.length = 0;
		put(&line, "Events: ");
		put(&line, JP_UI_NOT_INITIALIZED_NAME);
		send_line(context, &line, first_failure);
		return;
	}
	for (size_t i = 0; i < log.event_count; i++) {
		JPBootEvent event;

		line.length = 0;
		put(&line, "Event ");
		put_decimal(&line, i);
		put(&line, ": ");
		if (jp_boot_log_get(context, i, &event) != JP_BOOT_SUCCESS) {
			put(&line, "not readable");
		} else {
			put(&line, stage_name(event.stage));
			put(&line, " ");
			put(&line, severity_name(event.severity));
			put(&line, " ");
			put(&line, result_name(event.result));
			put(&line, " ");
			put(&line, source_name(event.source_component));
		}
		send_line(context, &line, first_failure);
	}
}

static bool verbose_is_set(const JPBootContext *context)
{
	JPBootConfig config;

	return jp_boot_config_get(context, &config) == JP_BOOT_SUCCESS && config.verbose_output;
}

/* ---- Public API (phase B) ---- */

JPBootResult jp_boot_ui_present_summary(JPBootContext *context, const JPBootSummary *summary)
{
	JPBootResult first_failure = JP_BOOT_SUCCESS;
	JPUiLine line;

	if (context == NULL || summary == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	for (size_t i = 0; i < JP_UI_SUMMARY_LINE_COUNT; i++) {
		summary_line(summary, i, &line);
		send_line(context, &line, &first_failure);
	}
	if (verbose_is_set(context)) {
		send_events(context, &first_failure);
	}
	return first_failure;
}

JPBootResult jp_boot_ui_present_failure(JPBootContext *context)
{
	JPBootResult first_failure = JP_BOOT_SUCCESS;
	JPBootStatusState status;
	JPBootError error;
	JPBootLogStatus log;
	JPUiLine line;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	line.length = 0;
	put(&line, "JustOS boot diagnostics");
	send_line(context, &line, &first_failure);

	line.length = 0;
	put(&line, "Status: ");
	put(&line, jp_boot_status_get(context, &status) == JP_BOOT_SUCCESS ? status_name(status.status) : JP_UI_NOT_INITIALIZED_NAME);
	send_line(context, &line, &first_failure);

	line.length = 0;
	if (jp_boot_error_get(context, &error) == JP_BOOT_SUCCESS) {
		put(&line, "Error: ");
		put(&line, result_name(error.original_result));
		put(&line, " in ");
		put(&line, stage_name(error.stage));
		send_line(context, &line, &first_failure);

		line.length = 0;
		put(&line, "Severity: ");
		put(&line, severity_name(error.severity));
		put(&line, ", ");
		put(&line, recoverability_name(error.recoverability));
		put(&line, ", source ");
		put(&line, source_name(error.source_component));
		send_line(context, &line, &first_failure);
	} else {
		put(&line, "Error: none recorded");
		send_line(context, &line, &first_failure);
	}

	line.length = 0;
	put(&line, "Events: ");
	if (jp_boot_log_get_status(context, &log) == JP_BOOT_SUCCESS) {
		put_decimal(&line, log.event_count);
	} else {
		put(&line, JP_UI_NOT_INITIALIZED_NAME);
	}
	send_line(context, &line, &first_failure);

	if (verbose_is_set(context)) {
		send_events(context, &first_failure);
	}
	return first_failure;
}

#endif
