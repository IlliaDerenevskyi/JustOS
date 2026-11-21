/*
 * fake_boot_manager.c - stands in for the Stage 4 recovery transition.
 *
 * jp_boot_recovery_enter() ends in jp_boot_manager_request_recovery(), which
 * Stage 4 (boot_manager.c) implements. Until Stage 4 exists, this stand-in
 * plays the "recovery diagnostics state": it finalizes the Boot Report, shows
 * what Stage 3 holds, and hands the machine to kernel_main (which reports the
 * data and ends the run). Test code only; never linked into production.
 */

#include "fake_stage3_lines.h"
#include "justos/boot/boot_manager.h"
#include "justos/boot/boot_report.h"

void kernel_main(JPBootContext *context) __attribute__((noreturn));

JPBootTransitionResult jp_boot_manager_request_recovery(JPBootContext *context, const JPBootRecoveryState *recovery)
{
	(void)recovery;
	(void)jp_boot_report_finalize(context);
	jp_test_stage3_report(context);
	kernel_main(context);
}
