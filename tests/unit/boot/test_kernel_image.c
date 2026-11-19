/*
 * Unit test (level U) of src/boot/kernel_image.c.
 * Rules: see "Decided rules (kernel image)" in milestone0.md.
 */

#include <assert.h>
#include <string.h>

#include "fake_environment.h"
#include "justos/boot/kernel_image.h"

static const unsigned char image_bytes[64] = { 1 };

static void prepare(JPBootContext *context, JPFakeEnvironment *fake)
{
	jp_fake_environment_attach(context, fake);
	fake->kernel_image.state = JP_DETECTION_STATE_UNKNOWN;
	fake->kernel_image.buffer = image_bytes;
	fake->kernel_image.size = sizeof image_bytes;
}

static void test_success(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	assert(jp_boot_acquire_kernel_image(&context) == JP_BOOT_SUCCESS);
	assert(fake.kernel_image_calls == 1);
	assert(context.current_stage == JP_BOOT_STAGE_KERNEL_IMAGE_ACQUISITION);
	assert(context.kernel.image.state == JP_DETECTION_STATE_DETECTED);
	assert(context.kernel.image.buffer == image_bytes);
	assert(context.kernel.image.size == sizeof image_bytes);
	assert(context.kernel.validation_state == JP_KERNEL_VALIDATION_NOT_INITIALIZED);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_NOT_INITIALIZED);
}

static void test_invalid_parameter(void)
{
	assert(jp_boot_acquire_kernel_image(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_provider_failures_are_unchanged(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	fake.kernel_image_result = JP_BOOT_NOT_DETECTED;
	assert(jp_boot_acquire_kernel_image(&context) == JP_BOOT_NOT_DETECTED);
	assert(context.kernel.image.state == JP_DETECTION_STATE_NOT_DETECTED);

	prepare(&context, &fake);
	fake.kernel_image_result = JP_BOOT_FAILED;
	assert(jp_boot_acquire_kernel_image(&context) == JP_BOOT_FAILED);
	assert(context.kernel.image.state == JP_DETECTION_STATE_FAILED);
	assert(context.kernel.validation_state != JP_KERNEL_VALIDATION_VALID);
}

static void test_invalid_storage(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	fake.kernel_image.buffer = NULL;
	assert(jp_boot_acquire_kernel_image(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.kernel.image.state == JP_DETECTION_STATE_FAILED);

	prepare(&context, &fake);
	fake.kernel_image.size = 0;
	assert(jp_boot_acquire_kernel_image(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.kernel.image.state == JP_DETECTION_STATE_FAILED);
}

int main(void)
{
	test_success();
	test_invalid_parameter();
	test_provider_failures_are_unchanged();
	test_invalid_storage();
	return 0;
}
