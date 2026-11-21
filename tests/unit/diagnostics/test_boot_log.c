/*
 * Unit test (level U) of src/boot/boot_log.c: initialization, appending,
 * the retention rule of a full log, reading, and finalization.
 */

#include <assert.h>
#include <string.h>

#include "justos/boot/boot_log.h"

static JPBootContext context;

static void setup(JPArchitectureType architecture)
{
	memset(&context, 0xCC, sizeof context);       /* garbage: initialize must clear the whole area */
	context.current_stage = JP_BOOT_STAGE_READY;
	context.handoff.state = JP_HANDOFF_STATE_READY;
	context.handoff.architecture = architecture;
	context.stage3.log_initialized = false;
}

static JPBootEvent make_event(JPBootSeverity severity, uint32_t tag)
{
	JPBootEvent e;

	memset(&e, 0, sizeof e);
	e.stage = JP_DIAGNOSTIC_STAGE_STAGE2;
	e.severity = severity;
	e.architecture = context.handoff.architecture;
	e.source_component = JP_BOOT_SOURCE_STAGE2;
	e.result = severity >= JP_BOOT_SEVERITY_ERROR ? JP_BOOT_FAILED : JP_BOOT_SUCCESS;
	e.diagnostic_context = tag;
	return e;
}

static JPBootEvent get(size_t index)
{
	JPBootEvent e;

	assert(jp_boot_log_get(&context, index, &e) == JP_BOOT_SUCCESS);
	return e;
}

static JPBootLogStatus status_of(void)
{
	JPBootLogStatus s;

	assert(jp_boot_log_get_status(&context, &s) == JP_BOOT_SUCCESS);
	return s;
}

static void ready(void)
{
	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
}

/* ---- initialize ---- */

static void test_initialize(void)
{
	JPBootEvent e;
	JPBootLogStatus s;
	size_t i;
	const uint8_t *area;

	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	s = status_of();
	assert(s.event_count == 1 && s.discarded_event_count == 0 && !s.truncated && !s.finalized);

	e = get(0);
	assert(e.boot_attempt_id == 1 && e.event_sequence == 0);
	assert(e.stage == JP_DIAGNOSTIC_STAGE_STAGE1 && e.source_component == JP_BOOT_SOURCE_STAGE1);
	assert(e.severity == JP_BOOT_SEVERITY_INFO && e.result == JP_BOOT_SUCCESS);
	assert(e.architecture == JP_ARCH_X86_64 && e.diagnostic_context == 0);

	/* the attempt and the recovery state */
	assert(context.stage3.attempt.boot_attempt_id == 1 && context.stage3.attempt.attempt_sequence == 0);
	assert(context.stage3.attempt.retry_count == 0 && context.stage3.attempt.maximum_retry_count == JP_BOOT_MAXIMUM_RETRY_COUNT);
	assert(context.stage3.recovery.current_attempt_id == 1 && context.stage3.recovery.previous_attempt_id == 0);
	assert(context.stage3.recovery.retry_count == 0 && !context.stage3.recovery.exhausted);
	assert(context.stage3.log_initialized);

	/* the whole area was cleared: nothing of the garbage is left, except what was set */
	assert(!context.stage3.status_initialized && !context.stage3.error_recorded && !context.stage3.first_panic_recorded);
	assert(context.stage3.panic == JP_BOOT_PANIC_NORMAL && context.stage3.output.next_line == 0);
	assert(!context.stage3.report.finalized && context.stage3.output.failed_messages == 0);
	area = (const uint8_t *)&context.stage3.log.events[1];     /* the unused events */
	for (i = 0; i < (JP_BOOT_LOG_CAPACITY - 1) * sizeof(JPBootEvent); i++) {
		assert(area[i] == 0);
	}

	/* the next event is number 1 */
	e = make_event(JP_BOOT_SEVERITY_INFO, 7);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(get(1).event_sequence == 1);
}

