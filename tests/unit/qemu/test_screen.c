/* Unit test (level U) of the on-screen list (tests/qemu/test_kernel/screen.c). */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "report.h"
#include "screen.h"

#define SENTINEL 0xAB
#define GUARD 4096

typedef struct Screen {
	uint8_t *memory;       /* GUARD bytes before and after the framebuffer */
	uint8_t *pixels;       /* the framebuffer itself */
	size_t size;
} Screen;

static Screen make_screen(const JPFramebufferInfo *fb)
{
	Screen screen;

	screen.size = (size_t)fb->pitch * fb->height;
	screen.memory = (uint8_t *)malloc(screen.size + 2 * GUARD);
	assert(screen.memory != NULL);
	memset(screen.memory, SENTINEL, screen.size + 2 * GUARD);
	screen.pixels = screen.memory + GUARD;
	return screen;
}

static void free_screen(Screen *screen)
{
	free(screen->memory);
}

static void make_context(JPBootContext *c, uint32_t width, uint32_t height, uint32_t pitch)
{
	memset(c, 0, sizeof *c);
	c->current_stage = JP_BOOT_STAGE_READY;
	c->environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	c->hardware.overall_state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.region_count = 1;
	c->memory_map.regions[0].length = 0x1000;
	c->memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;
	c->kernel.load_state = JP_KERNEL_LOAD_LOADED;
	c->handoff.state = JP_HANDOFF_STATE_READY;
	c->handoff.architecture = JP_ARCH_X86_64;
	c->handoff.paging_enabled = true;
	c->handoff.interrupts_disabled = true;
	c->handoff.ownership_transferred = true;
	c->framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c->framebuffer.base_address = 0x80000000ULL;
	c->framebuffer.width = width;
	c->framebuffer.height = height;
	c->framebuffer.pitch = pitch;
	c->framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
}

static bool pixel_lit(const Screen *screen, uint32_t pitch, size_t x, size_t y)
{
	const uint8_t *pixel = screen->pixels + y * pitch + x * 4;

	return pixel[0] == 0xFF && pixel[1] == 0xFF && pixel[2] == 0xFF && pixel[3] == 0;
}

static void check_guards_untouched(const Screen *screen)
{
	size_t i;

	for (i = 0; i < GUARD; i++) {
		assert(screen->memory[i] == SENTINEL);
		assert(screen->memory[GUARD + screen->size + i] == SENTINEL);
	}
}

typedef struct CountState {
	size_t lines;
	char first[64];
	size_t first_length;
} CountState;

static void count_put(void *user, char c)
{
	CountState *state = (CountState *)user;

	if (c == '\n') {
		state->lines++;
	} else if (state->lines == 0 && state->first_length + 1 < sizeof state->first) {
		state->first[state->first_length++] = c;
		state->first[state->first_length] = '\0';
	}
}

static size_t list_lines(const JPBootContext *c, char *first, size_t first_size)
{
	CountState state;
	JPTestSink sink = { count_put, &state };

	memset(&state, 0, sizeof state);
	jp_test_report_emit(c, &sink);
	if (first != NULL) {
		strncpy(first, state.first, first_size - 1);
		first[first_size - 1] = '\0';
	}
	return state.lines;
}

static void test_first_line_is_drawn_with_the_font(void)
{
	JPBootContext c;
	Screen screen;
	size_t drawn;
	size_t y;
	size_t x;
	const char *text = "JP|machine=x86_64";
	size_t i;

	make_context(&c, 1024, 768, 4096);
	screen = make_screen(&c.framebuffer);
	drawn = jp_test_screen_show(&c, screen.pixels);
	assert(drawn == list_lines(&c, NULL, 0));      /* everything fits */
	for (i = 0; text[i] != '\0'; i++) {
		for (y = 0; y < JP_TEST_FONT_HEIGHT; y++) {
			for (x = 0; x < JP_TEST_FONT_WIDTH; x++) {
				bool expected = ((jp_test_font8x16[text[i] - JP_TEST_FONT_FIRST_CHAR][y]
					>> (JP_TEST_FONT_WIDTH - 1 - x)) & 1U) != 0;

				assert(pixel_lit(&screen, 4096, i * JP_TEST_FONT_WIDTH + x, y) == expected);
			}
		}
	}
	check_guards_untouched(&screen);
	free_screen(&screen);
}

