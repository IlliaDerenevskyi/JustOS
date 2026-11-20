/*
 * Unit test (level U) of src/kernel/heap.c. The real memory.c builds the
 * ownership table. The heap region is a real buffer (its address is the
 * "physical" address, as in the identity map of the kernel). The physical
 * address width and the paging check are fakes.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/heap.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"

#define PAGE 0x1000ull
#define HEAP_SIZE ((uint64_t)JP_KERNEL_EARLY_HEAP_SIZE)
#define ARENA_SIZE (4ull * 1024 * 1024)
#define GUARD 4096

/* ---- The fakes ---- */

static JPBootResult fake_paging_result;
static unsigned int paging_calls;
static bool heap_was_owned_at_paging_check;
static JPBootContext *paging_context;

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	return 48;
}

JPBootResult jp_kernel_paging_validate(JPBootContext *context)
{
	JPKernelMemoryOwner owner = JP_KERNEL_MEMORY_AVAILABLE;
	size_t i;

	paging_calls++;
	paging_context = context;
	heap_was_owned_at_paging_check = false;
	for (i = 0; i < context->stage2.entry_count; i++) {
		if (context->stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_EARLY_HEAP) {
			heap_was_owned_at_paging_check = true;
		}
	}
	(void)owner;
	return fake_paging_result;
}

/* ---- The layout ---- */

#define RES_BASE 0x4000000ull
#define RES_END 0x4400000ull
#define STACK_TOP 0x4100000ull
#define POOL_BASE 0x4200000ull
#define POOL_SIZE (128ull * PAGE)

/* The arena: guard, the memory that memory.c may give out, guard. */
static _Alignas(4096) uint8_t arena[GUARD + ARENA_SIZE + GUARD];
static JPBootContext context;

static uint8_t *arena_start(void)
{
	return arena + GUARD;
}

static void add_region(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	JPMemoryRegion *r = &context.memory_map.regions[context.memory_map.region_count++];

	r->base_address = base;
	r->length = length;
	r->type = type;
}

static void build(uint64_t arena_bytes)
{
	uint64_t context_base;
	uint64_t context_end;

	memset(arena, 0xAB, sizeof arena);              /* guards and "dirty" memory: the heap must not rely on zeroes */
	memset(&context, 0, sizeof context);
	paging_calls = 0;
	fake_paging_result = JP_BOOT_SUCCESS;
	context.memory_map.state = JP_DETECTION_STATE_DETECTED;
	add_region(RES_BASE, RES_END - RES_BASE, JP_MEMORY_REGION_RESERVED);
	if (arena_bytes != 0) {
		add_region((uint64_t)(uintptr_t)arena_start(), arena_bytes, JP_MEMORY_REGION_USABLE);
	}
	context_base = (uint64_t)(uintptr_t)&context & ~(PAGE - 1);
	context_end = ((uint64_t)(uintptr_t)&context + sizeof context + PAGE - 1) & ~(PAGE - 1);
	add_region(context_base, context_end - context_base, JP_MEMORY_REGION_RESERVED);
	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.kernel.segment_count = 1;
	context.kernel.segments[0].physical_address = RES_BASE;
	context.kernel.segments[0].memory_size = 0x3000;
	context.handoff.stack_pointer = STACK_TOP;
	context.handoff.page_table_memory_base = POOL_BASE;
	context.handoff.page_table_memory_size = POOL_SIZE;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
}

static void ready(void)
{
	build(ARENA_SIZE);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_SUCCESS);
}

static JPBootStage2MemoryEntry *heap_entry(void)
{
	size_t i;

	for (i = 0; i < context.stage2.entry_count; i++) {
		if (context.stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_EARLY_HEAP) {
			return &context.stage2.entries[i];
		}
	}
	return NULL;
}

static bool in_heap(const void *p)
{
	uintptr_t a = (uintptr_t)p;

	return a >= (uintptr_t)arena_start() && a < (uintptr_t)arena_start() + HEAP_SIZE;
}

static void guards_are_intact(void)
{
	size_t i;

	for (i = 0; i < GUARD; i++) {
		assert(arena[i] == 0xAB);
		assert(arena[GUARD + ARENA_SIZE + i] == 0xAB);
	}
	/* the memory of the arena after the heap region has not been touched either */
	for (i = HEAP_SIZE; i < ARENA_SIZE; i++) {
		assert(arena_start()[i] == 0xAB);
	}
}

