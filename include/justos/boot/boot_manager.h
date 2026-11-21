/*
 * boot_manager.h - the part of the Stage 4 Boot Manager that Stage 3 needs.
 *
 * Purpose:
 *   The recovery transition: the one function through which Stage 3 reaches
 *   Stage 4. The names are binding (milestone0.md, "3 Recovery Transition
 *   Boundary").
 *
 * Limitations:
 *   Only this boundary is declared here. Stage 4 adds the rest of the header
 *   (the manager state, prepare, select, decide, and the others) when it is
 *   written, and implements the function.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 */

#ifndef JUSTOS_BOOT_BOOT_MANAGER_H
#define JUSTOS_BOOT_BOOT_MANAGER_H

#include "justos/boot/boot_types.h"

typedef enum JPBootTransitionResult {
	JP_BOOT_TRANSITION_ACCEPTED,
	JP_BOOT_TRANSITION_REJECTED,
	JP_BOOT_TRANSITION_TERMINAL
} JPBootTransitionResult;

JPBootTransitionResult jp_boot_manager_request_recovery(
	JPBootContext *context,
	const JPBootRecoveryState *recovery
);

#endif /* JUSTOS_BOOT_BOOT_MANAGER_H */