static void test_region_is_cleared_and_padding_is_untouched(void)
{
	JPBootContext c;
	Screen screen;
	size_t y;
	size_t x;

	make_context(&c, 800, 600, 4096);               /* pitch is larger than width * 4 */
	screen = make_screen(&c.framebuffer);
	assert(jp_test_screen_show(&c, screen.pixels) > 0);
	for (y = 0; y < 600; y++) {
		for (x = 800 * 4; x < 4096; x++) {
			assert(screen.pixels[y * 4096 + x] == SENTINEL);   /* padding of the pitch */
		}
	}
	/* The bottom-right pixel is background: black, not the sentinel. */
	assert(screen.pixels[599 * 4096 + 799 * 4] == 0);
	check_guards_untouched(&screen);
	free_screen(&screen);
}

static void test_nothing_is_drawn_without_a_usable_framebuffer(void)
{
	JPBootContext c;
	Screen screen;
	size_t i;

	make_context(&c, 1024, 768, 4096);
	screen = make_screen(&c.framebuffer);

	c.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(jp_test_screen_show(&c, screen.pixels) == 0);
	c.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c.framebuffer.format = JP_FRAMEBUFFER_FORMAT_UNKNOWN;
	assert(jp_test_screen_show(&c, screen.pixels) == 0);
	c.framebuffer.format = JP_FRAMEBUFFER_FORMAT_RGB_888;
	assert(jp_test_screen_show(&c, NULL) == 0);
	c.framebuffer.width = 639;                       /* narrower than one column */
	assert(jp_test_screen_show(&c, screen.pixels) == 0);
	c.framebuffer.width = 1024;
	c.framebuffer.height = 15;                       /* lower than one text row */
	assert(jp_test_screen_show(&c, screen.pixels) == 0);

	for (i = 0; i < screen.size + 2 * GUARD; i++) {
		assert(screen.memory[i] == SENTINEL);
	}
	free_screen(&screen);
}

static void test_a_list_that_does_not_fit_ends_with_a_notice(void)
{
	JPBootContext c;
	Screen screen;
	size_t total;
	size_t drawn;
	size_t rows = 4;
	size_t y;
	size_t x;
	size_t lit_in_last_row = 0;

	make_context(&c, 800, (uint32_t)(rows * JP_TEST_FONT_HEIGHT), 4096);
	total = list_lines(&c, NULL, 0);
	assert(total > rows);
	screen = make_screen(&c.framebuffer);
	drawn = jp_test_screen_show(&c, screen.pixels);
	assert(drawn == rows);                           /* rows - 1 lines and the notice */

	/* The last text row holds something: the notice starts with "(". */
	for (y = (rows - 1) * JP_TEST_FONT_HEIGHT; y < rows * JP_TEST_FONT_HEIGHT; y++) {
		for (x = 0; x < JP_TEST_FONT_WIDTH; x++) {
			if (pixel_lit(&screen, 4096, x, y)) {
				lit_in_last_row++;
			}
		}
	}
	assert(lit_in_last_row > 0);
	check_guards_untouched(&screen);
	free_screen(&screen);
}

static void test_two_columns_when_the_screen_is_wide_enough(void)
{
	JPBootContext c;
	Screen screen;
	size_t total;
	size_t rows;
	size_t drawn;
	size_t y;
	size_t x;
	bool second_column_used = false;

	make_context(&c, 1280, 1600, 5120);
	total = list_lines(&c, NULL, 0);
	rows = total / 2 + 1;                            /* too few rows for one column, enough for two */
	assert(total > rows && total < 2 * rows);
	make_context(&c, 1280, (uint32_t)(rows * JP_TEST_FONT_HEIGHT), 5120);   /* 2 columns of 640 px */
	screen = make_screen(&c.framebuffer);
	drawn = jp_test_screen_show(&c, screen.pixels);
	assert(drawn == total);                          /* everything fits in two columns */
	for (y = 0; y < rows * JP_TEST_FONT_HEIGHT; y++) {
		for (x = 640; x < 1280; x++) {
			if (pixel_lit(&screen, 5120, x, y)) {
				second_column_used = true;
			}
		}
	}
	assert(second_column_used);
	check_guards_untouched(&screen);
	free_screen(&screen);
}

int main(void)
{
	test_first_line_is_drawn_with_the_font();
	test_region_is_cleared_and_padding_is_untouched();
	test_nothing_is_drawn_without_a_usable_framebuffer();
	test_a_list_that_does_not_fit_ends_with_a_notice();
	test_two_columns_when_the_screen_is_wide_enough();
	return 0;
}
