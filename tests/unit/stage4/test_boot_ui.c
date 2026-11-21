/*
 * Unit test (level U) of the menu of src/boot_ui.c (phase A): every key of the key table, the accepted, discarded,
 * and rejected paths, the information panel, the refusals, and that no pixel is written outside the framebuffer.
 * The keyboard is a fake that plays a script of events; the framebuffer is a buffer with guard bytes. What is on
 * the screen is read back with the independent font of the test kernel (font.h), so a wrong glyph, a wrong
 * place, or a wrong colour is noticed. The real boot_config.c, boot_info.c, boot_status.c, and boot_log.c are used.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "justos/boot/boot_config.h"
#include "justos/boot/boot_environment.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/boot_ui.h"

#define WIDTH 640U
#define HEIGHT 400U
#define PITCH (WIDTH * 4U + 32U)                /* a pitch with padding at the end of every line */
#define MAX_WIDTH 1024U                         /* the buffer holds the widest framebuffer that a test uses */
#define GUARD 256U
#define SENTINEL 0xA5U
#define WHITE 0x00FFFFFFU
#define BLACK 0x00000000U
#define MAX_EVENTS 400U
#define CELL_W JP_TEST_FONT_WIDTH
#define CELL_H JP_TEST_FONT_HEIGHT

static JPBootContext context;
static uint8_t *memory;                          /* guard + frame + guard */
static uint8_t *frame;
static size_t frame_size;
static uint8_t *snapshot;                        /* the frame as it was at the last key read (the last drawn menu) */
static bool black_after_run;                     /* was every pixel black when the menu returned? */
static bool outside_untouched_after_run;         /* were the guards and the line padding untouched when it returned? */
static uint32_t cur_width = WIDTH;                /* the geometry of the framebuffer of the current test */
static uint32_t cur_height = HEIGHT;
static uint32_t cur_pitch = PITCH;

/* ---- The fake keyboard ---- */

static JPBootInputEvent events[MAX_EVENTS];
static size_t event_count;
static size_t next_event;
static JPBootResult fail_result;                 /* returned instead of the event number fail_at */
static size_t fail_at;
static bool exhausted;
static unsigned int reads;
static bool message_at_read[MAX_EVENTS];
static int row_at_read[MAX_EVENTS];

static bool cell_matches(uint32_t row, uint32_t column, char c, bool inverse);
static bool screen_has(const char *text);
static int selected_row(void);

JPBootResult jp_boot_environment_read_input(JPBootContext *ctx, JPBootInputEvent *event)
{
	assert(ctx == &context);
	memcpy(snapshot, frame, frame_size);
	if (reads < MAX_EVENTS) {
		message_at_read[reads] = screen_has("The values are not valid. Nothing was changed.");
		row_at_read[reads] = selected_row();
	}
	reads++;
	if (fail_result != JP_BOOT_SUCCESS && next_event == fail_at) {
		return fail_result;
	}
	if (next_event >= event_count) {
		exhausted = true;                        /* the script ended without Esc or F10: the test would hang */
		return JP_BOOT_FAILED;
	}
	*event = events[next_event++];
	return JP_BOOT_SUCCESS;
}

static void script_clear(void)
{
	event_count = 0;
	next_event = 0;
	fail_result = JP_BOOT_SUCCESS;
	exhausted = false;
	reads = 0;
}

static void key(JPBootInputKey k)
{
	events[event_count].kind = JP_BOOT_INPUT_KEY;
	events[event_count].key = k;
	events[event_count].character = 0;
	event_count++;
}

static void chr(uint32_t c)
{
	events[event_count].kind = JP_BOOT_INPUT_CHARACTER;
	events[event_count].key = JP_BOOT_KEY_UP;     /* must be ignored for a character */
	events[event_count].character = c;
	event_count++;
}

static void none(void)
{
	events[event_count].kind = JP_BOOT_INPUT_NONE;
	events[event_count].key = JP_BOOT_KEY_F10;    /* must be ignored for NONE */
	events[event_count].character = 'x';
	event_count++;
}

/* ---- The screen ---- */

