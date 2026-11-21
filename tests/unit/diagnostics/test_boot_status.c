/*
 * Unit test (level U) of src/boot/boot_status.c: the status graph, the stage
 * tags, the copy of the error into a failed status, and refusals that change
 * nothing. The real boot_log.c starts the attempt; the error record is put in
 * place by hand (error.c has its own test).
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_log.h"
#include "justos/boot/boot_status.h"

static JPBootContext context;

static void setup(JPArchitectureType architecture)
{
	memset(&context, 0, sizeof context);
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = architecture;
}

static void ready(void)
{
	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
}

static JPBootStatusState get(void)
{
	JPBootStatusState s;

	assert(jp_boot_status_get(&context, &s) == JP_BOOT_SUCCESS);
	return s;
}

static void record_error(JPBootResult result, JPBootSeverity severity, JPBootRecoverability recoverability)
{
	context.stage3.error.original_result = result;
	context.stage3.error.severity = severity;
	context.stage3.error.recoverability = recoverability;
	context.stage3.error_recorded = true;
}

/* A status that is reached by the allowed way, for the tests of the graph. */
static void reach(JPBootStatus status)
{
	ready();
	if (status == JP_BOOT_STATUS_BOOTING) {
		return;
	}
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	if (status == JP_BOOT_STATUS_INITIALIZING) {
		return;
	}
	if (status == JP_BOOT_STATUS_RUNNING) {
		assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_SUCCESS);
		return;
	}
	if (status == JP_BOOT_STATUS_FAILED) {
		record_error(JP_BOOT_FAILED, JP_BOOT_SEVERITY_ERROR, JP_BOOT_RECOVERABILITY_UNKNOWN);
		assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
		return;
	}
	if (status == JP_BOOT_STATUS_RECOVERY || status == JP_BOOT_STATUS_HALTED) {
		record_error(JP_BOOT_FAILED, JP_BOOT_SEVERITY_ERROR, JP_BOOT_RECOVERABILITY_UNKNOWN);
		assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
		assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, status) == JP_BOOT_SUCCESS);
	}
}

/* ---- initialize ---- */

static void test_initialize(void)
{
	JPBootStatusState s;

	setup(JP_ARCH_AARCH64);
	assert(jp_boot_status_get(&context, &s) == JP_BOOT_NOT_INITIALIZED);     /* nothing exists yet */
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_get(&context, &s) == JP_BOOT_NOT_INITIALIZED);     /* the log alone is not enough */
	assert(jp_boot_status_initialize(&context) == JP_BOOT_SUCCESS);
	s = get();
	assert(s.status == JP_BOOT_STATUS_BOOTING && s.stage == JP_DIAGNOSTIC_STAGE_STAGE1);
	assert(s.severity == JP_BOOT_SEVERITY_INFO && s.recoverability == JP_BOOT_RECOVERABILITY_UNKNOWN);
	assert(s.original_result == JP_BOOT_SUCCESS && s.architecture == JP_ARCH_AARCH64);
}

static void test_initialize_refusals(void)
{
	static JPBootContext copy;

	assert(jp_boot_status_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_NOT_INITIALIZED);   /* before the log */
	ready();
	memcpy(&copy, &context, sizeof copy);
	assert(jp_boot_status_initialize(&context) == JP_BOOT_FAILED);            /* a second time */
	assert(memcmp(&copy, &context, sizeof copy) == 0);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_boot_status_initialize(&context) == JP_BOOT_FAILED);
	assert(!context.stage3.status_initialized);
}

/* ---- the graph ---- */

static const JPBootStatus all_status[] = {
	JP_BOOT_STATUS_BOOTING, JP_BOOT_STATUS_INITIALIZING, JP_BOOT_STATUS_RUNNING,
	JP_BOOT_STATUS_FAILED, JP_BOOT_STATUS_RECOVERY, JP_BOOT_STATUS_HALTED
};

