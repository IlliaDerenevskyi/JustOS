/* Unit test (level U) of the generated 8x16 font of the test kernel. */

#include <assert.h>
#include <string.h>

#include "font.h"

static int lit_pixels(unsigned char c)
{
	int count = 0;
	size_t y;
	int x;

	for (y = 0; y < JP_TEST_FONT_HEIGHT; y++) {
		unsigned int row = jp_test_font8x16[c - JP_TEST_FONT_FIRST_CHAR][y];

		for (x = 0; x < JP_TEST_FONT_WIDTH; x++) {
			if ((row >> x) & 1U) {
				count++;
			}
		}
	}
	return count;
}

static void test_dimensions(void)
{
	assert(JP_TEST_FONT_WIDTH == 8);
	assert(JP_TEST_FONT_HEIGHT == 16);
	assert(JP_TEST_FONT_GLYPH_COUNT == 95);
}

static void test_space_is_blank_and_every_other_glyph_is_drawn(void)
{
	unsigned char c;

	assert(lit_pixels(' ') == 0);
	for (c = JP_TEST_FONT_FIRST_CHAR + 1; c <= JP_TEST_FONT_LAST_CHAR; c++) {
		assert(lit_pixels(c) > 0);
	}
}

static void test_glyphs_are_distinct(void)
{
	unsigned char a;
	unsigned char b;

	for (a = JP_TEST_FONT_FIRST_CHAR; a <= JP_TEST_FONT_LAST_CHAR; a++) {
		for (b = (unsigned char)(a + 1); b <= JP_TEST_FONT_LAST_CHAR; b++) {
			assert(memcmp(jp_test_font8x16[a - JP_TEST_FONT_FIRST_CHAR],
				jp_test_font8x16[b - JP_TEST_FONT_FIRST_CHAR],
				JP_TEST_FONT_HEIGHT) != 0);
		}
	}
}

static void test_glyph_stays_inside_the_cell_rows(void)
{
	/* Capital letters have no pixel in the first and in the last row. */
	unsigned char c;

	for (c = 'A'; c <= 'Z'; c++) {
		assert(jp_test_font8x16[c - JP_TEST_FONT_FIRST_CHAR][0] == 0);
		assert(jp_test_font8x16[c - JP_TEST_FONT_FIRST_CHAR][JP_TEST_FONT_HEIGHT - 1] == 0);
	}
}

int main(void)
{
	test_dimensions();
	test_space_is_blank_and_every_other_glyph_is_drawn();
	test_glyphs_are_distinct();
	test_glyph_stays_inside_the_cell_rows();
	return 0;
}