static uint32_t pixel(uint32_t x, uint32_t y)
{
	uint32_t v;

	memcpy(&v, frame + (size_t)y * cur_pitch + (size_t)x * 4U, sizeof v);
	return v;
}

/* Does the cell (row, column) show the glyph of c, white on black (or black on white when inverse)? */
static bool cell_matches(uint32_t row, uint32_t column, char c, bool inverse)
{
	for (uint32_t y = 0; y < CELL_H; y++) {
		for (uint32_t x = 0; x < CELL_W; x++) {
			bool lit = (jp_test_font8x16[(unsigned char)c - JP_TEST_FONT_FIRST_CHAR][y] & (0x80U >> x)) != 0;
			uint32_t want = (lit != inverse) ? WHITE : BLACK;

			if (pixel(column * CELL_W + x, row * CELL_H + y) != want) {
				return false;
			}
		}
	}
	return true;
}

static bool text_at(uint32_t row, uint32_t column, const char *text, bool inverse)
{
	for (size_t i = 0; text[i] != '\0'; i++) {
		if (column + i >= cur_width / CELL_W || (row + 1U) * CELL_H > cur_height || !cell_matches(row, column + (uint32_t)i, text[i], inverse)) {
			return false;
		}
	}
	return true;
}

/* Is the whole cell (row, column) black? */
static bool cell_is_blank(uint32_t row, uint32_t column)
{
	for (uint32_t y = 0; y < CELL_H; y++) {
		for (uint32_t x = 0; x < CELL_W; x++) {
			if (pixel(column * CELL_W + x, row * CELL_H + y) != BLACK) {
				return false;
			}
		}
	}
	return true;
}

static bool row_is_blank(uint32_t row)
{
	for (uint32_t column = 0; column < cur_width / CELL_W; column++) {
		if (!cell_is_blank(row, column)) {
			return false;
		}
	}
	return true;
}

/* The text anywhere on the grid of cells, written normally (the text of a selected line is inverse: see selected_row). */
static bool screen_has(const char *text)
{
	for (uint32_t row = 0; row < cur_height / CELL_H; row++) {
		for (uint32_t column = 0; column + strlen(text) <= cur_width / CELL_W; column++) {
			if (text_at(row, column, text, false)) {
				return true;
			}
		}
	}
	return false;
}

static bool screen_has_inverse_row(uint32_t row, const char *text)
{
	return text_at(row, 0, text, true);
}

/* The row (2 to 6) of the field that carries the marker "> " (drawn inverse), or -1. */
static int selected_row(void)
{
	for (uint32_t row = 2; row <= 6; row++) {
		if (text_at(row, 0, ">", true)) {
			return (int)row;
		}
	}
	return -1;
}

/* ---- Setup ---- */

static void set_up(JPFramebufferFormat format)
{
	if (memory == NULL) {
		frame_size = ((size_t)MAX_WIDTH * 4U + 32U) * HEIGHT;
		memory = malloc(GUARD + frame_size + GUARD);
		snapshot = malloc(frame_size);
		assert(snapshot != NULL);
		assert(memory != NULL);
		frame = memory + GUARD;
	}
	memset(memory, SENTINEL, GUARD + frame_size + GUARD);
	cur_width = WIDTH;
	cur_height = HEIGHT;
	cur_pitch = PITCH;
	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_X86_64;
	context.environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context.environment.state = JP_DETECTION_STATE_DETECTED;
	context.hardware.overall_state = JP_DETECTION_STATE_DETECTED;
	context.memory_map.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.base_address = (uint64_t)(uintptr_t)frame;
	context.framebuffer.width = WIDTH;
	context.framebuffer.height = HEIGHT;
	context.framebuffer.pitch = PITCH;
	context.framebuffer.format = format;
	assert(jp_boot_config_initialize(&context) == JP_BOOT_SUCCESS);
	context.stage4.state = JP_BOOT_MANAGER_PREPARED;
	script_clear();
}

static bool whole_screen_is_black(void);
static bool nothing_outside(size_t width, size_t height, size_t pitch);

/*
 * Runs the menu. The menu leaves the screen black, so that is recorded in black_after_run; then the last drawn menu
 * (the frame at the last key read) is put back, so that the tests can look at what the menu showed.
 */