/* ---- initialize ---- */

static void test_initialize(void)
{
	JPBootStage2MemoryEntry *entry;

	build(ARENA_SIZE);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_SUCCESS);
	entry = heap_entry();
	assert(entry != NULL);
	assert(entry->base_address == (uint64_t)(uintptr_t)arena_start());      /* the lowest available address */
	assert(entry->length == HEAP_SIZE);
	assert(paging_calls == 1 && paging_context == &context);
	assert(heap_was_owned_at_paging_check);                              /* the check ran after the region was taken */
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
	guards_are_intact();
}

static void test_initialize_refusals(void)
{
	static JPBootStage2Area before;

	assert(jp_kernel_heap_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	build(ARENA_SIZE);
	context.stage2.entry_count = 0;
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(paging_calls == 0);

	/* not enough available memory */
	build(HEAP_SIZE - PAGE);
	memcpy(&before, &context.stage2, sizeof before);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_FAILED);
	assert(memcmp(&before, &context.stage2, sizeof before) == 0);
	assert(paging_calls == 0);

	/* no available memory at all */
	build(0);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_FAILED);

	/* exactly enough */
	build(HEAP_SIZE);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_initialize_paging_failure(void)
{
	static JPBootStage2Area before;
	void *p;

	build(ARENA_SIZE);
	memcpy(&before, &context.stage2, sizeof before);
	fake_paging_result = JP_BOOT_FAILED;
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_FAILED);
	assert(paging_calls == 1);
	assert(memcmp(&before, &context.stage2, sizeof before) == 0);       /* the region was given back */
	assert(heap_entry() == NULL);
	p = jp_kernel_heap_alloc(&context, 16);
	assert(p == NULL);                                                  /* and the heap is not active */

	/* after the cause is gone, initialization works */
	fake_paging_result = JP_BOOT_SUCCESS;
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_heap_alloc(&context, 16) != NULL);
}

static void test_initialize_twice(void)
{
	static JPBootStage2Area before;
	void *p;

	ready();
	p = jp_kernel_heap_alloc(&context, 100);
	assert(p != NULL);
	memcpy(&before, &context.stage2, sizeof before);
	assert(jp_kernel_heap_initialize(&context) == JP_BOOT_FAILED);
	assert(memcmp(&before, &context.stage2, sizeof before) == 0);
	assert(paging_calls == 1);                                          /* the second call stopped before any work */
	/* the first heap is intact */
	memset(p, 0x11, 100);
	jp_kernel_heap_free(&context, p);
	assert(jp_kernel_heap_alloc(&context, 100) == p);
}

/* ---- alloc ---- */

static void test_alloc_before_initialize(void)
{
	build(ARENA_SIZE);
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
	jp_kernel_heap_free(&context, arena_start() + 32);                  /* does nothing, does not crash */
	assert(jp_kernel_heap_alloc(NULL, 16) == NULL);
	jp_kernel_heap_free(NULL, arena_start());
}

static void test_alloc_basic(void)
{
	static const size_t sizes[] = { 1, 2, 7, 8, 15, 16, 17, 31, 32, 33, 100, 255, 256, 1000, 4096, 5000 };
	void *pointers[sizeof sizes / sizeof sizes[0]];
	size_t i;
	size_t j;

	ready();
	assert(jp_kernel_heap_alloc(&context, 0) == NULL);                  /* zero size */
	for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
		pointers[i] = jp_kernel_heap_alloc(&context, sizes[i]);
		assert(pointers[i] != NULL);
		assert((uintptr_t)pointers[i] % 16 == 0);                      /* 16-byte alignment */
		assert(in_heap(pointers[i]));
		assert((uint8_t *)pointers[i] + sizes[i] <= arena_start() + HEAP_SIZE);
		memset(pointers[i], (int)(i + 1), sizes[i]);                    /* the whole size is usable */
	}
	/* no overlap, no damage of a neighbour */
	for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
		for (j = 0; j < sizes[i]; j++) {
			assert(((uint8_t *)pointers[i])[j] == (uint8_t)(i + 1));
		}
		for (j = i + 1; j < sizeof sizes / sizeof sizes[0]; j++) {
			assert((uint8_t *)pointers[i] + sizes[i] <= (uint8_t *)pointers[j] ||
			       (uint8_t *)pointers[j] + sizes[j] <= (uint8_t *)pointers[i]);
		}
	}
	guards_are_intact();
}

