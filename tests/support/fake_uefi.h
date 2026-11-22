/*
 * fake_uefi.h - fake UEFI provider for host tests.
 *
 * Test support only (fake_ prefix, Rule: never linked into production). It
 * defines JPUefiContext and implements the jp_uefi_* functions declared in
 * justos/boot/platform/uefi.h, so a test links it instead of the real
 * src/boot/platform/uefi.c. Behavior is configured by the test; the fake
 * never invents data: it returns exactly what the test supplied.
 *
 * Input rule of the contract: input works in INITIALIZED and ACTIVE only;
 * after jp_uefi_prepare_handoff() it returns JP_BOOT_NOT_INITIALIZED.
 */

#ifndef JUSTOS_TESTS_SUPPORT_FAKE_UEFI_H
#define JUSTOS_TESTS_SUPPORT_FAKE_UEFI_H

#include "fake_provider_state.h"
#include "justos/boot/boot_types.h"
#include "justos/boot/platform/uefi.h"


struct JPUefiContext {
	/* Configuration (set by the test). */
	JPBootResult initialize_result;
	JPBootResult memory_map_result;
	JPBootResult framebuffer_result;
	JPBootResult kernel_image_result;
	JPBootResult prepare_handoff_result;
	JPBootResult reserve_result;
	JPBootResult final_map_result;       /* get_memory_map after prepare_handoff */
	JPBootResult input_empty_result;     /* returned when the queue is empty */
	JPMemoryMap memory_map;
	JPMemoryMap final_memory_map;        /* returned by get_memory_map once the handoff is prepared */
	JPFramebufferInfo framebuffer;
	JPKernelImage kernel_image;
	JPBootInputEvent input_queue[JP_FAKE_INPUT_QUEUE_CAPACITY];
	size_t input_count;

	/* Observation (written by the fake). */
	char prepare_called_by;              /* the tag of the fake that ran prepare_handoff */
	char called_by;                      /* 'U' (UEFI fake) or 'B' (BIOS fake): which fake ran last */
	JPFakeProviderState state;
	size_t input_next;
	unsigned int initialize_calls;
	unsigned int memory_map_calls;
	unsigned int framebuffer_calls;
	unsigned int kernel_image_calls;
	unsigned int prepare_handoff_calls;
	unsigned int reserve_calls;
	uint64_t reserved_base[JP_FAKE_RESERVE_LOG];
	uint64_t reserved_length[JP_FAKE_RESERVE_LOG];
	unsigned int input_calls;
};

/* Clears the context; every operation succeeds and the input queue is empty (JP_BOOT_SUCCESS with kind NONE). */
void jp_fake_uefi_reset(JPUefiContext *context);

/* Appends one event to the input queue; returns 0 on success, -1 if full. */
int jp_fake_uefi_push_input(JPUefiContext *context, JPBootInputEvent event);

#endif /* JUSTOS_TESTS_SUPPORT_FAKE_UEFI_H */