static JPBootResult run(JPBootSelectionOutcome *outcome)
{
	JPBootResult result;

	*outcome = JP_BOOT_SELECTION_NOT_RUN;
	memcpy(snapshot, frame, frame_size);          /* a run that never reads a key restores the frame as it was */
	result = jp_boot_ui_select(&context, outcome);
	black_after_run = whole_screen_is_black();
	outside_untouched_after_run = nothing_outside(cur_width, cur_height, cur_pitch);
	memcpy(frame, snapshot, frame_size);
	return result;
}

static JPBootConfig config(void)
{
	JPBootConfig c;

	assert(jp_boot_config_get(&context, &c) == JP_BOOT_SUCCESS);
	return c;
}

/* The guard bytes before and after the frame, and the padding at the end of every line, are untouched. */
static bool nothing_outside(size_t width, size_t height, size_t pitch)
{
	for (size_t i = 0; i < GUARD; i++) {
		if (memory[i] != SENTINEL || frame[frame_size + i] != SENTINEL) {
			return false;
		}
	}
	for (size_t y = 0; y < height; y++) {
		for (size_t i = width * 4U; i < pitch; i++) {
			if (frame[y * pitch + i] != SENTINEL) {
				return false;
			}
		}
	}
	for (size_t i = height * pitch; i < frame_size; i++) {
		if (frame[i] != SENTINEL) {
			return false;             /* the lines below the screen */
		}
	}
	return true;
}

static void assert_nothing_outside(size_t width, size_t height, size_t pitch)
{
	assert(nothing_outside(width, height, pitch));
}

/* ---- The tests ---- */

