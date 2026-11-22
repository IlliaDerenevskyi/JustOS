/*
 * kernel_loader.h - Stage 1 kernel-loader interface.
 *
 * Purpose:
 *   Declares generic ELF validation and kernel loading used by Stage 1.
 *
 * Responsibilities:
 *   - declare kernel-image validation;
 *   - declare kernel loading and mapping.
 *
 * Limitations:
 *   Declarations only. No provider structures, page-table internals or JPBootInfo.
 *
 * Public API:
 *   jp_boot_validate_kernel, jp_boot_load_kernel.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   UEFI, BIOS, page-table internals, JPBootInfo.
 */

#ifndef JUSTOS_BOOT_KERNEL_LOADER_H
#define JUSTOS_BOOT_KERNEL_LOADER_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_validate_kernel(
	JPBootContext *context
);

JPBootResult jp_boot_load_kernel(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_KERNEL_LOADER_H */
