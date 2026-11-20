/*
 * heap.c - the early kernel heap.
 *
 * Purpose:
 *   A small allocator for the kernel before any other allocator exists. The
 *   rules are in "Decided rules (the early heap region)" of milestone0.md.
 *
 * Responsibilities:
 *   - takes one region of JP_KERNEL_EARLY_HEAP_SIZE bytes from memory.c;
 *   - keeps all its state inside that region: a 16-byte header and a chain of
 *     blocks (first fit by address, cut when the rest can hold a block, joined
 *     again on free);
 *   - alloc: 16-byte alignment, size 0 gives NULL, nothing left gives NULL;
 *   - free: ignores everything that is not a live block of this heap.
 *
 * Limitations:
 *   Never grows, never asks for more pages. The region is reached through its
 *   physical address (the identity map of Stage 2).
 *
 * Depends on:
 *   justos/kernel/heap.h, memory.h, paging.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/heap.h"

#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"

/* ---- Private constants and types ---- */

#define JP_HEAP_ALIGNMENT 16ULL
#define JP_HEAP_REGION_ALIGNMENT 4096ULL
#define JP_HEAP_MAGIC 0x4A50484541503031ULL          /* "JPHEAP01" */
#define JP_HEAP_BLOCK_FREE 0x4A5046524545424CULL     /* "JPFREEBL" */
#define JP_HEAP_BLOCK_USED 0x4A5055534544424CULL     /* "JPUSEDBL" */
#define JP_HEAP_HEADER_SIZE 16ULL
#define JP_HEAP_BLOCK_HEADER_SIZE 16ULL
#define JP_HEAP_MIN_BLOCK (JP_HEAP_BLOCK_HEADER_SIZE + JP_HEAP_ALIGNMENT)

typedef struct JPHeapHeader {
	uint64_t magic;
	uint64_t region_size;
} JPHeapHeader;

typedef struct JPHeapBlock {
	uint64_t size;          /* the whole block with this header */
	uint64_t state;         /* JP_HEAP_BLOCK_FREE or JP_HEAP_BLOCK_USED */
} JPHeapBlock;

/* ---- Private helpers ---- */

static uint64_t round_up_16(uint64_t value)
{
	return (value + JP_HEAP_ALIGNMENT - 1) & ~(JP_HEAP_ALIGNMENT - 1);
}

/* The one ownership entry of the heap; NULL if there is none or more than one. */
static const JPBootStage2MemoryEntry *find_heap_entry(const JPBootContext *context)
{
	const JPBootStage2MemoryEntry *found = NULL;
	size_t i;

	for (i = 0; i < context->stage2.entry_count && i < JP_KERNEL_MEMORY_MAX_ENTRIES; i++) {
		if (context->stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_EARLY_HEAP) {
			if (found != NULL) {
				return NULL;
			}
			found = &context->stage2.entries[i];
		}
	}
	return found;
}

/* The header of the active heap, or NULL when the heap is not active. */
static JPHeapHeader *active_header(const JPBootContext *context)
{
	const JPBootStage2MemoryEntry *entry = find_heap_entry(context);
	JPHeapHeader *header;

	if (entry == NULL || entry->length != JP_KERNEL_EARLY_HEAP_SIZE) {
		return NULL;
	}
	header = (JPHeapHeader *)(uintptr_t)entry->base_address;
	if (header->magic != JP_HEAP_MAGIC || header->region_size != entry->length) {
		return NULL;
	}
	return header;
}

static JPHeapBlock *first_block(JPHeapHeader *header)
{
	return (JPHeapBlock *)(void *)((uint8_t *)header + JP_HEAP_HEADER_SIZE);
}

static uint8_t *region_end(JPHeapHeader *header)
{
	return (uint8_t *)header + header->region_size;
}

/* The block after `block`; NULL at the end of the chain or when the chain is damaged. */
static JPHeapBlock *next_block(JPHeapHeader *header, JPHeapBlock *block)
{
	uint64_t remaining = (uint64_t)(region_end(header) - (uint8_t *)block);

	if (block->size < JP_HEAP_MIN_BLOCK || block->size % JP_HEAP_ALIGNMENT != 0 || block->size > remaining ||
	    (block->state != JP_HEAP_BLOCK_FREE && block->state != JP_HEAP_BLOCK_USED)) {
		return NULL;
	}
	if (block->size == remaining) {
		return NULL;
	}
	return (JPHeapBlock *)(void *)((uint8_t *)block + block->size);
}

