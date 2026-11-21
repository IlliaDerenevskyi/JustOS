/*
 * boot_log.c - the boot log of Stage 3.
 *
 * Purpose:
 *   A bounded list of the events of the current boot attempt (milestone0.md,
 *   "3 Boot Log Contract" and "3 Decided rules").
 *
 * Responsibilities:
 *   - initialize: clears the whole Stage 3 area, starts attempt 1, writes
 *     event 0 (a copy of what the context says about Stage 1);
 *   - append: the retention rule when the log is full;
 *   - get, get_status, finalize.
 *
 * Limitations:
 *   Writes only the Stage 3 area. No allocation: the storage is a fixed array
 *   inside the context. The area is cleared with a loop, not memset.
 *
 * Depends on:
 *   justos/boot/boot_log.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, heap.
 */

#include "justos/boot/boot_log.h"

/* ---- Private helpers ---- */

static bool architecture_is_valid(JPArchitectureType architecture)
{
	return architecture == JP_ARCH_X86_64 || architecture == JP_ARCH_AARCH64;
}

static bool event_is_valid(const JPBootContext *context, const JPBootEvent *event)
{
	return event->stage <= JP_DIAGNOSTIC_STAGE_STAGE4 &&
	       event->severity <= JP_BOOT_SEVERITY_FATAL &&
	       event->source_component <= JP_BOOT_SOURCE_BOOT_MANAGER &&
	       event->result >= JP_BOOT_SUCCESS && event->result <= JP_BOOT_ERROR_CORRUPTED_DATA &&
	       event->architecture == context->handoff.architecture;
}

/* Clears the whole Stage 3 area (a loop over bytes: the code must not need memset). */
static void clear_area(JPBootStage3Area *area)
{
	uint8_t *bytes = (uint8_t *)area;
	size_t i;

	for (i = 0; i < sizeof *area; i++) {
		bytes[i] = 0;
	}
}

/* The index of the event that may be replaced, or JP_LOG_NO_VICTIM. */
#define JP_LOG_NO_VICTIM ((size_t)-1)

static size_t find_victim(const JPBootLogStorage *log, JPBootSeverity incoming)
{
	size_t victim = 0;
	size_t i;

	for (i = 1; i < log->status.event_count; i++) {
		if (log->events[i].severity <= log->events[victim].severity) {
			victim = i;          /* lower or equal severity, and later: the latest of the lowest */
		}
	}
	if (log->events[victim].severity >= JP_BOOT_SEVERITY_ERROR || log->events[victim].severity >= incoming) {
		return JP_LOG_NO_VICTIM;
	}
	return victim;
}

static void count_discard(JPBootLogStorage *log)
{
	log->status.discarded_event_count++;
	log->status.truncated = true;
}

/* ---- Public API ---- */

JPBootResult jp_boot_log_append(JPBootContext *context, const JPBootEvent *event)
{
	JPBootStage3Area *area;
	JPBootLogStorage *log;
	JPBootEvent stored;
	size_t at;

	if (context == NULL || event == NULL || !event_is_valid(context, event)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (!area->log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	log = &area->log;
	if (log->status.finalized) {
		return JP_BOOT_FAILED;
	}

	stored = *event;
	stored.boot_attempt_id = area->attempt.boot_attempt_id;
	stored.event_sequence = log->next_event_sequence;

	if (log->status.event_count < JP_BOOT_LOG_CAPACITY) {
		at = log->status.event_count;
	} else {
		size_t victim = find_victim(log, event->severity);

		if (victim == JP_LOG_NO_VICTIM) {
			count_discard(log);     /* the new event is dropped; its sequence number is not used */
			return JP_BOOT_SUCCESS;
		}
		for (at = victim; at + 1 < log->status.event_count; at++) {
			log->events[at] = log->events[at + 1];
		}
		count_discard(log);
		log->status.event_count--;
		at = log->status.event_count;
	}
	log->events[at] = stored;
	log->status.event_count++;
	log->next_event_sequence++;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_log_initialize(JPBootContext *context)
{
	JPBootStage3Area *area;
	JPBootEvent first;
	bool stage1_ready;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	area = &context->stage3;
	if (area->log_initialized) {
		return JP_BOOT_FAILED;
	}
	if (!architecture_is_valid(context->handoff.architecture)) {
		return JP_BOOT_FAILED;
	}

	clear_area(area);
	area->attempt.boot_attempt_id = 1;
	area->attempt.attempt_sequence = 0;
	area->attempt.retry_count = 0;
	area->attempt.maximum_retry_count = JP_BOOT_MAXIMUM_RETRY_COUNT;
	area->recovery.current_attempt_id = 1;
	area->recovery.maximum_retry_count = JP_BOOT_MAXIMUM_RETRY_COUNT;
	area->log_initialized = true;

	/* Event 0: only a copy of facts that the context holds about Stage 1. */
	stage1_ready = context->current_stage == JP_BOOT_STAGE_READY && context->handoff.state == JP_HANDOFF_STATE_READY;
	first.boot_attempt_id = 0;
	first.event_sequence = 0;
	first.stage = JP_DIAGNOSTIC_STAGE_STAGE1;
	first.severity = stage1_ready ? JP_BOOT_SEVERITY_INFO : JP_BOOT_SEVERITY_ERROR;
	first.architecture = context->handoff.architecture;
	first.source_component = JP_BOOT_SOURCE_STAGE1;
	first.result = stage1_ready ? JP_BOOT_SUCCESS : JP_BOOT_FAILED;
	first.diagnostic_context = 0;
	return jp_boot_log_append(context, &first);
}

JPBootResult jp_boot_log_get(const JPBootContext *context, size_t index, JPBootEvent *event)
{
	if (context == NULL || event == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (index >= context->stage3.log.status.event_count) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	*event = context->stage3.log.events[index];
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_log_get_status(const JPBootContext *context, JPBootLogStatus *status)
{
	if (context == NULL || status == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	*status = context->stage3.log.status;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_log_finalize(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!context->stage3.log_initialized) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	context->stage3.log.status.finalized = true;     /* idempotent */
	return JP_BOOT_SUCCESS;
}
