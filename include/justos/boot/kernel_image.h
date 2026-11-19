/*
 * kernel_image.h - kernel image acquisition contract.
 *
 * Purpose:
 *   Normalized kernel image acquisition interface.
 *
 * Responsibilities:
 *   - acquire the kernel image;
 *   - validate image storage.
 *
 * Limitations:
 *   Declarations only. Storage mechanism is provider-defined.
 *
 * Public API:
 *   jp_boot_acquire_kernel_image.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Provider-native types.
 */

#ifndef JUSTOS_BOOT_KERNEL_IMAGE_H
#define JUSTOS_BOOT_KERNEL_IMAGE_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_acquire_kernel_image(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_KERNEL_IMAGE_H */
