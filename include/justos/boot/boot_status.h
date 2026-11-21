/*
 * boot_status.h - the boot status of Stage 3.
 *
 * Purpose:
 *   Keeps the one status of the current boot attempt and checks every change
 *   of it against the allowed transitions.
 *
 * Responsibilities:
 *   - jp_boot_status_initialize: status BOOTING, once;
 *   - jp_boot_status_update: BOOTING -> INITIALIZING -> RUNNING -> FAILED ->
 *     RECOVERY -> HALTED, with the exceptions of milestone0.md (a failure may
 *     come from any non-terminal status; HALTED is final);
 *   - jp_boot_status_get: reads the status.
 *
 * Limitations:
 *   Declarations only. A refused change leaves everything as it was.
 *
 * Public API:
 *   jp_boot_status_initialize, jp_boot_status_update, jp_boot_status_get.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo.
 */

#ifndef JUSTOS_BOOT_BOOT_STATUS_H
#define JUSTOS_BOOT_BOOT_STATUS_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_status_initialize(
	JPBootContext *context
);

JPBootResult jp_boot_status_update(
	JPBootContext *context,
	JPDiagnosticStage stage,
	JPBootStatus status
);

/* JP_BOOT_NOT_INITIALIZED before jp_boot_status_initialize(). */
JPBootResult jp_boot_status_get(
	const JPBootContext *context,
	JPBootStatusState *status
);

#endif /* JUSTOS_BOOT_BOOT_STATUS_H */
