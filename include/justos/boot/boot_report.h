/*
 * boot_report.h - the Boot Report and the finalization of Stage 3.
 *
 * Purpose:
 *   Freezes the diagnostic state and gives Stage 4 a read-only view of it.
 *
 * Responsibilities:
 *   - jp_boot_report_finalize: checks that status, log, error, and recovery
 *     agree, closes the log, and publishes the report; calling it again
 *     returns the same report;
 *   - jp_boot_report_get: reads the report (only after finalization).
 *
 * Limitations:
 *   Declarations only. Does not allocate, enable interrupts, or touch Stage 1
 *   and Stage 2 state.
 *
 * Public API:
 *   jp_boot_report_finalize, jp_boot_report_get.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, heap, Stage 4.
 */

#ifndef JUSTOS_BOOT_BOOT_REPORT_H
#define JUSTOS_BOOT_BOOT_REPORT_H

#include "justos/boot/boot_types.h"

JPBootResult jp_boot_report_finalize(
	JPBootContext *context
);

/* JP_BOOT_NOT_INITIALIZED before the report is final. */
JPBootResult jp_boot_report_get(
	const JPBootContext *context,
	JPBootReport *report
);

#endif /* JUSTOS_BOOT_BOOT_REPORT_H */
