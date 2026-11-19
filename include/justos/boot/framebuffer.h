/*
 * framebuffer.h - normalized framebuffer contract.
 *
 * Purpose:
 *   Normalized framebuffer interface.
 *
 * Responsibilities:
 *   - acquire framebuffer information;
 *   - validate framebuffer parameters.
 *
 * Limitations:
 *   Declarations only. Provider-specific formats are normalized before use.
 *
 * Public API:
 *   jp_boot_initialize_framebuffer, jp_boot_validate_framebuffer.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Provider-native types.
 */

#ifndef JUSTOS_BOOT_FRAMEBUFFER_H
#define JUSTOS_BOOT_FRAMEBUFFER_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_initialize_framebuffer(
	JPBootContext *context
);

JPBootResult jp_boot_validate_framebuffer(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_FRAMEBUFFER_H */
