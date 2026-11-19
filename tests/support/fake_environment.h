/*
 * fake_environment.h - fake of the generic boot-environment layer.
 *
 * Test support only. It implements the jp_boot_environment_* functions so
 * that memory-map, framebuffer, and kernel-image modules can be tested
 * without a provider. The test points context->environment.platform_context
 * at a JPFakeEnvironment. The fake returns exactly the data the test put in
 * it and never invents anything.
 */

#ifndef JUSTOS_TESTS_SUPPORT_FAKE_ENVIRONMENT_H
#define JUSTOS_TESTS_SUPPORT_FAKE_ENVIRONMENT_H

#include "justos/boot/boot_environment.h"
#include "justos/boot/boot_types.h"

#define JP_FAKE_ENV_RESERVE_LOG 32

typedef struct JPFakeEnvironment {
	JPBootResult memory_map_result;
	JPBootResult framebuffer_result;
	JPBootResult kernel_image_result;
	JPBootResult prepare_handoff_result;
	JPBootResult reserve_result;
	unsigned int reserve_refusals;            /* the first N reservations answer reserve_refusal_result */
	JPBootResult reserve_refusal_result;
	JPMemoryMap memory_map;
	JPFramebufferInfo framebuffer;
	JPKernelImage kernel_image;
	/* When use_final_memory_map is true, prepare_handoff publishes this map. */
	JPMemoryMap final_memory_map;
	bool use_final_memory_map;

	unsigned int memory_map_calls;
	unsigned int framebuffer_calls;
	unsigned int kernel_image_calls;
	unsigned int prepare_handoff_calls;
	unsigned int reserve_calls;
	uint64_t reserved_base[JP_FAKE_ENV_RESERVE_LOG];
	uint64_t reserved_length[JP_FAKE_ENV_RESERVE_LOG];
	/* The state of kernel segment i when the i-th reservation was requested. */
	JPKernelSegmentState segment_state_at_reserve[JP_FAKE_ENV_RESERVE_LOG];
	/* How many calls came before prepare_handoff (to check the order). */
	unsigned int reserve_calls_before_prepare;
} JPFakeEnvironment;

/* Zero-initializes the fake and the context and connects them. */
void jp_fake_environment_attach(JPBootContext *context, JPFakeEnvironment *fake);

#endif /* JUSTOS_TESTS_SUPPORT_FAKE_ENVIRONMENT_H */
