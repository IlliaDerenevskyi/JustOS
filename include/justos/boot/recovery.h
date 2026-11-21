/*
 * recovery.h - recovery decision and entry of Stage 3.
 *
 * Purpose:
 *   Decides what to do after a failure and hands the decision to the Stage 4
 *   Boot Manager.
 *
 * Responsibilities:
 *   - jp_boot_recovery_decide: ENTER_RECOVERY, RETRY_BOOT, or TERMINAL_HALT,
 *     by the rules of milestone0.md ("3 Decided rules");
 *   - jp_boot_recovery_enter: carries the decision out and never returns;
 *     with a null context and JP_RECOVERY_TERMINAL_HALT it is also the
 *     routine that stops the machine when nothing can be recorded.
 *
 * Limitations:
 *   Declarations only. JP_BOOT_MAXIMUM_RETRY_COUNT (diagnostic_types.h) is 0:
 *   Milestone 0 never retries.
 *
 * Public API:
 *   jp_boot_recovery_decide, jp_boot_recovery_enter.
 *
 * Depends on:
 *   justos/boot/boot_types.h; at the end of jp_boot_recovery_enter, the
 *   function jp_boot_manager_request_recovery() of Stage 4.
 *
 * Must not depend on:
 *   firmware, heap.
 */

#ifndef JUSTOS_BOOT_RECOVERY_H
#define JUSTOS_BOOT_RECOVERY_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_recovery_decide(
	const JPBootContext *context,
	JPRecoveryDecision *decision
);

_Noreturn void jp_boot_recovery_enter(
	JPBootContext *context,
	JPRecoveryDecision decision
);

#endif /* JUSTOS_BOOT_RECOVERY_H */
