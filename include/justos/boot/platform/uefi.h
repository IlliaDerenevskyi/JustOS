/*
 * uefi.h - UEFI provider contract.
 *
 * Purpose:
 *   Defines the UEFI-specific provider interface used by generic Stage 1.
 *
 * Responsibilities:
 *   - initialization;
 *   - memory-map, framebuffer and kernel image acquisition;
 *   - non-blocking input before the platform transition;
 *   - final handoff preparation.
 *
 * Limitations:
 *   Declarations only. JPUefiContext is provider-owned and its layout is private to
 *   the provider implementation; generic code never looks inside it.
 *   Input is permitted only before the transition (state INITIALIZED or
 *   ACTIVE); later it returns JP_BOOT_NOT_INITIALIZED. No event is fabricated.
 *
 * Public API:
 *   jp_uefi_initialize, _get_memory_map, _get_framebuffer, _get_kernel_image,
 *   _prepare_handoff, _read_input, _reserve_memory; jp_uefi_context_size and
 *   jp_uefi_context_setup, jp_uefi_boot_services_active (used by the UEFI entry
 *   point).
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   UEFI-native structures in generic Stage 1 (they do not escape the provider).
 */

#ifndef JUSTOS_BOOT_PLATFORM_UEFI_H
#define JUSTOS_BOOT_PLATFORM_UEFI_H

#include <stddef.h>

#include "justos/boot/boot_types.h"

/* Provider-owned context; the layout is private to the provider. */
typedef struct JPUefiContext JPUefiContext;

JPBootResult jp_uefi_initialize(
	JPUefiContext *context
);

JPBootResult jp_uefi_get_memory_map(
	JPUefiContext *context,
	JPMemoryMap *memory_map
);

JPBootResult jp_uefi_get_framebuffer(
	JPUefiContext *context,
	JPFramebufferInfo *framebuffer
);

JPBootResult jp_uefi_get_kernel_image(
	JPUefiContext *context,
	JPKernelImage *image
);

JPBootResult jp_uefi_prepare_handoff(
	JPUefiContext *context
);

JPBootResult jp_uefi_read_input(
	JPUefiContext *context,
	JPBootInputEvent *event
);

JPBootResult jp_uefi_reserve_memory(
	JPUefiContext *context,
	uint64_t physical,
	uint64_t length
);

/* The number of bytes the provider context needs (the entry point allocates them). */
size_t jp_uefi_context_size(void);

/*
 * Fills zeroed memory of that size with the image handle and the system table
 * of the firmware (both passed as opaque pointers, so no UEFI type appears
 * here) and returns the context; a null pointer for null arguments.
 */
JPUefiContext *jp_uefi_context_setup(
	void *memory,
	void *image_handle,
	void *system_table
);

/* True while the firmware services can still be used (after initialize, until the exit succeeded). */
bool jp_uefi_boot_services_active(
	const JPUefiContext *context
);

#endif /* JUSTOS_BOOT_PLATFORM_UEFI_H */