static void test_arguments_and_state(void)
{
	JPBootSelectionOutcome outcome = JP_BOOT_SELECTION_NOT_RUN;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	assert(jp_boot_ui_select(NULL, &outcome) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_ui_select(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	for (JPBootManagerState s = JP_BOOT_MANAGER_UNINITIALIZED; s <= JP_BOOT_MANAGER_DECIDED; s++) {
		if (s == JP_BOOT_MANAGER_PREPARED) {
			continue;
		}
		context.stage4.state = s;
		assert(jp_boot_ui_select(&context, &outcome) == JP_BOOT_FAILED);
	}
	assert(reads == 0 && outcome == JP_BOOT_SELECTION_NOT_RUN);
	assert(frame[0] == SENTINEL);                 /* nothing was drawn */
}

static void test_unusable_framebuffer(void)
{
	JPBootSelectionOutcome outcome;
	JPFramebufferInfo good;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	good = context.framebuffer;
	for (int variant = 0; variant < 9; variant++) {
		JPBootConfig before = config();

		context.framebuffer = good;
		switch (variant) {
		case 0: context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED; break;
		case 1: context.framebuffer.state = JP_DETECTION_STATE_FAILED; break;
		case 2: context.framebuffer.base_address = 0; break;
		case 3: context.framebuffer.width = 0; break;
		case 4: context.framebuffer.height = 0; break;
		case 5: context.framebuffer.pitch = 0; break;
		case 6: context.framebuffer.format = JP_FRAMEBUFFER_FORMAT_UNKNOWN; break;
		case 7: context.framebuffer.pitch = WIDTH * 4U - 4U; break;              /* too small for the width */
		default: context.framebuffer.pitch = WIDTH * 4U + 2U; break;             /* not a multiple of 4 */
		}
		script_clear();
		key(JP_BOOT_KEY_F10);
		assert(run(&outcome) == JP_BOOT_NOT_DETECTED);
		assert(reads == 0 && outcome == JP_BOOT_SELECTION_NOT_RUN);
		assert(frame[0] == SENTINEL && frame[frame_size - 1U] == SENTINEL);       /* nothing was drawn */
		assert(config().source == before.source && config().boot_logging == before.boot_logging);
	}
}

static void test_escape(void)
{
	JPBootSelectionOutcome outcome;
	JPBootConfig c;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_DOWN);
	key(JP_BOOT_KEY_ENTER);                       /* boot logging off, in the candidate only */
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(outcome == JP_BOOT_SELECTION_DISCARDED);
	c = config();
	assert(c.source == JP_BOOT_CONFIG_SOURCE_DEFAULT && c.boot_logging && c.state == JP_BOOT_CONFIG_STATE_VALIDATED);
	assert(next_event == 3 && !exhausted);        /* nothing was read after Esc */
	assert_nothing_outside(WIDTH, HEIGHT, PITCH);
}

/* Every pixel of the visible width of every line is black. */
static bool whole_screen_is_black(void)
{
	for (uint32_t y = 0; y < cur_height; y++) {
		for (uint32_t x = 0; x < cur_width; x++) {
			if (pixel(x, y) != BLACK) {
				return false;
			}
		}
	}
	return true;
}

/* When the menu ends after it was drawn, the whole screen is black: Stage 3 clears only its own lines. */
static void test_screen_is_cleared_on_exit(void)
{
	JPBootSelectionOutcome outcome;
	JPBootResult results[] = { JP_BOOT_FAILED, JP_BOOT_NOT_INITIALIZED };

	/* Esc */
	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(black_after_run);
	assert(outside_untouched_after_run);

	/* F10 after a change (the menu showed the changed field) */
	set_up(JP_FRAMEBUFFER_FORMAT_BGR_888);
	key(JP_BOOT_KEY_DOWN);
	key(JP_BOOT_KEY_ENTER);
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(outcome == JP_BOOT_SELECTION_APPLIED);
	assert(black_after_run);
	assert(outside_untouched_after_run);

	/* A keyboard failure after the menu was drawn: the failure is returned and the screen is still cleared. */
	for (size_t i = 0; i < sizeof results / sizeof results[0]; i++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		key(JP_BOOT_KEY_DOWN);
		fail_result = results[i];
		fail_at = 1;
		assert(run(&outcome) == results[i]);
		assert(black_after_run);
		assert(outside_untouched_after_run);
	}
}

static void test_f10_unchanged(void)
{
	JPBootSelectionOutcome outcome;
	JPBootConfig c;

	set_up(JP_FRAMEBUFFER_FORMAT_BGR_888);        /* BGR works the same */
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(outcome == JP_BOOT_SELECTION_APPLIED);
	c = config();
	assert(c.source == JP_BOOT_CONFIG_SOURCE_INTERACTIVE && c.boot_logging && !c.verbose_output);
	assert(c.state == JP_BOOT_CONFIG_STATE_VALIDATED);
}

static void test_screen_content(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	context.hardware.overall_state = JP_DETECTION_STATE_NOT_DETECTED;
	context.memory_map.state = JP_DETECTION_STATE_FAILED;
	context.handoff.architecture = JP_ARCH_AARCH64;
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	/* The screen at the time of the first read: the title, the five fields, the panel, the help. */
	assert(screen_has("JustOS boot menu"));
	assert(screen_has_inverse_row(2, "> Kernel parameters: "));        /* the current field is inverse */
	assert(screen_has("[x] Boot logging"));
	assert(screen_has("[ ] Verbose output"));
	assert(screen_has("[ ] Safe mode"));
	assert(screen_has("[ ] Recovery requested"));
	assert(screen_has("System information"));
	assert(screen_has("Architecture: AArch64"));
	assert(screen_has("Environment: BIOS"));
	assert(screen_has("Hardware: NOT DETECTED"));                      /* exactly as the owner reported */
	assert(screen_has("Memory map: FAILED"));
	assert(screen_has("Framebuffer: DETECTED"));
	assert(screen_has("Boot status: NOT INITIALIZED"));                /* Stage 3 has not started */
	assert(screen_has("Configuration: VALIDATED (DEFAULT)"));
	assert(screen_has("Up/Down: field"));
	assert(!screen_has("The values are not valid."));
	/* Nothing is drawn after the end of a text, and the rows that no text owns are black. */
	assert(cell_is_blank(0, 16));                  /* after the title */
	assert(row_is_blank(1) && row_is_blank(7) && row_is_blank(20));
	/* Outside the text everything is black. */
	assert(pixel(WIDTH - 1U, HEIGHT - 1U) == BLACK && pixel(WIDTH - 1U, 3) == BLACK);
	assert_nothing_outside(WIDTH, HEIGHT, PITCH);
}

/* Every value of every state that the panel names, down to the last one of each table. */
static void test_panel_names(void)
{
	JPBootSelectionOutcome outcome;
	static const char *const validation[] = { "UNKNOWN", "NOT INITIALIZED", "VALID", "INVALID", "FAILED" };
	static const char *const load[] = { "UNKNOWN", "NOT INITIALIZED", "LOADED", "FAILED" };
	static const char *const detection[] = { "UNKNOWN", "NOT DETECTED", "NOT INITIALIZED", "DETECTED", "FAILED" };
	static const char *const config_state[] = { "UNSET", "VALIDATED", "FROZEN" };
	static const char *const config_source[] = { "UNKNOWN", "DEFAULT", "INTERACTIVE" };
	static const char *const menu[] = {
		"NOT_RUN", "SKIPPED_POLICY", "SKIPPED_NO_FRAMEBUFFER", "SKIPPED_NO_INPUT", "APPLIED", "DISCARDED"
	};
	char text[64];

	for (int v = 0; v <= JP_KERNEL_VALIDATION_FAILED; v++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		context.kernel.validation_state = (JPKernelValidationState)v;
		key(JP_BOOT_KEY_ESCAPE);
		assert(run(&outcome) == JP_BOOT_SUCCESS);
		strcpy(text, "Kernel validation: ");
		strcat(text, validation[v]);
		assert(screen_has(text));
	}
	for (int v = 0; v <= JP_KERNEL_LOAD_FAILED; v++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		context.kernel.load_state = (JPKernelLoadState)v;
		key(JP_BOOT_KEY_ESCAPE);
		assert(run(&outcome) == JP_BOOT_SUCCESS);
		strcpy(text, "Kernel load: ");
		strcat(text, load[v]);
		assert(screen_has(text));
	}
	for (int v = 0; v <= JP_DETECTION_STATE_FAILED; v++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		context.environment.state = (JPDetectionState)v;
		key(JP_BOOT_KEY_ESCAPE);
		assert(run(&outcome) == JP_BOOT_SUCCESS);
		strcpy(text, "Environment state: ");
		strcat(text, detection[v]);
		assert(screen_has(text));
	}
	for (int v = 0; v <= JP_BOOT_CONFIG_STATE_FROZEN; v++) {
		for (int w = 0; w <= JP_BOOT_CONFIG_SOURCE_INTERACTIVE; w++) {
			set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
			context.stage4.config.state = (JPBootConfigState)(v == 0 ? 1 : v);      /* the menu needs a set configuration */
			context.stage4.config.source = (JPBootConfigSource)w;
			key(JP_BOOT_KEY_ESCAPE);
			assert(run(&outcome) == JP_BOOT_SUCCESS);
			strcpy(text, "Configuration: ");
			strcat(text, config_state[v == 0 ? 1 : v]);
			strcat(text, " (");
			strcat(text, config_source[w]);
			strcat(text, ")");
			assert(screen_has(text));
		}
	}
	for (int v = 0; v <= JP_BOOT_SELECTION_DISCARDED; v++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		context.stage4.selection_outcome = (JPBootSelectionOutcome)v;
		key(JP_BOOT_KEY_ESCAPE);
		assert(run(&outcome) == JP_BOOT_SUCCESS);
		strcpy(text, "Menu: ");
		strcat(text, menu[v]);
		assert(screen_has(text));
	}
	/* Every boot status that Stage 3 can report (set in the Stage 3 area; the real status module reads it). */
	{
		static const char *const status[] = { "BOOTING", "INITIALIZING", "RUNNING", "FAILED", "RECOVERY", "HALTED" };

		for (int v = 0; v <= JP_BOOT_STATUS_HALTED; v++) {
			set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
			context.stage3.log_initialized = true;
			context.stage3.status_initialized = true;
			context.stage3.status.status = (JPBootStatus)v;
			key(JP_BOOT_KEY_ESCAPE);
			assert(run(&outcome) == JP_BOOT_SUCCESS);
			strcpy(text, "Boot status: ");
			strcat(text, status[v]);
			assert(screen_has(text));
		}
	}
	/* A boot status that Stage 3 reported is shown (the real status module starts here). */
	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(screen_has("Boot status: BOOTING"));
	/* A value outside every table says UNKNOWN. */
	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	context.handoff.architecture = (JPArchitectureType)9;
	context.environment.type = (JPBootEnvironmentType)9;
	context.kernel.validation_state = (JPKernelValidationState)9;
	context.kernel.load_state = (JPKernelLoadState)9;
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(screen_has("Architecture: UNKNOWN") && screen_has("Environment: UNKNOWN"));
	assert(screen_has("Kernel validation: UNKNOWN") && screen_has("Kernel load: UNKNOWN"));
}

/* A character that is not printable is drawn as '?', and a long text is cut at 80 columns, also on a wide screen. */
static void test_text_drawing_limits(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	strcpy(context.stage4.config.kernel_parameters, "a\x01" "b~ ");
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(text_at(2, 2, "Kernel parameters: a?b~ ", true));       /* the last printable character is drawn as it is */

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	cur_width = 800;
	cur_pitch = 800U * 4U + 32U;
	context.framebuffer.width = cur_width;
	context.framebuffer.pitch = cur_pitch;
	memset(context.stage4.config.kernel_parameters, 'm', JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY - 1U);
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(!cell_is_blank(2, 79));                 /* the last column that is used */
	assert(cell_is_blank(2, 80) && cell_is_blank(2, 99));
	assert_nothing_outside(800, HEIGHT, cur_pitch);
}

static void test_navigation_and_switches(void)
{
	JPBootSelectionOutcome outcome;
	JPBootConfig c;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_UP);                          /* from the first field to the last (wraps) */
	key(JP_BOOT_KEY_ENTER);                       /* recovery requested on */
	key(JP_BOOT_KEY_DOWN);                        /* from the last field to the first (wraps) */
	key(JP_BOOT_KEY_DOWN);                        /* boot logging */
	key(JP_BOOT_KEY_LEFT);                        /* boot logging off */
	key(JP_BOOT_KEY_DOWN);                        /* verbose output */
	key(JP_BOOT_KEY_RIGHT);                       /* verbose on */
	key(JP_BOOT_KEY_DOWN);                        /* safe mode */
	key(JP_BOOT_KEY_ENTER);                       /* safe on */
	key(JP_BOOT_KEY_ENTER);                       /* safe off again */
	key(JP_BOOT_KEY_ENTER);                       /* safe on */
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_APPLIED);
	c = config();
	assert(!c.boot_logging && c.verbose_output && c.safe_mode && c.recovery_requested);
	assert(c.kernel_parameters[0] == '\0');
	/* The selected row at each read (the marker is drawn before the key is read): 2, 6, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5. */
	assert(row_at_read[0] == 2 && row_at_read[1] == 6 && row_at_read[2] == 6 && row_at_read[3] == 2);
	assert(row_at_read[4] == 3 && row_at_read[5] == 3 && row_at_read[6] == 4 && row_at_read[7] == 4);
	assert(row_at_read[8] == 5 && row_at_read[9] == 5);
}

