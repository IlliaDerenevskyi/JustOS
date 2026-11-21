/*
 * boot_log.h - the boot log of Stage 3.
 *
 * Purpose:
 *   A bounded list of the events of the current boot attempt, in the order
 *   in which they were accepted.
 *
 * Responsibilities:
 *   - jp_boot_log_initialize: clears the whole Stage 3 area, starts the first
 *     attempt, and writes event 0 (a copy of what the context says about
 *     Stage 1);
 *   - jp_boot_log_append: adds an event; when the log is full, the latest
 *     lowest-severity event is dropped (an ERROR or FATAL event never is) and
 *     the discard is counted;
 *   - jp_boot_log_get, jp_boot_log_get_status: read events and counters;
 *   - jp_boot_log_finalize: closes the log (no more events).
 *
 * Limitations:
 *   Declarations only. No allocation: the storage is inside the context.
 *
 * Public API:
 *   jp_boot_log_initialize, jp_boot_log_append, jp_boot_log_get,
 *   jp_boot_log_get_status, jp_boot_log_finalize.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, heap.
 */

#ifndef JUSTOS_BOOT_BOOT_LOG_H
#define JUSTOS_BOOT_BOOT_LOG_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_log_initialize(
	JPBootContext *context
);

/* The event_sequence and boot_attempt_id of the argument are set by the log, not by the caller. */
JPBootResult jp_boot_log_append(
	JPBootContext *context,
	const JPBootEvent *event
);

/* JP_BOOT_ERROR_INVALID_PARAM for an index at or after event_count. */
JPBootResult jp_boot_log_get(
	const JPBootContext *context,
	size_t index,
	JPBootEvent *event
);

JPBootResult jp_boot_log_get_status(
	const JPBootContext *context,
	JPBootLogStatus *status
);

JPBootResult jp_boot_log_finalize(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_BOOT_LOG_H */
