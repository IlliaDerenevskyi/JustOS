/*
 * fake_boot_manager_rejects.c - a Stage 4 stand-in whose recovery transition is refused.
 *
 * jp_boot_recovery_enter() asks jp_boot_manager_request_recovery() to take over. This stand-in answers
 * "rejected", so Stage 3 must take its terminal path: record the halt, finalize the report, show the panic
 * screen, and stop the processor. It is used by the QEMU variant "panic". Test code only; never linked into
 * production.
 */

#include "justos/boot/boot_manager.h"

JPBootTransitionResult jp_boot_manager_request_recovery(JPBootContext *context, const JPBootRecoveryState *recovery)
{
	(void)context;
	(void)recovery;
	return JP_BOOT_TRANSITION_REJECTED;
}