static void test_left_right_on_text_field_do_nothing(void)
{
	JPBootSelectionOutcome outcome;
	JPBootConfig c;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_LEFT);
	key(JP_BOOT_KEY_RIGHT);
	key(JP_BOOT_KEY_BACKSPACE);                   /* not editing: ignored */
	chr('q');                                     /* not editing: ignored */
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	c = config();
	assert(c.kernel_parameters[0] == '\0' && c.boot_logging && !c.verbose_output && !c.safe_mode && !c.recovery_requested);
	assert(row_at_read[1] == 2 && row_at_read[2] == 2);
}

static void test_editing(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);                       /* start editing */
	chr('a');
	chr('b');
	chr(' ');
	chr('c');
	key(JP_BOOT_KEY_BACKSPACE);                   /* remove c */
	key(JP_BOOT_KEY_DOWN);                        /* ignored while editing */
	key(JP_BOOT_KEY_UP);
	key(JP_BOOT_KEY_LEFT);
	key(JP_BOOT_KEY_RIGHT);
	chr(0x07);                                    /* not printable: ignored */
	chr(0x7F);
	chr(0x100);
	chr(0xE9);
	chr(0);
	chr('~');                                    /* the last printable character is accepted */
	chr('x');
	key(JP_BOOT_KEY_ENTER);                       /* stop editing */
	key(JP_BOOT_KEY_DOWN);                        /* now it moves */
	chr('z');                                     /* not editing: ignored */
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_APPLIED);
	assert(strcmp(config().kernel_parameters, "ab ~x") == 0);
	/* Reads 7 to 10 follow Down, Up, Left, Right while editing: the field did not move. */
	assert(row_at_read[7] == 2 && row_at_read[8] == 2 && row_at_read[9] == 2 && row_at_read[10] == 2);
	assert(row_at_read[18] == 2 && row_at_read[19] == 3);     /* after the second Enter, Down moves */
}

