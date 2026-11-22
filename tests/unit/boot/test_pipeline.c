/*
 * Unit test (level U) of src/boot/boot_pipeline.c.
 * The nine public Stage 1 functions are replaced by local fakes that log the
 * order of the calls and return configured results.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_environment.h"
#include "justos/boot/boot_pipeline.h"
#include "justos/boot/framebuffer.h"
#include "justos/boot/handoff.h"
#include "justos/boot/hardware.h"
#include "justos/boot/kernel_image.h"
#include "justos/boot/kernel_loader.h"
#include "justos/boot/memory_map.h"

enum {
	STEP_ENV = 1, STEP_HARDWARE, STEP_MEMORY, STEP_VALIDATE_MEMORY,
	STEP_FRAMEBUFFER, STEP_IMAGE, STEP_VALIDATE_KERNEL, STEP_LOAD, STEP_HANDOFF,
	STEP_COUNT = STEP_HANDOFF
};

static int call_log[32];
static size_t call_count;
static JPBootResult results[STEP_COUNT + 1];
static const JPBootContext *seen_context[STEP_COUNT + 1];

static JPBootResult step(int id, const JPBootContext *context)
{
	assert(call_count < sizeof call_log / sizeof call_log[0]);
	call_log[call_count++] = id;
	seen_context[id] = context;
	return results[id];
}

JPBootResult jp_boot_environment_initialize(JPBootContext *c) { return step(STEP_ENV, c); }
JPBootResult jp_boot_discover_hardware(JPBootContext *c) { return step(STEP_HARDWARE, c); }
JPBootResult jp_boot_discover_memory(JPBootContext *c) { return step(STEP_MEMORY, c); }
JPBootResult jp_boot_validate_memory_map(JPBootContext *c) { return step(STEP_VALIDATE_MEMORY, c); }
JPBootResult jp_boot_initialize_framebuffer(JPBootContext *c) { return step(STEP_FRAMEBUFFER, c); }
JPBootResult jp_boot_acquire_kernel_image(JPBootContext *c) { return step(STEP_IMAGE, c); }
JPBootResult jp_boot_validate_kernel(JPBootContext *c) { return step(STEP_VALIDATE_KERNEL, c); }
JPBootResult jp_boot_load_kernel(JPBootContext *c) { return step(STEP_LOAD, c); }
JPBootResult jp_boot_prepare_handoff(JPBootContext *c) { return step(STEP_HANDOFF, c); }

static void reset(void)
{
	memset(call_log, 0, sizeof call_log);
	memset(results, 0, sizeof results);
	memset(seen_context, 0, sizeof seen_context);
	call_count = 0;
}

static void test_all_steps_run_in_order(void)
{
	JPBootContext context;
	int step_id;

	reset();
	memset(&context, 0, sizeof context);
	assert(jp_boot_run_pipeline(&context) == JP_BOOT_SUCCESS);
	assert(call_count == STEP_COUNT);
	for (step_id = 1; step_id <= STEP_COUNT; step_id++) {
		assert(call_log[step_id - 1] == step_id);
		assert(seen_context[step_id] == &context);   /* every step gets the same context */
	}
}

static void test_each_failure_stops_the_pipeline_with_its_own_result(void)
{
	JPBootContext context;
	int failing;

	for (failing = 1; failing <= STEP_COUNT; failing++) {
		if (failing == STEP_FRAMEBUFFER) {
			continue;                                   /* tested separately: it has an exception */
		}
		reset();
		memset(&context, 0, sizeof context);
		results[failing] = JP_BOOT_ERROR_CORRUPTED_DATA;
		assert(jp_boot_run_pipeline(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
		assert(call_count == (size_t)failing);            /* nothing after the failing step ran */
		assert(call_log[call_count - 1] == failing);
	}
}

static void test_framebuffer_rules(void)
{
	JPBootContext context;

	/* No framebuffer is not an error: the pipeline goes on. */
	reset();
	memset(&context, 0, sizeof context);
	results[STEP_FRAMEBUFFER] = JP_BOOT_NOT_DETECTED;
	assert(jp_boot_run_pipeline(&context) == JP_BOOT_SUCCESS);
	assert(call_count == STEP_COUNT);

	/* Any other framebuffer failure stops it, with its own result. */
	reset();
	results[STEP_FRAMEBUFFER] = JP_BOOT_FAILED;
	assert(jp_boot_run_pipeline(&context) == JP_BOOT_FAILED);
	assert(call_count == STEP_FRAMEBUFFER);

	reset();
	results[STEP_FRAMEBUFFER] = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_run_pipeline(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(call_count == STEP_FRAMEBUFFER);
}

static void test_results_are_not_converted(void)
{
	JPBootContext context;
	static const JPBootResult codes[] = {
		JP_BOOT_NOT_INITIALIZED, JP_BOOT_NOT_DETECTED, JP_BOOT_UNKNOWN,
		JP_BOOT_FAILED, JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_ERROR_CORRUPTED_DATA
	};
	size_t i;

	for (i = 0; i < sizeof codes / sizeof codes[0]; i++) {
		reset();
		memset(&context, 0, sizeof context);
		results[STEP_HARDWARE] = codes[i];
		assert(jp_boot_run_pipeline(&context) == codes[i]);
	}
}

static void test_null_context(void)
{
	reset();
	assert(jp_boot_run_pipeline(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(call_count == 0);
}

static void test_pipeline_does_not_write_the_context(void)
{
	JPBootContext context;
	JPBootContext before;

	reset();
	memset(&context, 0xAB, sizeof context);
	before = context;
	assert(jp_boot_run_pipeline(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &context, sizeof context) == 0);
}

int main(void)
{
	test_all_steps_run_in_order();
	test_each_failure_stops_the_pipeline_with_its_own_result();
	test_framebuffer_rules();
	test_results_are_not_converted();
	test_null_context();
	test_pipeline_does_not_write_the_context();
	return 0;
}