/* True when the whole chain is consistent and ends exactly at the end of the region. */
static bool chain_is_valid(JPHeapHeader *header)
{
	JPHeapBlock *block = first_block(header);

	for (;;) {
		JPHeapBlock *next;
		uint64_t remaining = (uint64_t)(region_end(header) - (uint8_t *)block);

		if (block->size < JP_HEAP_MIN_BLOCK || block->size % JP_HEAP_ALIGNMENT != 0 || block->size > remaining ||
		    (block->state != JP_HEAP_BLOCK_FREE && block->state != JP_HEAP_BLOCK_USED)) {
			return false;
		}
		next = next_block(header, block);
		if (next == NULL) {
			return block->size == remaining;
		}
		block = next;
	}
}

/* Joins every run of neighbouring FREE blocks in one pass. */
static void join_free_blocks(JPHeapHeader *header)
{
	JPHeapBlock *block = first_block(header);

	while (block != NULL) {
		JPHeapBlock *next = next_block(header, block);

		if (next != NULL && block->state == JP_HEAP_BLOCK_FREE && next->state == JP_HEAP_BLOCK_FREE) {
			block->size += next->size;       /* the same block is looked at again: it may join the next one too */
			continue;
		}
		block = next;
	}
}

/* ---- Public API ---- */

JPBootResult jp_kernel_heap_initialize(JPBootContext *context)
{
	JPBootResult result;
	uint64_t base;
	JPHeapHeader *header;
	JPHeapBlock *block;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage2.entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (find_heap_entry(context) != NULL) {
		return JP_BOOT_FAILED;     /* the heap exists already */
	}
	result = jp_kernel_memory_allocate(context, JP_KERNEL_EARLY_HEAP_SIZE, JP_HEAP_REGION_ALIGNMENT,
		JP_KERNEL_MEMORY_EARLY_HEAP, &base);
	if (result != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	if (jp_kernel_paging_validate(context) != JP_BOOT_SUCCESS) {
		(void)jp_kernel_memory_release(context, base, JP_KERNEL_EARLY_HEAP_SIZE, JP_KERNEL_MEMORY_EARLY_HEAP);
		return JP_BOOT_FAILED;
	}

	/* One FREE block for the whole region, then the magic number: the heap is active only now. */
	header = (JPHeapHeader *)(uintptr_t)base;
	block = first_block(header);
	block->size = JP_KERNEL_EARLY_HEAP_SIZE - JP_HEAP_HEADER_SIZE;
	block->state = JP_HEAP_BLOCK_FREE;
	header->region_size = JP_KERNEL_EARLY_HEAP_SIZE;
	header->magic = JP_HEAP_MAGIC;
	return JP_BOOT_SUCCESS;
}

void *jp_kernel_heap_alloc(JPBootContext *context, size_t size)
{
	JPHeapHeader *header;
	JPHeapBlock *block;
	uint64_t needed;

	if (context == NULL || size == 0 || size > JP_KERNEL_EARLY_HEAP_SIZE) {
		return NULL;
	}
	header = active_header(context);
	if (header == NULL || !chain_is_valid(header)) {
		return NULL;
	}
	needed = JP_HEAP_BLOCK_HEADER_SIZE + round_up_16(size);

	for (block = first_block(header); block != NULL; block = next_block(header, block)) {
		if (block->state != JP_HEAP_BLOCK_FREE || block->size < needed) {
			continue;
		}
		if (block->size - needed >= JP_HEAP_MIN_BLOCK) {
			JPHeapBlock *rest = (JPHeapBlock *)(void *)((uint8_t *)block + needed);

			rest->size = block->size - needed;
			rest->state = JP_HEAP_BLOCK_FREE;
			block->size = needed;
		}
		block->state = JP_HEAP_BLOCK_USED;
		return (uint8_t *)block + JP_HEAP_BLOCK_HEADER_SIZE;
	}
	return NULL;
}

void jp_kernel_heap_free(JPBootContext *context, void *ptr)
{
	JPHeapHeader *header;
	JPHeapBlock *block;

	if (context == NULL || ptr == NULL) {
		return;
	}
	header = active_header(context);
	if (header == NULL || !chain_is_valid(header)) {
		return;
	}
	for (block = first_block(header); block != NULL; block = next_block(header, block)) {
		if ((uint8_t *)block + JP_HEAP_BLOCK_HEADER_SIZE == (uint8_t *)ptr) {
			if (block->state == JP_HEAP_BLOCK_USED) {
				block->state = JP_HEAP_BLOCK_FREE;
				join_free_blocks(header);
			}
			return;
		}
	}
}