static void test_up_in_the_middle_and_backspace_outside_editing(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	strcpy(context.stage4.config.kernel_parameters, "abc");
	key(JP_BOOT_KEY_BACKSPACE);                   /* not editing: the text stays */
	key(JP_BOOT_KEY_DOWN);
	key(JP_BOOT_KEY_DOWN);
	key(JP_BOOT_KEY_DOWN);
	key(JP_BOOT_KEY_UP);                          /* from the middle: one field back */
	key(JP_BOOT_KEY_UP);
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(strcmp(config().kernel_parameters, "abc") == 0);
	assert(row_at_read[4] == 5 && row_at_read[5] == 4 && row_at_read[6] == 3);
}

static void test_editing_is_shown(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);
	chr('h');
	chr('i');
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	/* The last screen was drawn before Esc: the text and the cursor are shown. */
	assert(text_at(2, 2, "Kernel parameters: hi_", true));
}

static void test_text_limit(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);
	for (int i = 0; i < 300; i++) {
		chr('z');
	}
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_APPLIED);
	assert(strlen(config().kernel_parameters) == JP_BOOT_CONFIG_KERNEL_PARAMETERS_CAPACITY - 1U);
	assert_nothing_outside(WIDTH, HEIGHT, PITCH);          /* a line of 255 characters is cut at the screen */
}

