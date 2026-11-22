/*
 * boot_pipeline.h - the Stage 1 pipeline.
 *
 * Purpose:
 *   One function that runs Stage 1 from "the environment is chosen" to "the
 *   handoff is ready". The platform entry points call it.
 *
 * Responsibilities:
 *   - call the public Stage 1 functions in the one order in which they can
 *     succeed and stop at the first failure.
 *
 * Limitations:
 *   The caller zero-fills the context and sets environment.type,
 *   environment.platform_context, and handoff.architecture first. The function
 *   does not enter the kernel; the caller does that with
 *   jp_<arch>_enter_kernel().
 *
 * Public API:
 *   jp_boot_run_pipeline.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Any firmware or architecture header.
 */

#ifndef JUSTOS_BOOT_BOOT_PIPELINE_H
#define JUSTOS_BOOT_BOOT_PIPELINE_H

#include "justos/boot/boot_types.h"

/* Returns the first failing result unchanged; JP_BOOT_SUCCESS when the handoff is READY. */
JPBootResult jp_boot_run_pipeline(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_BOOT_PIPELINE_H */
