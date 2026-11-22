/*
 * heap.h - the early kernel heap.
 *
 * Purpose:
 *   A small allocator for the kernel before any other allocator exists.
 *
 * Responsibilities:
 *   - takes one region of JP_KERNEL_EARLY_HEAP_SIZE bytes from memory;
 *   - alloc: 16-byte alignment and granularity; size 0 gives NULL; no memory
 *     left gives NULL;
 *   - free: returns a block; a pointer that is NULL or not from this heap is
 *     ignored.
 *
 * Limitations:
 *   The heap never grows and never asks for more pages.
 *
 * Public API:
 *   jp_kernel_heap_initialize, jp_kernel_heap_alloc, jp_kernel_heap_free.
 *
 * Depends on:
 *   justos/boot/boot_types.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#ifndef JUSTOS_KERNEL_HEAP_H
#define JUSTOS_KERNEL_HEAP_H

#include "justos/boot/boot_types.h"

/* 1 MiB. The only place that states the size. */
#define JP_KERNEL_EARLY_HEAP_SIZE 1048576

JPBootResult jp_kernel_heap_initialize(
	JPBootContext *context
);

void *jp_kernel_heap_alloc(
	JPBootContext *context,
	size_t size
);

void jp_kernel_heap_free(
	JPBootContext *context,
	void *ptr
);

#endif /* JUSTOS_KERNEL_HEAP_H */