static void test_alloc_addresses_are_deterministic(void)
{
	void *a;
	void *b;
	void *c;

	ready();
	a = jp_kernel_heap_alloc(&context, 16);
	b = jp_kernel_heap_alloc(&context, 16);
	c = jp_kernel_heap_alloc(&context, 40);
	/* header 16 + header of the block 16, then payload; 16-byte blocks are 32 bytes apart */
	assert((uint8_t *)a == arena_start() + 32);
	assert((uint8_t *)b == (uint8_t *)a + 32);
	assert((uint8_t *)c == (uint8_t *)b + 32);
	/* 40 bytes rounds up to 48; the next block starts after header and payload */
	assert((uint8_t *)jp_kernel_heap_alloc(&context, 1) == (uint8_t *)c + 48 + 16);
}

static void test_alloc_limits(void)
{
	void *p;
	size_t count = 0;

	ready();
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE + 1) == NULL);
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE) == NULL);          /* the headers do not fit */
	assert(jp_kernel_heap_alloc(&context, SIZE_MAX) == NULL);
	assert(jp_kernel_heap_alloc(&context, SIZE_MAX - 15) == NULL);
	assert(jp_kernel_heap_alloc(&context, SIZE_MAX - 16) == NULL);
	assert(jp_kernel_heap_alloc(&context, (size_t)1 << 63) == NULL);
	/* the biggest single block: the region minus the heap header and the block header */
	p = jp_kernel_heap_alloc(&context, HEAP_SIZE - 32);
	assert(p != NULL);
	assert((uint8_t *)p == arena_start() + 32);
	assert((uint8_t *)p + (HEAP_SIZE - 32) == arena_start() + HEAP_SIZE);
	assert(jp_kernel_heap_alloc(&context, 1) == NULL);                  /* full */
	jp_kernel_heap_free(&context, p);
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE - 31) == NULL);     /* one byte too many */
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE - 32) == p);

	/* many small blocks until the heap is full: an exact count */
	ready();
	while ((p = jp_kernel_heap_alloc(&context, 16)) != NULL) {
		count++;
		assert(count < 100000);
	}
	assert(count == (HEAP_SIZE - 16) / 32);                              /* 32767: the last block takes 48 bytes */
	guards_are_intact();
	assert(jp_kernel_heap_alloc(&context, 1) == NULL);                  /* still full, and no growth */
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
}

static void test_split_rule(void)
{
	void *a;
	void *b;
	void *c;

	/* a free block that is 16 bytes bigger than needed is not cut (the rest could not hold a block) */
	ready();
	a = jp_kernel_heap_alloc(&context, 32);       /* block of 48 */
	b = jp_kernel_heap_alloc(&context, 16);       /* block of 32: a barrier */
	assert(a != NULL && b != NULL);
	jp_kernel_heap_free(&context, a);
	c = jp_kernel_heap_alloc(&context, 16);       /* needs 32 of the 48: remainder 16, so the whole block is given */
	assert(c == a);
	assert(jp_kernel_heap_alloc(&context, 16) == (void *)((uint8_t *)b + 32));   /* no 16-byte rest was made: the next block is after b */

	/* a free block that is 32 bytes bigger is cut: the rest is a block of its own */
	ready();
	a = jp_kernel_heap_alloc(&context, 48);       /* block of 64 */
	b = jp_kernel_heap_alloc(&context, 16);
	jp_kernel_heap_free(&context, a);
	c = jp_kernel_heap_alloc(&context, 16);       /* needs 32 of 64: remainder 32, so it is cut */
	assert(c == a);
	assert(jp_kernel_heap_alloc(&context, 16) == (void *)((uint8_t *)a + 32));
	(void)b;
}

/* ---- free ---- */

