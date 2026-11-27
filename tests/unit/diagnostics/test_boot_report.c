/*
 * Unit test (level U) of src/boot/boot_report.c: the consistency checks, the
 * published report, idempotence, and refusals. The real boot_log.c,
 * boot_status.c, and error.c build the records.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_log.h"
#include "justos/boot/boot_report.h"
#include "justos/boot/boot_status.h"
#include "justos/boot/error.h"

static JPBootContext context;

static void ready(JPArchitectureType architecture)
{
	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = architecture;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
}

static void fail_with(JPBootResult result)
{
	JPBootErrorInput in;

	memset(&in, 0, sizeof in);
	in.original_result = result;
	in.source_component = JP_BOOT_SOURCE_STAGE2;
	in.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	in.diagnostic_context = 0x42;
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
}

static JPBootReport report(void)
{
	JPBootReport r;

	assert(jp_boot_report_get(&context, &r) == JP_BOOT_SUCCESS);
	return r;
}

/* ---- A good report ---- */

static void test_running(void)
{
	JPBootReport r;
	JPBootError zero;

	ready(JP_ARCH_X86_64);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
	assert(jp_boot_report_get(&context, &r) == JP_BOOT_NOT_INITIALIZED);        /* not final yet */
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	r = report();
	assert(r.finalized && r.boot_attempt_id == 1 && r.architecture == JP_ARCH_X86_64);
	assert(r.status.status == JP_BOOT_STATUS_RUNNING && r.status.original_result == JP_BOOT_SUCCESS);
	assert(r.log_status.event_count == 1 && r.log_status.finalized && !r.log_status.truncated);
	assert(r.recovery.current_attempt_id == 1 && !r.recovery.exhausted && r.recovery.retry_count == 0);
	/* no error: the error of the report is all zero (a real error never has the result SUCCESS) */
	memset(&zero, 0, sizeof zero);
	assert(memcmp(&r.error, &zero, sizeof zero) == 0);
	assert(context.stage3.report.finalized && context.stage3.log.status.finalized);
}

static void test_failed(void)
{
	JPBootReport r;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		ready(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64);
		fail_with(JP_BOOT_ERROR_CORRUPTED_DATA);
		assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
		r = report();
		assert(r.status.status == JP_BOOT_STATUS_FAILED && r.status.original_result == JP_BOOT_ERROR_CORRUPTED_DATA);
		assert(r.error.original_result == JP_BOOT_ERROR_CORRUPTED_DATA && r.error.severity == JP_BOOT_SEVERITY_FATAL);
		assert(r.error.diagnostic_context == 0x42 && r.error.boot_attempt_id == 1);
		assert(r.architecture == (arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64));
	}
}

static void test_recovery_and_halted(void)
{
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	context.stage3.recovery.decision = JP_RECOVERY_ENTER;
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(report().status.status == JP_BOOT_STATUS_RECOVERY && report().recovery.decision == JP_RECOVERY_ENTER);

	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(report().status.status == JP_BOOT_STATUS_HALTED && report().error.original_result == JP_BOOT_FAILED);

	/* recovery asked for through the configuration: no error, status RECOVERY */
	ready(JP_ARCH_X86_64);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(report().error.original_result == JP_BOOT_SUCCESS);
}

/* ---- Idempotence ---- */

static void test_idempotent(void)
{
	static JPBootContext copy;
	JPBootReport first;

	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	memcpy(&copy, &context, sizeof copy);
	/* The stored report is copied with memcpy. A copy by value (as report() makes) may leave the padding bytes of the
	 * structure in any state, and the compilers differ in that (gcc on AArch64 does), so a memcmp of such a copy is not a test. */
	memcpy(&first, &context.stage3.report, sizeof first);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
	assert(memcmp(&first, &context.stage3.report, sizeof first) == 0);

	/* once final, even damaged records do not matter: the report is returned as it was */
	context.stage3.attempt.boot_attempt_id = 99;
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(report().boot_attempt_id == 1);
}

