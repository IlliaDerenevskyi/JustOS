/*
 * stack.h - Stage 2 stack validation.
 *
 * Purpose:
 *   Checks the stack that Stage 1 prepared. Stage 2 keeps using it.
 *
 * Responsibilities:
 *   - the KERNEL_STACK range exists and has the Stage 1 size;
 *   - the top and the current stack pointer are inside the range and aligned
 *     as the ABI of the architecture requires;
 *   - the range is writable in the active map.
 *
 * Limitations:
 *   Never changes the stack pointer and never switches to another stack.
 *
 * Public API:
 *   jp_kernel_stack_initialize.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_STACK_H
#define JUSTOS_KERNEL_STACK_H

#include "justos/boot/boot_types.h"

JPBootResult jp_kernel_stack_initialize(
	JPBootContext *context
);

#endif /* JUSTOS_KERNEL_STACK_H */
