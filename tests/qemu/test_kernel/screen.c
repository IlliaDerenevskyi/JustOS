#include "screen.h"

#include "font.h"
#include "report.h"

enum {
	JP_SCREEN_BYTES_PER_PIXEL = 4,
	JP_SCREEN_LIT = 0xFF,
	JP_SCREEN_DARK = 0x00,
	JP_SCREEN_DECIMAL_BASE = 10,
	JP_SCREEN_NUMBER_DIGITS = 20
};

/* Counts the lines of the list. */
typedef struct CountSink {
	size_t lines;
} CountSink;

/* Draws the lines of the list; the line buffer holds the current line. */
typedef struct DrawSink {
	uint8_t *pixels;
	uint32_t pitch;
	size_t rows;                 /* text rows in one column */
	size_t columns;              /* text columns on the screen */
	size_t capacity;             /* rows * columns */
	size_t last_line;            /* index of the last line that may be drawn */
	size_t omitted;              /* lines left out when the list does not fit */
	size_t line_index;           /* index of the line being collected */
	size_t drawn;                /* lines drawn so far */
	char line[JP_TEST_SCREEN_COLUMN_CHARS + 1];
	size_t length;
} DrawSink;

static void count_put(void *user, char c)
{
	CountSink *sink = (CountSink *)user;

	if (c == '\n') {
		sink->lines++;
	}
}

static void draw_glyph(const DrawSink *sink, size_t column, size_t row, size_t cell, char c)
{
	size_t glyph;
	size_t y;
	size_t x;
	uint8_t *origin;

	if (c < JP_TEST_FONT_FIRST_CHAR || c > JP_TEST_FONT_LAST_CHAR) {
		c = '?';
	}
	glyph = (size_t)(c - JP_TEST_FONT_FIRST_CHAR);
	origin = sink->pixels
		+ (row * JP_TEST_FONT_HEIGHT) * (size_t)sink->pitch
		+ (column * JP_TEST_SCREEN_COLUMN_CHARS + cell) * JP_TEST_FONT_WIDTH * JP_SCREEN_BYTES_PER_PIXEL;
	for (y = 0; y < JP_TEST_FONT_HEIGHT; y++) {
		uint8_t bits = jp_test_font8x16[glyph][y];
		uint8_t *line = origin + y * (size_t)sink->pitch;

		for (x = 0; x < JP_TEST_FONT_WIDTH; x++) {
			uint8_t value = ((bits >> (JP_TEST_FONT_WIDTH - 1 - x)) & 1U) != 0 ? JP_SCREEN_LIT : JP_SCREEN_DARK;
			uint8_t *pixel = line + x * JP_SCREEN_BYTES_PER_PIXEL;

			pixel[0] = value;
			pixel[1] = value;
			pixel[2] = value;
			pixel[3] = JP_SCREEN_DARK;
		}
	}
}

static void draw_text(DrawSink *sink, size_t slot, const char *text, size_t length)
{
	size_t column = slot / sink->rows;
	size_t row = slot % sink->rows;
	size_t i;

	for (i = 0; i < length; i++) {
		draw_glyph(sink, column, row, i, text[i]);
	}
	sink->drawn++;
}

/* Draws "(N more lines on serial)". */
static void draw_omitted_notice(DrawSink *sink, size_t slot)
{
	static const char head[] = "(";
	static const char tail[] = " more lines on serial)";
	char text[JP_TEST_SCREEN_COLUMN_CHARS];
	char digits[JP_SCREEN_NUMBER_DIGITS];
	size_t count = 0;
	size_t n = sink->omitted;
	size_t length = 0;
	size_t i;

	do {
		digits[count] = (char)('0' + (n % JP_SCREEN_DECIMAL_BASE));
		n /= JP_SCREEN_DECIMAL_BASE;
		count++;
	} while (n != 0);
	for (i = 0; head[i] != '\0'; i++) {
		text[length++] = head[i];
	}
	while (count > 0) {
		count--;
		text[length++] = digits[count];
	}
	for (i = 0; tail[i] != '\0'; i++) {
		text[length++] = tail[i];
	}
	draw_text(sink, slot, text, length);
}

static void draw_put(void *user, char c)
{
	DrawSink *sink = (DrawSink *)user;

	if (c != '\n') {
		if (sink->length < JP_TEST_SCREEN_COLUMN_CHARS) {
			sink->line[sink->length] = c;
			sink->length++;
		}
		return;
	}
	if (sink->omitted == 0 || sink->line_index < sink->last_line) {
		draw_text(sink, sink->line_index, sink->line, sink->length);
	} else if (sink->line_index == sink->last_line) {
		draw_omitted_notice(sink, sink->last_line);
	}
	sink->line_index++;
	sink->length = 0;
}

static void clear_region(uint8_t *pixels, uint32_t pitch, uint32_t width, uint32_t height)
{
	uint32_t y;
	size_t x;

	for (y = 0; y < height; y++) {
		uint8_t *line = pixels + (size_t)y * pitch;

		for (x = 0; x < (size_t)width * JP_SCREEN_BYTES_PER_PIXEL; x++) {
			line[x] = JP_SCREEN_DARK;
		}
	}
}

size_t jp_test_screen_show(const JPBootContext *context, void *pixels)
{
	const JPFramebufferInfo *fb = &context->framebuffer;
	CountSink counter;
	JPTestSink count_sink = { count_put, &counter };
	DrawSink draw;
	JPTestSink draw_sink = { draw_put, &draw };
	size_t rows;
	size_t columns;

	if (pixels == NULL || fb->state != JP_DETECTION_STATE_DETECTED) {
		return 0;
	}
	if (fb->format != JP_FRAMEBUFFER_FORMAT_RGB_888 && fb->format != JP_FRAMEBUFFER_FORMAT_BGR_888) {
		return 0;
	}
	rows = fb->height / JP_TEST_FONT_HEIGHT;
	columns = fb->width / (JP_TEST_FONT_WIDTH * JP_TEST_SCREEN_COLUMN_CHARS);
	if (rows == 0 || columns == 0) {
		return 0;
	}

	counter.lines = 0;
	(void)jp_test_report_emit(context, &count_sink);

	draw.pixels = (uint8_t *)pixels;
	draw.pitch = fb->pitch;
	draw.rows = rows;
	draw.columns = columns;
	draw.capacity = rows * columns;
	draw.line_index = 0;
	draw.drawn = 0;
	draw.length = 0;
	if (counter.lines > draw.capacity) {
		draw.last_line = draw.capacity - 1;
		draw.omitted = counter.lines - draw.last_line;
	} else {
		draw.last_line = counter.lines;
		draw.omitted = 0;
	}

	clear_region(draw.pixels, fb->pitch, fb->width, fb->height);
	(void)jp_test_report_emit(context, &draw_sink);
	return draw.drawn;
}
