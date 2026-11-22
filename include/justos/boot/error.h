/*
 * error.h - error classification of Stage 3.
 *
 * Purpose:
 *   Turns a JPBootResult into one immutable error record with a severity and
 *   a recoverability.
 *
 * Responsibilities:
 *   - jp_boot_error_create: validates the input, keeps the original result,
 *     applies the default severity and recoverability (and the overrides that
 *     do not weaken a FATAL or NON_RECOVERABLE result), stores one record for
 *     the current attempt;
 *   - jp_boot_error_get: reads the record;
 *   - jp_boot_error_clear: removes it for a new attempt (only when the retry
 *     contract allows it).
 *
 * Limitations:
 *   Declarations only. The types are in diagnostic_types.h (included by
 *   boot_types.h). The original result is never replaced.
 *
 * Public API:
 *   jp_boot_error_create, jp_boot_error_get, jp_boot_error_clear.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 4.
 */

#ifndef JUSTOS_BOOT_ERROR_H
#define JUSTOS_BOOT_ERROR_H

#include "justos/boot/boot_types.h"

/*
 * JP_BOOT_ERROR_INVALID_PARAM for a null pointer or a result that is
 * JP_BOOT_SUCCESS; JP_BOOT_NOT_INITIALIZED before the log and status exist;
 * JP_BOOT_FAILED when an error is already recorded for this attempt (the
 * first one stays) or the report is final.
 */
JPBootResult jp_boot_error_create(
	JPBootContext *context,
	const JPBootErrorInput *input
);

/* JP_BOOT_NOT_DETECTED when no error was recorded. */
JPBootResult jp_boot_error_get(
	const JPBootContext *context,
	JPBootError *error
);

JPBootResult jp_boot_error_clear(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_ERROR_H */
