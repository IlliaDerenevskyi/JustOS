/*
 * paging.h - Stage 2 paging.
 *
 * Purpose:
 *   Replaces the writable-and-executable identity map of Stage 1 by a map
 *   with correct permissions at the same addresses.
 *
 * Responsibilities:
 *   - builds the new tables from the unused pages of the Stage 1 table pool;
 *   - switches the root register (x86_64 CR3; AArch64 TTBR0_EL1) and
 *     synchronizes the TLB;
 *   - AArch64: sets SCTLR_EL1.WXN after the switch;
 *   - checks the active map.
 *
 * Limitations:
 *   Owns mappings, not physical pages (memory owns them). Does not allocate
 *   any other memory.
 *
 * Public API:
 *   jp_kernel_paging_initialize, jp_kernel_paging_validate.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_PAGING_H
#define JUSTOS_KERNEL_PAGING_H

#include "justos/boot/boot_types.h"

JPBootResult jp_kernel_paging_initialize(
	JPBootContext *context
);

/*
 * Success only if the active map matches the rules: every KERNEL_STACK,
 * PAGE_TABLES, EARLY_HEAP and AVAILABLE range is read-write and not
 * executable, and every kernel segment has the permissions of its ELF flags.
 */
JPBootResult jp_kernel_paging_validate(
	JPBootContext *context
);

#endif /* JUSTOS_KERNEL_PAGING_H */