static void test_backspace_on_empty_text(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);
	key(JP_BOOT_KEY_BACKSPACE);
	key(JP_BOOT_KEY_BACKSPACE);
	chr('k');
	key(JP_BOOT_KEY_BACKSPACE);
	key(JP_BOOT_KEY_BACKSPACE);
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_SUCCESS);
	assert(config().kernel_parameters[0] == '\0');
}

static void test_submit_and_discard_while_editing(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);
	chr('a');
	key(JP_BOOT_KEY_F10);                         /* F10 works while editing */
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_APPLIED);
	assert(strcmp(config().kernel_parameters, "a") == 0);

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	key(JP_BOOT_KEY_ENTER);
	chr('a');
	key(JP_BOOT_KEY_ESCAPE);                      /* Esc works while editing */
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(config().kernel_parameters[0] == '\0' && config().source == JP_BOOT_CONFIG_SOURCE_DEFAULT);
}

static void test_none_events_and_unknown_keys(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	none();
	none();
	events[event_count].kind = JP_BOOT_INPUT_KEY;      /* a key value that the table does not know */
	events[event_count].key = (JPBootInputKey)99;
	event_count++;
	none();
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(reads == 5);
	assert(row_at_read[4] == 2);
}

static void test_rejected_candidate(void)
{
	JPBootSelectionOutcome outcome;
	JPBootConfig before;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	context.stage4.config.entry_count = JP_BOOT_CONFIG_ENTRY_CAPACITY + 1U;   /* the candidate is copied from here and is not valid */
	before = config();
	key(JP_BOOT_KEY_F10);                         /* rejected: the menu stays open */
	key(JP_BOOT_KEY_DOWN);                        /* removes the message, and then moves */
	key(JP_BOOT_KEY_F10);                         /* rejected again */
	key(JP_BOOT_KEY_ESCAPE);                      /* removes the message, then discards */
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(!message_at_read[0]);
	assert(message_at_read[1]);                   /* shown after the rejection */
	assert(row_at_read[1] == 2);                  /* ... and the field has not moved yet */
	assert(!message_at_read[2] && row_at_read[2] == 3);   /* the next key removed it and was handled (the field moved) */
	assert(message_at_read[3]);
	assert(config().source == before.source && config().entry_count == before.entry_count);
}

