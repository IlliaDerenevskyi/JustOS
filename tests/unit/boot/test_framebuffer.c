/*
 * Unit test (level U) of src/boot/framebuffer.c.
 * Rules: see "Decided rules (framebuffer)" in milestone0.md.
 * RGB_888 and BGR_888 use 4 bytes per pixel.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "fake_environment.h"
#include "justos/boot/framebuffer.h"

enum { WIDTH = 800, HEIGHT = 600, BYTES_PER_PIXEL = 4 };

static void prepare(JPBootContext *context, JPFakeEnvironment *fake)
{
	jp_fake_environment_attach(context, fake);
	fake->framebuffer.base_address = 0x80000000ULL;
	fake->framebuffer.width = WIDTH;
	fake->framebuffer.height = HEIGHT;
	fake->framebuffer.pitch = WIDTH * BYTES_PER_PIXEL;
	fake->framebuffer.format = JP_FRAMEBUFFER_FORMAT_BGR_888;
}

static void test_success_for_both_formats(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_SUCCESS);
	assert(context.current_stage == JP_BOOT_STAGE_FRAMEBUFFER_INIT);
	assert(context.framebuffer.state == JP_DETECTION_STATE_DETECTED);
	assert(context.framebuffer.width == WIDTH && context.framebuffer.height == HEIGHT);
	assert(context.framebuffer.format == JP_FRAMEBUFFER_FORMAT_BGR_888);
	assert(jp_boot_validate_framebuffer(&context) == JP_BOOT_SUCCESS);

	prepare(&context, &fake);
	fake.framebuffer.format = JP_FRAMEBUFFER_FORMAT_RGB_888;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_validate_framebuffer(&context) == JP_BOOT_SUCCESS);
}

static void test_pitch_larger_than_row_is_valid(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	fake.framebuffer.pitch = (WIDTH + 64) * BYTES_PER_PIXEL;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_SUCCESS);
}

static void test_provider_failures(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	assert(jp_boot_initialize_framebuffer(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context, &fake);
	fake.framebuffer_result = JP_BOOT_NOT_DETECTED;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_NOT_DETECTED);
	assert(context.framebuffer.state == JP_DETECTION_STATE_NOT_DETECTED);

	prepare(&context, &fake);
	fake.framebuffer_result = JP_BOOT_FAILED;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_FAILED);
	assert(context.framebuffer.state == JP_DETECTION_STATE_FAILED);

	/* An unknown format from the provider is a provider failure. */
	prepare(&context, &fake);
	fake.framebuffer.format = JP_FRAMEBUFFER_FORMAT_UNKNOWN;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_FAILED);
	assert(context.framebuffer.state == JP_DETECTION_STATE_FAILED);
}

#define EXPECT_INVALID(context, fake) \
	do { \
		assert(jp_boot_initialize_framebuffer(&(context)) == JP_BOOT_ERROR_CORRUPTED_DATA); \
		assert((context).framebuffer.state == JP_DETECTION_STATE_FAILED); \
	} while (0)

static void test_invalid_configurations(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	fake.framebuffer.base_address = 0;
	EXPECT_INVALID(context, fake);

	prepare(&context, &fake);
	fake.framebuffer.width = 0;
	EXPECT_INVALID(context, fake);

	prepare(&context, &fake);
	fake.framebuffer.height = 0;
	EXPECT_INVALID(context, fake);

	prepare(&context, &fake);
	fake.framebuffer.pitch = WIDTH * BYTES_PER_PIXEL - 4;
	EXPECT_INVALID(context, fake);

	prepare(&context, &fake);
	fake.framebuffer.pitch = WIDTH * BYTES_PER_PIXEL + 2;
	EXPECT_INVALID(context, fake);

	/* width * 4 does not fit in 32 bits: it must not wrap around to 0. */
	prepare(&context, &fake);
	fake.framebuffer.width = 0x40000000U;
	fake.framebuffer.pitch = UINT32_MAX - 3;
	EXPECT_INVALID(context, fake);

	/* The largest consistent values are valid (32-bit fields; nothing wraps around). */
	prepare(&context, &fake);
	fake.framebuffer.width = 4096;
	fake.framebuffer.height = UINT32_MAX;
	fake.framebuffer.pitch = UINT32_MAX - 3;
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_SUCCESS);
}

static void test_validate_requires_acquisition(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	assert(jp_boot_validate_framebuffer(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	prepare(&context, &fake);
	assert(jp_boot_validate_framebuffer(&context) == JP_BOOT_NOT_INITIALIZED);
}

static void test_validate_detects_later_corruption(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	assert(jp_boot_initialize_framebuffer(&context) == JP_BOOT_SUCCESS);
	context.framebuffer.pitch = 8;
	assert(jp_boot_validate_framebuffer(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.framebuffer.state == JP_DETECTION_STATE_FAILED);
}

int main(void)
{
	test_success_for_both_formats();
	test_pitch_larger_than_row_is_valid();
	test_provider_failures();
	test_invalid_configurations();
	test_validate_requires_acquisition();
	test_validate_detects_later_corruption();
	return 0;
}