/* ---- The consistency checks ---- */

static void expect_corrupted(void)
{
	static JPBootContext copy;

	memcpy(&copy, &context, sizeof copy);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(memcmp(&copy, &context, sizeof copy) == 0);                      /* nothing is published, nothing is changed */
	assert(!context.stage3.report.finalized && !context.stage3.log.status.finalized);
}

static void test_inconsistent_records(void)
{
	/* the recovery state belongs to another attempt */
	ready(JP_ARCH_X86_64);
	context.stage3.recovery.current_attempt_id = 2;
	expect_corrupted();

	/* the error belongs to another attempt */
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	context.stage3.error.boot_attempt_id = 7;
	expect_corrupted();

	/* the status FAILED without an error */
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	context.stage3.error_recorded = false;
	expect_corrupted();

	/* the status holds another result than the error, for FAILED, RECOVERY, and HALTED */
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	context.stage3.status.original_result = JP_BOOT_NOT_DETECTED;
	expect_corrupted();
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	context.stage3.status.original_result = JP_BOOT_UNKNOWN;
	expect_corrupted();
	ready(JP_ARCH_X86_64);
	fail_with(JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	context.stage3.status.original_result = JP_BOOT_UNKNOWN;
	expect_corrupted();

	/* the retry counter above its limit */
	ready(JP_ARCH_X86_64);
	context.stage3.recovery.retry_count = 1;
	context.stage3.recovery.maximum_retry_count = 0;
	expect_corrupted();
	ready(JP_ARCH_X86_64);
	context.stage3.attempt.retry_count = 3;
	context.stage3.attempt.maximum_retry_count = 2;
	expect_corrupted();

	/* the log count is above the capacity, or differs from the status */
	ready(JP_ARCH_X86_64);
	context.stage3.log.status.event_count = JP_BOOT_LOG_CAPACITY + 1;
	expect_corrupted();
}

static void test_boundary_values_are_consistent(void)
{
	/* a retry counter equal to its limit, and a full log, are fine */
	ready(JP_ARCH_X86_64);
	context.stage3.recovery.retry_count = 2;
	context.stage3.recovery.maximum_retry_count = 2;
	context.stage3.attempt.retry_count = 2;
	context.stage3.attempt.maximum_retry_count = 2;
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(report().recovery.exhausted);

	ready(JP_ARCH_X86_64);
	context.stage3.log.status.event_count = JP_BOOT_LOG_CAPACITY;
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
}

static void test_exhausted_flag(void)
{
	/* exhausted only when retry_count > 0 and retry_count >= maximum */
	ready(JP_ARCH_X86_64);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(!report().recovery.exhausted);                                   /* 0 and 0: no retry was used */

	ready(JP_ARCH_X86_64);
	context.stage3.recovery.retry_count = 1;
	context.stage3.recovery.maximum_retry_count = 2;
	context.stage3.attempt.retry_count = 1;
	context.stage3.attempt.maximum_retry_count = 2;
	assert(jp_boot_report_finalize(&context) == JP_BOOT_SUCCESS);
	assert(!report().recovery.exhausted);                                   /* 1 of 2 */
}

/* ---- Refusals ---- */

static void test_refusals(void)
{
	JPBootReport r;

	assert(jp_boot_report_finalize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_report_get(NULL, &r) == JP_BOOT_ERROR_INVALID_PARAM);
	ready(JP_ARCH_X86_64);
	assert(jp_boot_report_get(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	memset(&context, 0, sizeof context);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_boot_report_get(&context, &r) == JP_BOOT_NOT_INITIALIZED);
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_report_finalize(&context) == JP_BOOT_NOT_INITIALIZED);   /* no status yet */
}

int main(void)
{
	test_running();
	test_failed();
	test_recovery_and_halted();
	test_idempotent();
	test_inconsistent_records();
	test_boundary_values_are_consistent();
	test_exhausted_flag();
	test_refusals();
	return 0;
}