static bool allowed(JPBootStatus from, JPBootStatus to)
{
	switch (from) {
	case JP_BOOT_STATUS_BOOTING: return to == JP_BOOT_STATUS_INITIALIZING;
	case JP_BOOT_STATUS_INITIALIZING: return to == JP_BOOT_STATUS_RUNNING || to == JP_BOOT_STATUS_FAILED;
	case JP_BOOT_STATUS_RUNNING: return to == JP_BOOT_STATUS_FAILED || to == JP_BOOT_STATUS_RECOVERY;
	case JP_BOOT_STATUS_FAILED: return to == JP_BOOT_STATUS_RECOVERY || to == JP_BOOT_STATUS_HALTED;
	case JP_BOOT_STATUS_RECOVERY: return to == JP_BOOT_STATUS_BOOTING || to == JP_BOOT_STATUS_HALTED;
	default: return false;
	}
}

/* The tag that fits a change. */
static JPDiagnosticStage fitting_tag(JPBootStatus to)
{
	switch (to) {
	case JP_BOOT_STATUS_BOOTING: return JP_DIAGNOSTIC_STAGE_STAGE3;
	case JP_BOOT_STATUS_INITIALIZING: return JP_DIAGNOSTIC_STAGE_STAGE2;
	case JP_BOOT_STATUS_FAILED: return JP_DIAGNOSTIC_STAGE_STAGE2;
	default: return JP_DIAGNOSTIC_STAGE_STAGE3;
	}
}

static void test_every_pair(void)
{
	size_t from;
	size_t to;

	for (from = 0; from < sizeof all_status / sizeof all_status[0]; from++) {
		for (to = 0; to < sizeof all_status / sizeof all_status[0]; to++) {
			static JPBootContext copy;
			JPBootResult result;

			reach(all_status[from]);
			record_error(JP_BOOT_FAILED, JP_BOOT_SEVERITY_ERROR, JP_BOOT_RECOVERABILITY_UNKNOWN);   /* a failure may need it */
			memcpy(&copy, &context, sizeof copy);
			result = jp_boot_status_update(&context, fitting_tag(all_status[to]), all_status[to]);
			if (allowed(all_status[from], all_status[to])) {
				assert(result == JP_BOOT_SUCCESS);
				assert(get().status == all_status[to]);
			} else {
				assert(result == JP_BOOT_FAILED);
				assert(memcmp(&copy, &context, sizeof copy) == 0);       /* nothing changed */
			}
		}
	}
}

static void test_halted_is_final(void)
{
	size_t to;
	JPBootStatusState before;

	reach(JP_BOOT_STATUS_HALTED);
	before = get();
	for (to = 0; to < sizeof all_status / sizeof all_status[0]; to++) {
		assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, all_status[to]) != JP_BOOT_SUCCESS);
		assert(memcmp(&before, &context.stage3.status, sizeof before) == 0);
	}
}

static void test_a_success_is_never_restored(void)
{
	/* after a failure, RUNNING (a success state) cannot come back in the same attempt */
	reach(JP_BOOT_STATUS_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_FAILED);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_FAILED);
	assert(get().status == JP_BOOT_STATUS_FAILED && get().original_result == JP_BOOT_FAILED);
}

/* ---- the tags ---- */

static void test_tags(void)
{
	JPDiagnosticStage tag;
	JPBootStatus to;

	for (to = JP_BOOT_STATUS_BOOTING; to <= JP_BOOT_STATUS_HALTED; to++) {
		for (tag = JP_DIAGNOSTIC_STAGE_UNKNOWN; tag <= JP_DIAGNOSTIC_STAGE_STAGE4; tag++) {
			bool fits;
			JPBootStatus from;

			switch (to) {
			case JP_BOOT_STATUS_BOOTING: fits = tag == JP_DIAGNOSTIC_STAGE_STAGE1 || tag == JP_DIAGNOSTIC_STAGE_STAGE3; from = JP_BOOT_STATUS_RECOVERY; break;
			case JP_BOOT_STATUS_INITIALIZING: fits = tag == JP_DIAGNOSTIC_STAGE_STAGE1 || tag == JP_DIAGNOSTIC_STAGE_STAGE2; from = JP_BOOT_STATUS_BOOTING; break;
			case JP_BOOT_STATUS_RUNNING: fits = tag == JP_DIAGNOSTIC_STAGE_STAGE3; from = JP_BOOT_STATUS_INITIALIZING; break;
			case JP_BOOT_STATUS_FAILED: fits = tag != JP_DIAGNOSTIC_STAGE_UNKNOWN; from = JP_BOOT_STATUS_RUNNING; break;
			case JP_BOOT_STATUS_RECOVERY: fits = tag == JP_DIAGNOSTIC_STAGE_STAGE3; from = JP_BOOT_STATUS_FAILED; break;
			default: fits = tag == JP_DIAGNOSTIC_STAGE_STAGE3; from = JP_BOOT_STATUS_FAILED; break;
			}
			reach(from);
			record_error(JP_BOOT_FAILED, JP_BOOT_SEVERITY_ERROR, JP_BOOT_RECOVERABILITY_UNKNOWN);
			if (fits) {
				assert(jp_boot_status_update(&context, tag, to) == JP_BOOT_SUCCESS);
				assert(get().stage == tag);
			} else {
				assert(jp_boot_status_update(&context, tag, to) == JP_BOOT_ERROR_INVALID_PARAM);
				assert(get().status == from);
			}
		}
	}
}

