/*
 * fake_stage2_complete.c - stands in for the Stage 4 continuation.
 *
 * Stage 2 ends by calling jp_kernel_stage2_complete(context), which Stage 4
 * (boot_manager.c) implements. Stage 4 does not exist yet, so the test kernel
 * provides this stand-in. It does the part of the Stage 4 flow that uses
 * Stage 3 ("4 Stage 4 Execution Flow", steps 3 to 7), so that the QEMU tests
 * run the real Stage 3 code and the real serial and screen output:
 *
 *   start the log and the status; record the Stage 2 result; set the status;
 *   on success finalize the report and go to kernel_main; on a failure decide
 *   and enter recovery (jp_boot_recovery_enter), which reaches the stand-in
 *   jp_boot_manager_request_recovery() of fake_boot_manager.c.
 *
 * Test code only; never linked into production. It is replaced by Stage 4.
 */

#include "fake_stage3_lines.h"
#include "justos/boot/boot_log.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/diagnostic_output.h"
#include "justos/boot/error.h"
#include "justos/kernel/init.h"
#include "justos/boot/recovery.h"

void kernel_main(JPBootContext *context) __attribute__((noreturn));

static void add_stage2_event(JPBootContext *context)
{
	JPBootEvent event;

	event.boot_attempt_id = 0;
	event.event_sequence = 0;
	event.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	event.severity = context->stage2_result == JP_BOOT_SUCCESS ? JP_BOOT_SEVERITY_INFO : JP_BOOT_SEVERITY_ERROR;
	event.architecture = context->handoff.architecture;
	event.source_component = JP_BOOT_SOURCE_STAGE2;
	event.result = context->stage2_result;
	event.diagnostic_context = 0;
	(void)jp_boot_log_append(context, &event);
}

static void record_stage2_failure(JPBootContext *context)
{
	JPBootErrorInput input;

	input.original_result = context->stage2_result;
	input.source_component = JP_BOOT_SOURCE_STAGE2;
	input.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	input.severity_override = JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED;
	input.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED;
	input.diagnostic_context = 0;
	(void)jp_boot_error_create(context, &input);
}

_Noreturn void jp_kernel_stage2_complete(JPBootContext *context)
{
	JPRecoveryDecision decision;

	if (context == NULL || jp_boot_log_initialize(context) != JP_BOOT_SUCCESS ||
	    jp_boot_status_initialize(context) != JP_BOOT_SUCCESS) {
		jp_boot_recovery_enter(NULL, JP_RECOVERY_TERMINAL_HALT);     /* the Stage 3 stopping routine */
	}
	add_stage2_event(context);
	(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING);
	jp_test_stage3_line(context, "JP|stage3.output=OK");            /* the line went through the real output path */

	if (context->stage2_result == JP_BOOT_SUCCESS) {
		(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING);
		(void)jp_boot_report_finalize(context);
		jp_test_stage3_report(context);
		kernel_main(context);
	}
	record_stage2_failure(context);
	(void)jp_boot_status_update(context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED);
	if (jp_boot_recovery_decide(context, &decision) != JP_BOOT_SUCCESS) {
		decision = JP_RECOVERY_TERMINAL_HALT;
	}
	jp_boot_recovery_enter(context, decision);
}
