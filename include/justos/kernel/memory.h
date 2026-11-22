/*
 * memory.h - Stage 2 physical-memory ownership.
 *
 * Purpose:
 *   The one place that knows who owns every range of physical memory.
 *
 * Responsibilities:
 *   - builds the table "address range -> owner" in context->stage2 from the
 *     Stage 1 memory map, the kernel segments, the stack, the table pool, the
 *     framebuffer and the context itself;
 *   - reserve, allocate, query, release, and validate operations on it.
 *
 * Limitations:
 *   Owns physical memory only; mappings belong to paging. The table has
 *   JP_KERNEL_MEMORY_MAX_ENTRIES entries. A rejected request changes nothing.
 *
 * Public API:
 *   jp_kernel_memory_initialize, _reserve, _allocate, _query, _release,
 *   _validate_ownership.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_MEMORY_H
#define JUSTOS_KERNEL_MEMORY_H

#include "justos/boot/boot_types.h"

typedef enum JPKernelMemoryOwner {
	JP_KERNEL_MEMORY_FIRMWARE_RESERVED = 0,
	JP_KERNEL_MEMORY_UNAVAILABLE,
	JP_KERNEL_MEMORY_STAGE1_RESERVED,
	JP_KERNEL_MEMORY_KERNEL_IMAGE,
	JP_KERNEL_MEMORY_KERNEL_STACK,
	JP_KERNEL_MEMORY_PAGE_TABLES,
	JP_KERNEL_MEMORY_FRAMEBUFFER,
	JP_KERNEL_MEMORY_STAGE2_RUNTIME,
	JP_KERNEL_MEMORY_EARLY_HEAP,
	JP_KERNEL_MEMORY_AVAILABLE
} JPKernelMemoryOwner;

/* Builds the table (see "Decided rules (the ownership table and the initial owners)"). */
JPBootResult jp_kernel_memory_initialize(
	JPBootContext *context
);

/* Gives an available range to `owner` (KERNEL_IMAGE, KERNEL_STACK, PAGE_TABLES, FRAMEBUFFER, STAGE2_RUNTIME, EARLY_HEAP). */
JPBootResult jp_kernel_memory_reserve(
	JPBootContext *context,
	uint64_t base_address,
	uint64_t length,
	JPKernelMemoryOwner owner
);

/* The lowest available range at or above 1 MiB of `length` bytes, aligned to `alignment`; gives it to `owner`, returns its base. */
JPBootResult jp_kernel_memory_allocate(
	JPBootContext *context,
	uint64_t length,
	uint64_t alignment,
	JPKernelMemoryOwner owner,
	uint64_t *base_address
);

/* The owner of the whole range; JP_BOOT_NOT_DETECTED when the range is not in the table or has several owners. */
JPBootResult jp_kernel_memory_query(
	const JPBootContext *context,
	uint64_t base_address,
	uint64_t length,
	JPKernelMemoryOwner *owner
);

/* Only EARLY_HEAP ranges can be released; they become JP_KERNEL_MEMORY_AVAILABLE. */
JPBootResult jp_kernel_memory_release(
	JPBootContext *context,
	uint64_t base_address,
	uint64_t length,
	JPKernelMemoryOwner owner
);

/* Checks that the table is sorted, has no overlap, and agrees with the Stage 1 map. */
JPBootResult jp_kernel_memory_validate_ownership(
	const JPBootContext *context
);

#endif /* JUSTOS_KERNEL_MEMORY_H */
