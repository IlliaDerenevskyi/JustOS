/*
 * panic.h - the terminal diagnostic state of Stage 3.
 *
 * Purpose:
 *   Shows the failure and stops the processor when recovery cannot safely
 *   continue.
 *
 * Responsibilities:
 *   - jp_boot_panic: keeps the first panic record, sends the panic lines
 *     through diagnostic_output, and stops. A second panic does not show
 *     the lines again; it only stops.
 *
 * Limitations:
 *   Declarations only. Never returns and never enables interrupts.
 *
 * Public API:
 *   jp_boot_panic.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, heap, Stage 4.
 */

#ifndef JUSTOS_BOOT_PANIC_H
#define JUSTOS_BOOT_PANIC_H

#include "justos/boot/boot_types.h"

_Noreturn void jp_boot_panic(
	JPBootContext *context,
	const JPBootError *error
);

#endif /* JUSTOS_BOOT_PANIC_H */
