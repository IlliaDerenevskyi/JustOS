/*
 * bios.h - BIOS provider contract.
 *
 * Purpose:
 *   Defines the BIOS-specific provider interface used by generic Stage 1.
 *
 * Responsibilities:
 *   - initialization;
 *   - memory-map, framebuffer and kernel image acquisition;
 *   - non-blocking input before the platform transition;
 *   - final handoff preparation.
 *
 * Limitations:
 *   Declarations only. JPBiosContext is provider-owned and its layout is private to
 *   the provider implementation; generic code never looks inside it.
 *   Input is permitted only before the transition (state INITIALIZED or
 *   ACTIVE); later it returns JP_BOOT_NOT_INITIALIZED. No event is fabricated.
 *
 * Public API:
 *   jp_bios_initialize, _get_memory_map, _get_framebuffer, _get_kernel_image,
 *   _prepare_handoff, _read_input, _reserve_memory.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   BIOS-native structures in generic Stage 1 (they do not escape the provider).
 */

#ifndef JUSTOS_BOOT_PLATFORM_BIOS_H
#define JUSTOS_BOOT_PLATFORM_BIOS_H

#include <stddef.h>

#include "justos/boot/boot_types.h"

/* Provider-owned context; the layout is private to the provider. */
typedef struct JPBiosContext JPBiosContext;

JPBootResult jp_bios_initialize(
	JPBiosContext *context
);

JPBootResult jp_bios_get_memory_map(
	JPBiosContext *context,
	JPMemoryMap *memory_map
);

JPBootResult jp_bios_get_framebuffer(
	JPBiosContext *context,
	JPFramebufferInfo *framebuffer
);

JPBootResult jp_bios_get_kernel_image(
	JPBiosContext *context,
	JPKernelImage *image
);

JPBootResult jp_bios_prepare_handoff(
	JPBiosContext *context
);

JPBootResult jp_bios_read_input(
	JPBiosContext *context,
	JPBootInputEvent *event
);

JPBootResult jp_bios_reserve_memory(
	JPBiosContext *context,
	uint64_t physical,
	uint64_t length
);

/* The number of bytes the provider context needs (the entry point gives it zeroed memory). */
size_t jp_bios_context_size(void);

/*
 * Fills zeroed memory of that size with the pointer to the boot data block that
 * the loader left (an opaque pointer, so no BIOS type appears here) and returns
 * the context; a null pointer for null arguments.
 */
JPBiosContext *jp_bios_context_setup(
	void *memory,
	const void *block
);

#endif /* JUSTOS_BOOT_PLATFORM_BIOS_H */