static void test_initialize_other_architecture_and_stage1_state(void)
{
	JPBootEvent e;

	setup(JP_ARCH_AARCH64);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	assert(get(0).architecture == JP_ARCH_AARCH64);

	/* Stage 1 is not READY: event 0 says so (the record tells the truth) */
	setup(JP_ARCH_X86_64);
	context.current_stage = JP_BOOT_STAGE_KERNEL_LOADING;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	e = get(0);
	assert(e.severity == JP_BOOT_SEVERITY_ERROR && e.result == JP_BOOT_FAILED);
	setup(JP_ARCH_X86_64);
	context.handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED;
	assert(jp_boot_log_initialize(&context) == JP_BOOT_SUCCESS);
	e = get(0);
	assert(e.severity == JP_BOOT_SEVERITY_ERROR && e.result == JP_BOOT_FAILED);
}

static void test_initialize_refusals(void)
{
	assert(jp_boot_log_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	setup(JP_ARCH_UNKNOWN);
	assert(jp_boot_log_initialize(&context) == JP_BOOT_FAILED);
	assert(!context.stage3.log_initialized);
	/* a second call changes nothing */
	ready();
	{
		static JPBootContext copy;

		memcpy(&copy, &context, sizeof copy);
		assert(jp_boot_log_initialize(&context) == JP_BOOT_FAILED);
		assert(memcmp(&copy, &context, sizeof copy) == 0);
	}
}

/* ---- append ---- */

static void test_append(void)
{
	JPBootEvent e;
	size_t i;

	ready();
	for (i = 0; i < 10; i++) {
		e = make_event(JP_BOOT_SEVERITY_INFO, (uint32_t)(100 + i));
		e.event_sequence = 999;                      /* ignored: the log sets it */
		e.boot_attempt_id = 777;
		assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	}
	assert(status_of().event_count == 11);
	for (i = 0; i < 11; i++) {
		e = get(i);
		assert(e.event_sequence == i && e.boot_attempt_id == 1);
		if (i > 0) {
			assert(e.diagnostic_context == 100 + i - 1);
		}
	}
}

static void test_append_refusals(void)
{
	JPBootEvent e;
	static JPBootContext copy;

	ready();
	e = make_event(JP_BOOT_SEVERITY_INFO, 1);
	assert(jp_boot_log_append(NULL, &e) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_log_append(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	memcpy(&copy, &context, sizeof copy);
#define BAD(statement) \
	do { \
		e = make_event(JP_BOOT_SEVERITY_INFO, 1); \
		statement; \
		assert(jp_boot_log_append(&context, &e) == JP_BOOT_ERROR_INVALID_PARAM); \
		assert(memcmp(&copy, &context, sizeof copy) == 0); \
	} while (0)
	BAD(e.stage = (JPDiagnosticStage)5);
	BAD(e.severity = (JPBootSeverity)4);
	BAD(e.source_component = (JPBootSourceComponent)14);
	BAD(e.result = (JPBootResult)7);
	BAD(e.architecture = JP_ARCH_AARCH64);                  /* not the architecture of the context */
	BAD(e.architecture = JP_ARCH_UNKNOWN);
#undef BAD
	/* boundary values that are valid */
	e = make_event(JP_BOOT_SEVERITY_FATAL, 1);
	e.stage = JP_DIAGNOSTIC_STAGE_STAGE4;
	e.source_component = JP_BOOT_SOURCE_BOOT_MANAGER;
	e.result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	e = make_event(JP_BOOT_SEVERITY_INFO, 1);
	e.stage = JP_DIAGNOSTIC_STAGE_UNKNOWN;
	e.source_component = JP_BOOT_SOURCE_UNKNOWN;
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);

	setup(JP_ARCH_X86_64);
	e = make_event(JP_BOOT_SEVERITY_INFO, 1);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_NOT_INITIALIZED);
}

/* ---- the full log ---- */

/* Fills the log: event 0 is INFO; the others are given by the severities of the pattern. */
static void fill(JPBootSeverity (*pattern)(size_t))
{
	size_t i;

	ready();
	for (i = 1; i < JP_BOOT_LOG_CAPACITY; i++) {
		JPBootEvent e = make_event(pattern(i), (uint32_t)i);

		assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	}
	assert(status_of().event_count == JP_BOOT_LOG_CAPACITY);
	assert(!status_of().truncated && status_of().discarded_event_count == 0);
}

static JPBootSeverity all_info(size_t i)
{
	(void)i;
	return JP_BOOT_SEVERITY_INFO;
}

static JPBootSeverity all_error(size_t i)
{
	(void)i;
	return JP_BOOT_SEVERITY_ERROR;
}

static JPBootSeverity mixed(size_t i)
{
	return (JPBootSeverity)(i % 4);        /* INFO, WARNING, ERROR, FATAL, ... */
}

static void test_exact_capacity(void)
{
	JPBootEvent e;

	/* event 0 is INFO, the other 255 are ERROR; the log holds exactly 256 events */
	fill(all_error);
	assert(get(255).event_sequence == 255);
	assert(status_of().event_count == 256);

	/* the 257th event: the lowest retained event is event 0 (INFO), so an ERROR event replaces it */
	e = make_event(JP_BOOT_SEVERITY_ERROR, 9999);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().event_count == 256 && status_of().discarded_event_count == 1 && status_of().truncated);
	assert(context.stage3.log.next_event_sequence == 257);
	assert(get(0).diagnostic_context == 1 && get(255).diagnostic_context == 9999 && get(255).event_sequence == 256);

	/* now every retained event is ERROR: the next ERROR event is dropped and counted, its sequence number is not used */
	e = make_event(JP_BOOT_SEVERITY_ERROR, 9998);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().event_count == 256 && status_of().discarded_event_count == 2);
	assert(context.stage3.log.next_event_sequence == 257);
	assert(get(255).diagnostic_context == 9999);
	assert(jp_boot_log_get(&context, 256, &e) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_full_log_replaces_the_latest_lowest(void)
{
	JPBootEvent e;
	size_t i;

	/* all INFO: a WARNING replaces the latest INFO (event 255); the later events move down */
	fill(all_info);
	e = make_event(JP_BOOT_SEVERITY_WARNING, 5000);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().event_count == 256 && status_of().discarded_event_count == 1 && status_of().truncated);
	assert(get(255).diagnostic_context == 5000 && get(255).event_sequence == 256);   /* the new one is last */
	assert(get(254).diagnostic_context == 254);
	assert(context.stage3.log.next_event_sequence == 257);

	/* an INFO event does not replace an INFO event (equal severity: the earlier one stays) */
	e = make_event(JP_BOOT_SEVERITY_INFO, 5001);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().discarded_event_count == 2 && context.stage3.log.next_event_sequence == 257);
	assert(get(255).diagnostic_context == 5000);

	/* an ERROR replaces the latest of the lowest: now that is event 254 (INFO) */
	e = make_event(JP_BOOT_SEVERITY_ERROR, 5002);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(get(255).diagnostic_context == 5002 && get(254).diagnostic_context == 5000 && get(253).diagnostic_context == 253);
	assert(status_of().discarded_event_count == 3);

	/* the order of the sequence numbers is increasing, with gaps where events were removed */
	for (i = 1; i < 256; i++) {
		assert(get(i).event_sequence > get(i - 1).event_sequence);
	}
}

