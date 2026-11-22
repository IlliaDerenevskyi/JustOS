/*
 * boot_config.h - the boot configuration (Stage 4).
 *
 * Purpose:
 *   Sets up, checks, applies, freezes, and reads JPBootConfig, which lives in
 *   the stage4 area of the context (milestone0.md, "4 Boot Configuration
 *   Contract"; file specifications 1 and 2).
 *
 * Responsibilities:
 *   - jp_boot_config_initialize: the default values, UNSET to VALIDATED;
 *   - jp_boot_config_apply: a valid candidate replaces the editable contents
 *     (all or nothing), the source becomes INTERACTIVE;
 *   - jp_boot_config_freeze: VALIDATED to FROZEN (final);
 *   - jp_boot_config_get: a copy for the caller.
 *
 * Limitations:
 *   The entries are only stored, never interpreted. A frozen configuration is
 *   protected by the functions only, not by the memory.
 *
 * Depends on:
 *   justos/boot/boot_types.h (which includes stage4_types.h).
 *
 * Must not depend on:
 *   firmware, Stage 3, the other Stage 4 modules.
 */

#ifndef JUSTOS_BOOT_BOOT_CONFIG_H
#define JUSTOS_BOOT_BOOT_CONFIG_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_config_initialize(JPBootContext *context);
JPBootResult jp_boot_config_apply(JPBootContext *context, const JPBootConfig *candidate);
JPBootResult jp_boot_config_freeze(JPBootContext *context);
JPBootResult jp_boot_config_get(const JPBootContext *context, JPBootConfig *config);

#endif /* JUSTOS_BOOT_BOOT_CONFIG_H */