static void test_free_and_reuse(void)
{
	void *a;
	void *b;
	void *c;

	ready();
	a = jp_kernel_heap_alloc(&context, 64);
	b = jp_kernel_heap_alloc(&context, 64);
	c = jp_kernel_heap_alloc(&context, 64);
	jp_kernel_heap_free(&context, b);
	/* the first fit by address: the freed hole is the lowest free block that fits */
	assert(jp_kernel_heap_alloc(&context, 64) == b);
	assert(jp_kernel_heap_alloc(&context, 64) != b);
	(void)a;
	(void)c;
}

static void test_free_coalesces(void)
{
	void *a;
	void *b;
	void *c;
	void *d;
	void *big;

	ready();
	a = jp_kernel_heap_alloc(&context, 48);       /* blocks of 64 */
	b = jp_kernel_heap_alloc(&context, 48);
	c = jp_kernel_heap_alloc(&context, 48);
	d = jp_kernel_heap_alloc(&context, 48);       /* a barrier at the end */
	assert((uint8_t *)b == (uint8_t *)a + 64 && (uint8_t *)c == (uint8_t *)b + 64);

	/* free the outer two, then the middle one: all three join into one block of 192 */
	jp_kernel_heap_free(&context, a);
	jp_kernel_heap_free(&context, c);
	jp_kernel_heap_free(&context, b);
	big = jp_kernel_heap_alloc(&context, 192 - 16);
	assert(big == a);
	(void)d;

	/* free in the order middle, first, last: the same */
	ready();
	a = jp_kernel_heap_alloc(&context, 48);
	b = jp_kernel_heap_alloc(&context, 48);
	c = jp_kernel_heap_alloc(&context, 48);
	d = jp_kernel_heap_alloc(&context, 48);
	jp_kernel_heap_free(&context, b);
	jp_kernel_heap_free(&context, a);
	jp_kernel_heap_free(&context, c);
	assert(jp_kernel_heap_alloc(&context, 192 - 16) == a);
	(void)d;

	/* the last block joins the free rest of the region */
	ready();
	a = jp_kernel_heap_alloc(&context, 48);
	b = jp_kernel_heap_alloc(&context, 48);
	jp_kernel_heap_free(&context, b);
	jp_kernel_heap_free(&context, a);
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE - 32) == a);        /* everything is one block again */
}

static void test_free_ignores_bad_pointers(void)
{
	void *a;
	void *b;
	uint8_t *region = arena_start();
	size_t i;

	ready();
	a = jp_kernel_heap_alloc(&context, 64);
	b = jp_kernel_heap_alloc(&context, 64);

	jp_kernel_heap_free(&context, NULL);
	jp_kernel_heap_free(&context, region - 16);                         /* outside, before */
	jp_kernel_heap_free(&context, region + HEAP_SIZE);                  /* outside, after */
	jp_kernel_heap_free(&context, region + HEAP_SIZE + 4096);
	jp_kernel_heap_free(&context, region);                              /* the heap header */
	jp_kernel_heap_free(&context, region + 16);                         /* the first block header */
	jp_kernel_heap_free(&context, (uint8_t *)a + 16);                   /* in the middle of a block */
	jp_kernel_heap_free(&context, (uint8_t *)a + 1);                    /* not aligned */
	jp_kernel_heap_free(&context, (uint8_t *)a - 16);                   /* the header of a block */
	jp_kernel_heap_free(&context, (uint8_t *)b + 64);                   /* the free block after b: its header */
	jp_kernel_heap_free(&context, (uint8_t *)b + 64 + 16);              /* the payload of a block that is free */
	for (i = 0; i < 64; i++) {
		((uint8_t *)a)[i] = 0x5A;
		((uint8_t *)b)[i] = 0x5B;
	}
	/* a and b are still allocated and intact: new blocks do not overlap them */
	{
		void *c = jp_kernel_heap_alloc(&context, 64);

		assert(c != a && c != b);
		assert(((uint8_t *)a)[63] == 0x5A && ((uint8_t *)b)[63] == 0x5B);
	}

	/* a double free changes nothing */
	jp_kernel_heap_free(&context, a);
	jp_kernel_heap_free(&context, a);
	{
		void *x = jp_kernel_heap_alloc(&context, 64);
		void *y = jp_kernel_heap_alloc(&context, 64);

		assert(x == a);
		assert(y != a);                                               /* it was given out only once */
	}
	guards_are_intact();
}