static void test_full_log_never_loses_error_or_fatal(void)
{
	JPBootEvent e;
	size_t i;

	/* ERROR events and the INFO event 0: a FATAL event takes the place of event 0, then a second FATAL is dropped */
	fill(all_error);
	e = make_event(JP_BOOT_SEVERITY_FATAL, 7000);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().discarded_event_count == 1);
	e = make_event(JP_BOOT_SEVERITY_FATAL, 7001);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().discarded_event_count == 2 && status_of().truncated);
	for (i = 0; i < 255; i++) {
		assert(get(i).severity == JP_BOOT_SEVERITY_ERROR && get(i).diagnostic_context == i + 1);
	}
	assert(get(255).severity == JP_BOOT_SEVERITY_FATAL && get(255).diagnostic_context == 7000);

	/* a mixed log: only INFO and WARNING events are ever removed, and the removed one is the lowest, latest */
	fill(mixed);
	{
		size_t errors_before = 0;
		size_t fatals_before = 0;
		size_t errors_after = 0;
		size_t fatals_after = 0;

		for (i = 0; i < 256; i++) {
			errors_before += get(i).severity == JP_BOOT_SEVERITY_ERROR;
			fatals_before += get(i).severity == JP_BOOT_SEVERITY_FATAL;
		}
		e = make_event(JP_BOOT_SEVERITY_FATAL, 8000);
		assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
		for (i = 0; i < 256; i++) {
			errors_after += get(i).severity == JP_BOOT_SEVERITY_ERROR;
			fatals_after += get(i).severity == JP_BOOT_SEVERITY_FATAL;
		}
		assert(errors_after == errors_before);
		assert(fatals_after == fatals_before + 1);
		/* INFO is i % 4 == 0; the latest INFO event below 256 is 252 */
		for (i = 0; i < 256; i++) {
			assert(get(i).diagnostic_context != 252 || i == 0);
		}
	}
}

