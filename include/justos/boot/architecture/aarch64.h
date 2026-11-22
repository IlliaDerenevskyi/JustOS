/*
 * aarch64.h - AArch64 architecture contract of Stage 1.
 *
 * Purpose:
 *   Border between generic Stage 1 code and AArch64 machine-dependent work.
 *
 * Responsibilities:
 *   - named entry-state constants;
 *   - the operations generic code may ask of a AArch64 machine.
 *
 * Limitations:
 *   Declarations only. No firmware (UEFI/BIOS) logic. The operations write
 *   only the handoff fields they establish and never set
 *   JP_HANDOFF_STATE_READY.
 *
 * Public API:
 *   jp_aarch64_validate_entry_state, _setup_paging, _prepare_entry,
 *   _validate_elf_machine, _discover_cpu, _map_segment,
 *   _phys_access, _enter_kernel.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   Provider headers.
 */

#ifndef JUSTOS_BOOT_ARCHITECTURE_AARCH64_H
#define JUSTOS_BOOT_ARCHITECTURE_AARCH64_H

#include "justos/boot/boot_types.h"

#define JP_AARCH64_STACK_ALIGNMENT 16

/* The page tables live in one block of this many pages (see milestone0.md). */
#define JP_AARCH64_TABLE_POOL_PAGES 128

/* How many places are tried for the table pool when the firmware refuses a reservation. */
#define JP_AARCH64_POOL_ATTEMPTS 16

/* A RESERVED or MMIO region of at least this size is not part of the identity map. */
#define JP_AARCH64_IDENTITY_RESERVED_MAX 0x40000000ULL

/* Checks every entry-state requirement of this architecture; changes nothing. */
JPBootResult jp_aarch64_validate_entry_state(
	const JPBootContext *context
);

/* Builds the boot-time paging structures; sets page_table_root and paging_enabled. */
JPBootResult jp_aarch64_setup_paging(
	JPBootContext *context
);

/* Records stack pointer, entry address, architecture and interrupt state in handoff. */
JPBootResult jp_aarch64_prepare_entry(
	JPBootContext *context
);

/* Succeeds only if the ELF machine matches JP_ELF_MACHINE_AARCH64. */
JPBootResult jp_aarch64_validate_elf_machine(
	const JPKernelHeader *header
);

/* Fills cpu from what the processor reports; DETECTED only after a real read. */
JPBootResult jp_aarch64_discover_cpu(
	JPCpuInfo *cpu
);

/* Maps kernel.segments[index]; SUCCESS only after the mapping really exists. */
JPBootResult jp_aarch64_map_segment(
	JPBootContext *context,
	size_t index
);

/* Pointer for reading and writing a physical range now, or NULL if impossible. */
void *jp_aarch64_phys_access(
	JPBootContext *context,
	uint64_t physical,
	uint64_t length
);

/*
 * The final step of Stage 1: loads the page-table root, switches to the
 * kernel stack, and jumps to the kernel entry with the context. Called only
 * when the handoff is READY. Never returns.
 */
_Noreturn void jp_aarch64_enter_kernel(
	JPBootContext *context
);

#endif /* JUSTOS_BOOT_ARCHITECTURE_AARCH64_H */
