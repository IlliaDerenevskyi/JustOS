/*
 * handoff.h - final Stage 1 handoff contract.
 *
 * Purpose:
 *   Validates and prepares the Stage 1 to Stage 2 transition.
 *
 * Responsibilities:
 *   - register the optional pre-transition interaction hook;
 *   - validate prerequisites, run the hook, prepare the platform handoff.
 *
 * Limitations:
 *   Declarations only. JPBootInfo is not defined. At most one hook is recorded; registration alone does not run it.
 *
 * Public API:
 *   jp_boot_register_interaction, jp_boot_prepare_handoff.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   JPBootInfo.
 */

#ifndef JUSTOS_BOOT_HANDOFF_H
#define JUSTOS_BOOT_HANDOFF_H

#include "justos/boot/boot_types.h"

/*
 * Fails with JP_BOOT_ERROR_INVALID_PARAM for a null context or hook, and with
 * JP_BOOT_FAILED if a hook is already registered or preparation has started.
 */
JPBootResult jp_boot_register_interaction(
	JPBootContext *context,
	JPBootInteractionHook hook,
	void *user_data
);

JPBootResult jp_boot_prepare_handoff(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_HANDOFF_H */
