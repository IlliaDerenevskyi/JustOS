/*
 * boot_manager.h - the Stage 4 Boot Manager.
 *
 * Purpose:
 *   Runs Stage 4: the preparation and the menu of phase A, the completion
 *   after Stage 2 (jp_kernel_stage2_complete is declared in kernel/init.h and
 *   implemented in boot_manager.c), the decision, and the recovery
 *   transition that Stage 3 asks for (milestone0.md, "4 Boot Manager
 *   Decision Contract", "4 Recovery Transition Owner Contract"; file
 *   specifications 7 and 8).
 *
 * Limitations:
 *   The functions of phase A (prepare, select) exist only in the boot
 *   program; decide and request_recovery only in the kernel image
 *   ("4 Decided rules"). The Stage 4 types are in stage4_types.h (included by
 *   boot_types.h).
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 */

#ifndef JUSTOS_BOOT_BOOT_MANAGER_H
#define JUSTOS_BOOT_BOOT_MANAGER_H

#include "justos/boot/boot_types.h"

typedef enum JPBootSelectionPolicy {
	JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY = 0,   /* never show the menu */
	JP_BOOT_SELECTION_POLICY_INTERACTIVE         /* show the menu */
} JPBootSelectionPolicy;

/* Chosen when the program is built (-DJP_BOOT_SELECTION_POLICY=...); without a choice the menu is never shown. */
#ifndef JP_BOOT_SELECTION_POLICY
#define JP_BOOT_SELECTION_POLICY JP_BOOT_SELECTION_POLICY_DEFAULT_ONLY
#endif

typedef enum JPBootTransitionResult {
	JP_BOOT_TRANSITION_ACCEPTED,     /* the request was accepted and the destination started */
	JP_BOOT_TRANSITION_REJECTED,     /* the request was invalid; nothing changed */
	JP_BOOT_TRANSITION_TERMINAL      /* there is no safe destination */
} JPBootTransitionResult;

JPBootResult jp_boot_manager_prepare(JPBootContext *context);
JPBootResult jp_boot_manager_select(JPBootContext *context, void *user_data);
JPBootResult jp_boot_manager_decide(const JPBootContext *context, JPBootDecision *decision);
JPBootTransitionResult jp_boot_manager_request_recovery(
	JPBootContext *context,
	const JPBootRecoveryState *recovery
);

/* Declared here because Stage 4 calls it; the kernel defines it. */
_Noreturn void kernel_main(JPBootContext *context);

#endif /* JUSTOS_BOOT_BOOT_MANAGER_H */