static void test_warning_replaces_only_lower(void)
{
	JPBootEvent e;

	/* all INFO: a WARNING replaces the latest INFO; a further INFO is dropped (equal to the lowest retained) */
	fill(all_info);
	e = make_event(JP_BOOT_SEVERITY_WARNING, 1);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().discarded_event_count == 1 && get(255).diagnostic_context == 1);
	e = make_event(JP_BOOT_SEVERITY_INFO, 2);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_SUCCESS);
	assert(status_of().discarded_event_count == 2 && get(255).diagnostic_context == 1);
}

/* ---- get, status, finalize ---- */

static void test_get(void)
{
	JPBootEvent e;

	ready();
	assert(jp_boot_log_get(NULL, 0, &e) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_log_get(&context, 0, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_log_get(&context, 1, &e) == JP_BOOT_ERROR_INVALID_PARAM);     /* only event 0 exists */
	assert(jp_boot_log_get(&context, (size_t)-1, &e) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_boot_log_get(&context, 0, &e) == JP_BOOT_SUCCESS);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_get(&context, 0, &e) == JP_BOOT_NOT_INITIALIZED);
	{
		JPBootLogStatus s;

		assert(jp_boot_log_get_status(NULL, &s) == JP_BOOT_ERROR_INVALID_PARAM);
		assert(jp_boot_log_get_status(&context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
		assert(jp_boot_log_get_status(&context, &s) == JP_BOOT_NOT_INITIALIZED);
	}
}

static void test_finalize(void)
{
	JPBootEvent e;
	static JPBootContext copy;

	assert(jp_boot_log_finalize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	setup(JP_ARCH_X86_64);
	assert(jp_boot_log_finalize(&context) == JP_BOOT_NOT_INITIALIZED);

	ready();
	assert(jp_boot_log_finalize(&context) == JP_BOOT_SUCCESS);
	assert(status_of().finalized);
	/* no event after the end; and calling it again changes nothing */
	e = make_event(JP_BOOT_SEVERITY_FATAL, 1);
	memcpy(&copy, &context, sizeof copy);
	assert(jp_boot_log_append(&context, &e) == JP_BOOT_FAILED);
	assert(jp_boot_log_finalize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
	assert(status_of().event_count == 1);
}

int main(void)
{
	test_initialize();
	test_initialize_other_architecture_and_stage1_state();
	test_initialize_refusals();
	test_append();
	test_append_refusals();
	test_exact_capacity();
	test_full_log_replaces_the_latest_lowest();
	test_full_log_never_loses_error_or_fatal();
	test_warning_replaces_only_lower();
	test_get();
	test_finalize();
	return 0;
}