static void test_invalid_arguments(void)
{
	assert(jp_boot_status_update(NULL, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RUNNING) == JP_BOOT_ERROR_INVALID_PARAM);
	ready();
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE1, (JPBootStatus)6) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_status_update(&context, (JPDiagnosticStage)5, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_ERROR_INVALID_PARAM);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE1, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE1, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_NOT_INITIALIZED);
}

/* ---- a failure needs an error ---- */

static void test_failure_copies_the_error(void)
{
	JPBootStatusState s;

	reach(JP_BOOT_STATUS_RUNNING);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_FAILED) == JP_BOOT_FAILED);   /* no error recorded */
	assert(get().status == JP_BOOT_STATUS_RUNNING && get().original_result == JP_BOOT_SUCCESS);
	record_error(JP_BOOT_ERROR_CORRUPTED_DATA, JP_BOOT_SEVERITY_FATAL, JP_BOOT_RECOVERABILITY_NON_RECOVERABLE);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE4, JP_BOOT_STATUS_FAILED) == JP_BOOT_SUCCESS);
	s = get();
	assert(s.status == JP_BOOT_STATUS_FAILED && s.stage == JP_DIAGNOSTIC_STAGE_STAGE4);
	assert(s.original_result == JP_BOOT_ERROR_CORRUPTED_DATA && s.severity == JP_BOOT_SEVERITY_FATAL);
	assert(s.recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE && s.architecture == JP_ARCH_X86_64);

	/* the failure result stays through RECOVERY and HALTED */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(get().original_result == JP_BOOT_ERROR_CORRUPTED_DATA && get().severity == JP_BOOT_SEVERITY_FATAL);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_HALTED) == JP_BOOT_SUCCESS);
	assert(get().original_result == JP_BOOT_ERROR_CORRUPTED_DATA && get().recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE);
}

static void test_recovery_without_error(void)
{
	/* a recovery asked for through the configuration: RUNNING to RECOVERY needs no error */
	reach(JP_BOOT_STATUS_RUNNING);
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_SUCCESS);
	assert(get().original_result == JP_BOOT_SUCCESS && get().severity == JP_BOOT_SEVERITY_INFO);
}

static void test_final_report_freezes_the_status(void)
{
	reach(JP_BOOT_STATUS_RUNNING);
	context.stage3.report.finalized = true;
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE3, JP_BOOT_STATUS_RECOVERY) == JP_BOOT_FAILED);
	assert(get().status == JP_BOOT_STATUS_RUNNING);
}

static void test_get_refusals(void)
{
	JPBootStatusState s;

	ready();
	assert(jp_boot_status_get(NULL, &s) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_status_get(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

int main(void)
{
	test_initialize();
	test_initialize_refusals();
	test_every_pair();
	test_halted_is_final();
	test_a_success_is_never_restored();
	test_tags();
	test_invalid_arguments();
	test_failure_copies_the_error();
	test_recovery_without_error();
	test_final_report_freezes_the_status();
	test_get_refusals();
	return 0;
}
