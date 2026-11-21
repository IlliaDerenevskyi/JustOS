/*
 * Unit test (level U) of src/boot/error.c: defaults, overrides, one record
 * per attempt, reading, and clearing. The real boot_log.c and boot_status.c
 * start the attempt.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_log.h"
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

static JPBootErrorInput input(JPBootResult result)
{
	JPBootErrorInput in;

	memset(&in, 0, sizeof in);
	in.original_result = result;
	in.source_component = JP_BOOT_SOURCE_STAGE2;
	in.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	in.diagnostic_context = 0x1234;
	return in;
}

static JPBootError get(void)
{
	JPBootError e;

	assert(jp_boot_error_get(&context, &e) == JP_BOOT_SUCCESS);
	return e;
}

/* ---- the defaults ---- */

static void test_defaults(void)
{
	static const struct {
		JPBootResult result;
		JPBootRecoverability recoverability;
		JPBootSeverity severity;
	} table[] = {
		{ JP_BOOT_ERROR_INVALID_PARAM, JP_BOOT_RECOVERABILITY_NON_RECOVERABLE, JP_BOOT_SEVERITY_FATAL },
		{ JP_BOOT_ERROR_CORRUPTED_DATA, JP_BOOT_RECOVERABILITY_NON_RECOVERABLE, JP_BOOT_SEVERITY_FATAL },
		{ JP_BOOT_FAILED, JP_BOOT_RECOVERABILITY_UNKNOWN, JP_BOOT_SEVERITY_ERROR },
		{ JP_BOOT_NOT_INITIALIZED, JP_BOOT_RECOVERABILITY_RECOVERABLE, JP_BOOT_SEVERITY_ERROR },
		{ JP_BOOT_NOT_DETECTED, JP_BOOT_RECOVERABILITY_RECOVERABLE, JP_BOOT_SEVERITY_ERROR },
		{ JP_BOOT_UNKNOWN, JP_BOOT_RECOVERABILITY_UNKNOWN, JP_BOOT_SEVERITY_ERROR }
	};
	size_t i;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		for (i = 0; i < sizeof table / sizeof table[0]; i++) {
			JPBootErrorInput in = input(table[i].result);
			JPBootError e;

			ready(arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64);
			in.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
			in.stage = JP_DIAGNOSTIC_STAGE_STAGE4;
			assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
			e = get();
			assert(e.original_result == table[i].result);
			assert(e.recoverability == table[i].recoverability && e.severity == table[i].severity);
			assert(e.boot_attempt_id == 1 && e.stage == JP_DIAGNOSTIC_STAGE_STAGE4);
			assert(e.source_component == JP_BOOT_SOURCE_BOOT_MANAGER && e.diagnostic_context == 0x1234);
			assert(e.architecture == (arch == 0 ? JP_ARCH_X86_64 : JP_ARCH_AARCH64));
		}
	}
}

/* ---- the overrides ---- */

static void test_overrides(void)
{
	JPBootErrorInput in;

	/* a severity can be lowered below ERROR when the default is not FATAL, and raised */
	ready(JP_ARCH_X86_64);
	in = input(JP_BOOT_FAILED);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_WARNING;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().severity == JP_BOOT_SEVERITY_WARNING);
	ready(JP_ARCH_X86_64);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_INFO;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().severity == JP_BOOT_SEVERITY_INFO);
	ready(JP_ARCH_X86_64);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_FATAL;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().severity == JP_BOOT_SEVERITY_FATAL);
	ready(JP_ARCH_X86_64);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_ERROR;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().severity == JP_BOOT_SEVERITY_ERROR);

	/* UNSPECIFIED means absent */
	ready(JP_ARCH_X86_64);
	in = input(JP_BOOT_FAILED);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_UNSPECIFIED;
	in.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_UNSPECIFIED;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().severity == JP_BOOT_SEVERITY_ERROR && get().recoverability == JP_BOOT_RECOVERABILITY_UNKNOWN);

	/* recoverability: UNKNOWN is refined, a recoverable result can be made non-recoverable */
	ready(JP_ARCH_X86_64);
	in.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_RECOVERABLE;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().recoverability == JP_BOOT_RECOVERABILITY_RECOVERABLE);
	ready(JP_ARCH_X86_64);
	in.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_NON_RECOVERABLE;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().recoverability == JP_BOOT_RECOVERABILITY_NON_RECOVERABLE);
	ready(JP_ARCH_X86_64);
	in = input(JP_BOOT_NOT_INITIALIZED);
	in.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_UNKNOWN;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(get().recoverability == JP_BOOT_RECOVERABILITY_UNKNOWN);
}