/* ---- a long sequence ---- */

static void test_stress(void)
{
	enum { SLOTS = 64, ROUNDS = 5000 };
	void *slot[SLOTS];
	size_t length[SLOTS];
	uint8_t pattern[SLOTS];
	uint32_t seed = 12345;
	unsigned int round;
	size_t i;
	size_t j;

	ready();
	memset(slot, 0, sizeof slot);
	for (round = 0; round < ROUNDS; round++) {
		seed = seed * 1103515245u + 12345u;
		i = (seed >> 8) % SLOTS;
		if (slot[i] != NULL) {
			for (j = 0; j < length[i]; j++) {
				assert(((uint8_t *)slot[i])[j] == pattern[i]);
			}
			jp_kernel_heap_free(&context, slot[i]);
			slot[i] = NULL;
		} else {
			length[i] = 1 + ((seed >> 16) % 3000);
			slot[i] = jp_kernel_heap_alloc(&context, length[i]);
			assert(slot[i] != NULL);
			assert((uintptr_t)slot[i] % 16 == 0 && in_heap(slot[i]));
			pattern[i] = (uint8_t)(round | 1);
			memset(slot[i], pattern[i], length[i]);
		}
	}
	/* everything still holds its pattern, then everything is freed and the heap is one block again */
	for (i = 0; i < SLOTS; i++) {
		if (slot[i] != NULL) {
			for (j = 0; j < length[i]; j++) {
				assert(((uint8_t *)slot[i])[j] == pattern[i]);
			}
			jp_kernel_heap_free(&context, slot[i]);
		}
	}
	assert(jp_kernel_heap_alloc(&context, HEAP_SIZE - 32) == arena_start() + 32);
	guards_are_intact();
}

/* ---- a damaged heap ---- */

static void test_damaged_heap(void)
{
	uint64_t *header;
	void *p;

	/* a block size that is not a multiple of 16 */
	ready();
	p = jp_kernel_heap_alloc(&context, 16);
	header = (uint64_t *)(void *)(arena_start() + 16);
	header[0] += 8;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
	jp_kernel_heap_free(&context, p);                                   /* does not crash */

	/* a block size of zero would loop forever */
	ready();
	header = (uint64_t *)(void *)(arena_start() + 16);
	header[0] = 0;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);

	/* a block size past the end of the region */
	ready();
	header = (uint64_t *)(void *)(arena_start() + 16);
	header[0] = HEAP_SIZE;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);

	/* an unknown state marker */
	ready();
	header = (uint64_t *)(void *)(arena_start() + 16);
	header[1] ^= 0x100;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);

	/* the magic number of the heap header */
	ready();
	header = (uint64_t *)(void *)arena_start();
	header[0] ^= 1;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
	ready();
	header = (uint64_t *)(void *)arena_start();
	header[1] += 16;                                                     /* the size that the header records */
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
}

static void test_ownership_of_the_heap(void)
{
	JPBootStage2MemoryEntry *entry;

	/* a second range with the same owner makes the heap ambiguous */
	ready();
	assert(jp_kernel_memory_reserve(&context, (uint64_t)(uintptr_t)arena_start() + 2 * HEAP_SIZE, PAGE,
		JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);

	/* a heap entry of the wrong length */
	ready();
	entry = heap_entry();
	entry->length -= PAGE;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
	ready();
	entry = heap_entry();
	entry->length += PAGE;
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);

	/* the heap given back to memory: no active heap */
	ready();
	assert(jp_kernel_memory_release(&context, (uint64_t)(uintptr_t)arena_start(), HEAP_SIZE,
		JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(jp_kernel_heap_alloc(&context, 16) == NULL);
}

int main(void)
{
	test_initialize();
	test_initialize_refusals();
	test_initialize_paging_failure();
	test_initialize_twice();
	test_alloc_before_initialize();
	test_alloc_basic();
	test_alloc_addresses_are_deterministic();
	test_alloc_limits();
	test_split_rule();
	test_free_and_reuse();
	test_free_coalesces();
	test_free_ignores_bad_pointers();
	test_stress();
	test_damaged_heap();
	test_ownership_of_the_heap();
	return 0;
}
