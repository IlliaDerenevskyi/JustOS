/*
 * boot_environment.h - generic boot-environment interface.
 *
 * Purpose:
 *   Platform-independent interface between generic Stage 1 logic and providers.
 *
 * Responsibilities:
 *   - initialize the selected boot environment;
 *   - expose normalized provider operations;
 *   - keep provider-private state opaque;
 *   - dispatch normalized provider input while provider services are valid.
 *
 * Limitations:
 *   Declarations only. Input is permitted only while the provider is INITIALIZED or ACTIVE; otherwise JP_BOOT_NOT_INITIALIZED is returned.
 *
 * Public API:
 *   jp_boot_environment_initialize, _get_memory_map, _get_framebuffer,
 *   _get_kernel_image, _prepare_handoff, _read_input, _reserve_memory.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   UEFI or BIOS native structures.
 */

#ifndef JUSTOS_BOOT_BOOT_ENVIRONMENT_H
#define JUSTOS_BOOT_BOOT_ENVIRONMENT_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_environment_initialize(
	JPBootContext *context
);

JPBootResult jp_boot_environment_get_memory_map(
	JPBootContext *context
);

JPBootResult jp_boot_environment_get_framebuffer(
	JPBootContext *context
);

JPBootResult jp_boot_environment_get_kernel_image(
	JPBootContext *context
);

JPBootResult jp_boot_environment_prepare_handoff(
	JPBootContext *context
);

/* Returns the provider result unchanged; follows the pre-transition interaction contract. */
JPBootResult jp_boot_environment_read_input(
	JPBootContext *context,
	JPBootInputEvent *event
);

/*
 * Tells the provider that a physical range now belongs to the kernel
 * execution environment (rounded outward to whole pages). Succeeds only after
 * the reservation really happened.
 */
JPBootResult jp_boot_environment_reserve_memory(
	JPBootContext *context,
	uint64_t physical,
	uint64_t length
);

#endif /* JUSTOS_BOOT_BOOT_ENVIRONMENT_H */