static void test_overrides_never_weaken(void)
{
	static JPBootContext copy;
	JPBootErrorInput in;
	JPBootSeverityOverride sev;
	JPBootRecoverabilityOverride rec;

	/* a FATAL default cannot be lowered */
	for (sev = JP_BOOT_SEVERITY_OVERRIDE_INFO; sev <= JP_BOOT_SEVERITY_OVERRIDE_ERROR; sev++) {
		ready(JP_ARCH_X86_64);
		memcpy(&copy, &context, sizeof copy);
		in = input(JP_BOOT_ERROR_CORRUPTED_DATA);
		in.severity_override = sev;
		assert(jp_boot_error_create(&context, &in) == JP_BOOT_ERROR_INVALID_PARAM);
		assert(memcmp(&copy, &context, sizeof copy) == 0);
	}
	/* a NON_RECOVERABLE default cannot be changed to anything else */
	for (rec = JP_BOOT_RECOVERABILITY_OVERRIDE_RECOVERABLE; rec <= JP_BOOT_RECOVERABILITY_OVERRIDE_UNKNOWN; rec++) {
		if (rec == JP_BOOT_RECOVERABILITY_OVERRIDE_NON_RECOVERABLE) {
			continue;
		}
		ready(JP_ARCH_X86_64);
		memcpy(&copy, &context, sizeof copy);
		in = input(JP_BOOT_ERROR_INVALID_PARAM);
		in.recoverability_override = rec;
		assert(jp_boot_error_create(&context, &in) == JP_BOOT_ERROR_INVALID_PARAM);
		assert(memcmp(&copy, &context, sizeof copy) == 0);
	}
	/* keeping the same value is fine */
	ready(JP_ARCH_X86_64);
	in = input(JP_BOOT_ERROR_CORRUPTED_DATA);
	in.severity_override = JP_BOOT_SEVERITY_OVERRIDE_FATAL;
	in.recoverability_override = JP_BOOT_RECOVERABILITY_OVERRIDE_NON_RECOVERABLE;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
}

/* ---- invalid input and state ---- */

static void test_invalid_input(void)
{
	static JPBootContext copy;
	JPBootErrorInput in;

	ready(JP_ARCH_X86_64);
	memcpy(&copy, &context, sizeof copy);
	in = input(JP_BOOT_FAILED);
	assert(jp_boot_error_create(NULL, &in) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_error_create(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
#define BAD(statement) \
	do { in = input(JP_BOOT_FAILED); statement; \
		assert(jp_boot_error_create(&context, &in) == JP_BOOT_ERROR_INVALID_PARAM); \
		assert(memcmp(&copy, &context, sizeof copy) == 0); } while (0)
	BAD(in.original_result = JP_BOOT_SUCCESS);                 /* success is not an error */
	BAD(in.original_result = (JPBootResult)7);
	BAD(in.source_component = (JPBootSourceComponent)14);
	BAD(in.stage = (JPDiagnosticStage)5);
	BAD(in.severity_override = (JPBootSeverityOverride)5);
	BAD(in.recoverability_override = (JPBootRecoverabilityOverride)4);
#undef BAD
	/* the extreme valid values */
	in = input(JP_BOOT_ERROR_CORRUPTED_DATA);
	in.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
	in.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
}

static void test_state_checks(void)
{
	JPBootErrorInput in = input(JP_BOOT_FAILED);

	memset(&context, 0, sizeof context);
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_NOT_INITIALIZED);            /* nothing started */
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.current_stage = JP_BOOT_STAGE_READY;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_NOT_INITIALIZED);            /* no status yet */

	ready(JP_ARCH_X86_64);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_FAILED);

	ready(JP_ARCH_X86_64);
	context.stage3.report.finalized = true;
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_FAILED);
}

/* ---- one error per attempt ---- */

static void test_one_error(void)
{
	static JPBootContext copy;
	JPBootErrorInput in = input(JP_BOOT_FAILED);
	JPBootError e;

	ready(JP_ARCH_X86_64);
	assert(jp_boot_error_get(&context, &e) == JP_BOOT_NOT_DETECTED);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	memcpy(&copy, &context, sizeof copy);
	/* the same result again, and another result: the first one stays */
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_FAILED);
	in = input(JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_FAILED);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
	assert(get().original_result == JP_BOOT_FAILED);
}

static void test_get_refusals(void)
{
	JPBootError e;

	ready(JP_ARCH_X86_64);
	assert(jp_boot_error_get(NULL, &e) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_error_get(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	memset(&context, 0, sizeof context);
	assert(jp_boot_error_get(&context, &e) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_FAILED);        /* architecture unknown: log does not start */
	context.handoff.architecture = JP_ARCH_X86_64;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_error_get(&context, &e) == JP_BOOT_NOT_INITIALIZED);   /* no status */
}

/* ---- clear ---- */

static void test_clear(void)
{
	JPBootErrorInput in = input(JP_BOOT_FAILED);
	JPBootError e;

	assert(jp_boot_error_clear(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	memset(&context, 0, sizeof context);
	assert(jp_boot_error_clear(&context) == JP_BOOT_NOT_INITIALIZED);

	ready(JP_ARCH_X86_64);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS);
	assert(jp_boot_error_clear(&context) == JP_BOOT_SUCCESS);        /* status is BOOTING: a new attempt */
	assert(jp_boot_error_get(&context, &e) == JP_BOOT_NOT_DETECTED);
	assert(jp_boot_error_create(&context, &in) == JP_BOOT_SUCCESS); /* a new error can be recorded */

	/* after the status moved on, the error cannot be removed */
	assert(jp_boot_status_update(&context, JP_DIAGNOSTIC_STAGE_STAGE2, JP_BOOT_STATUS_INITIALIZING) == JP_BOOT_SUCCESS);
	assert(jp_boot_error_clear(&context) == JP_BOOT_FAILED);
	assert(jp_boot_error_get(&context, &e) == JP_BOOT_SUCCESS);

	ready(JP_ARCH_X86_64);
	context.stage3.report.finalized = true;
	assert(jp_boot_error_clear(&context) == JP_BOOT_FAILED);
}

int main(void)
{
	test_defaults();
	test_overrides();
	test_overrides_never_weaken();
	test_invalid_input();
	test_state_checks();
	test_one_error();
	test_get_refusals();
	test_clear();
	return 0;
}