static void test_other_apply_failure_is_returned(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	assert(jp_boot_config_freeze(&context) == JP_BOOT_SUCCESS);      /* apply now fails with JP_BOOT_FAILED */
	key(JP_BOOT_KEY_F10);
	assert(run(&outcome) == JP_BOOT_FAILED);
	assert(outcome == JP_BOOT_SELECTION_NOT_RUN);
}

static void test_keyboard_failure(void)
{
	JPBootSelectionOutcome outcome;
	JPBootResult results[] = { JP_BOOT_FAILED, JP_BOOT_NOT_INITIALIZED, JP_BOOT_ERROR_CORRUPTED_DATA };

	for (size_t i = 0; i < sizeof results / sizeof results[0]; i++) {
		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		key(JP_BOOT_KEY_DOWN);
		key(JP_BOOT_KEY_ENTER);
		key(JP_BOOT_KEY_F10);
		fail_result = results[i];
		fail_at = 2;                              /* the third read fails, after one change */
		assert(run(&outcome) == results[i]);      /* unchanged */
		assert(outcome == JP_BOOT_SELECTION_NOT_RUN);
		assert(config().source == JP_BOOT_CONFIG_SOURCE_DEFAULT && config().boot_logging);
	}
}

static void test_small_framebuffers(void)
{
	static const uint32_t sizes[][2] = { { 8, 8 }, { 16, 16 }, { 100, 40 }, { 8, 400 }, { 640, 20 }, { 640, 16 }, { 640, 32 }, { 12, 17 } };
	JPBootSelectionOutcome outcome;

	for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
		uint32_t w = sizes[i][0];
		uint32_t h = sizes[i][1];
		uint32_t pitch = w * 4U + 16U;

		set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
		cur_width = w;
		cur_height = h;
		cur_pitch = pitch;
		context.framebuffer.width = w;
		context.framebuffer.height = h;
		context.framebuffer.pitch = pitch;
		key(JP_BOOT_KEY_ENTER);
		for (int k = 0; k < 120; k++) {
			chr('w');
		}
		key(JP_BOOT_KEY_F10);
		assert(run(&outcome) == JP_BOOT_SUCCESS);
		assert_nothing_outside(w, h, pitch);
		if (h >= CELL_H && w >= 16U * CELL_W) {
			assert(text_at(0, 0, "JustOS boot menu", false));     /* a row that fits exactly is drawn */
		}
	}
}

/* A pitch of exactly width * 4 (no padding) is usable. */
static void test_exact_pitch(void)
{
	JPBootSelectionOutcome outcome;

	set_up(JP_FRAMEBUFFER_FORMAT_RGB_888);
	cur_pitch = WIDTH * 4U;
	context.framebuffer.pitch = cur_pitch;
	key(JP_BOOT_KEY_ESCAPE);
	assert(run(&outcome) == JP_BOOT_SUCCESS && outcome == JP_BOOT_SELECTION_DISCARDED);
	assert(screen_has("JustOS boot menu"));
}

int main(void)
{
	test_arguments_and_state();
	test_unusable_framebuffer();
	test_escape();
	test_f10_unchanged();
	test_screen_is_cleared_on_exit();
	test_screen_content();
	test_panel_names();
	test_text_drawing_limits();
	test_exact_pitch();
	test_up_in_the_middle_and_backspace_outside_editing();
	test_navigation_and_switches();
	test_left_right_on_text_field_do_nothing();
	test_editing();
	test_editing_is_shown();
	test_text_limit();
	test_backspace_on_empty_text();
	test_submit_and_discard_while_editing();
	test_none_events_and_unknown_keys();
	test_rejected_candidate();
	test_other_apply_failure_is_returned();
	test_keyboard_failure();
	test_small_framebuffers();
	free(snapshot);
	free(memory);
	return 0;
}
