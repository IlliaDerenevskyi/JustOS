/*
 * diagnostic_output.h - where the diagnostic text goes.
 *
 * Purpose:
 *   The one owner of the choice of output: the screen (framebuffer) when its
 *   Stage 1 description is valid, and the serial port as the fallback that
 *   needs no display.
 *
 * Responsibilities:
 *   - jp_boot_diagnostic_output: sends one message of text to every sink that
 *     is available; succeeds if at least one sink accepted it.
 *
 * Limitations:
 *   Declarations only. A failed sink is counted and never starts a panic.
 *   Does not allocate and does not change ownership, mappings, or the stack.
 *
 * Public API:
 *   jp_boot_diagnostic_output.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, heap, scheduler, interrupts, Stage 4, panic, recovery.
 */

#ifndef JUSTOS_BOOT_DIAGNOSTIC_OUTPUT_H
#define JUSTOS_BOOT_DIAGNOSTIC_OUTPUT_H

#include "justos/boot/boot_types.h"

/*
 * JP_BOOT_ERROR_INVALID_PARAM for a null context, message, or text;
 * JP_BOOT_FAILED when every sink refused the message.
 */
JPBootResult jp_boot_diagnostic_output(
	JPBootContext *context,
	const JPBootDiagnosticMessage *message
);

#endif /* JUSTOS_BOOT_DIAGNOSTIC_OUTPUT_H */
