/*
 * Unit test (level U) of src/kernel/memory.c: the ownership table, its
 * initial owners, and the operations reserve, allocate, query, release, and
 * validate_ownership. The processor hook is replaced by a fake.
 */

#define _GNU_SOURCE   /* MAP_ANONYMOUS and MAP_FIXED_NOREPLACE, used by fixed_context.h */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/memory.h"
#include "../../support/fixed_context.h"

#define PAGE 0x1000ull
#define MIB 0x100000ull

static uint32_t fake_address_bits = 48;

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	return fake_address_bits;
}

/* The context must be a real object: its own address is a range of the map. It lives at a fixed address (see
 * fixed_context.h), and the name "context" stands for the object, so the tests read as before. */
static JPBootContext *context_object;
#define context (*context_object)

/* Synthetic layout. */
#define LOW_BASE (0x1000ull)
#define LOW_END (0x100000ull)
#define MAIN_BASE (0x100000ull)
#define MAIN_END (0x4000000ull)
#define RES_BASE (0x4000000ull)
#define RES_END (0x4400000ull)
#define HIGH_BASE (0x4400000ull)
#define HIGH_END (0x8000000ull)
#define ACPI_BASE (0x9000000ull)
#define ACPI_END (0x9100000ull)
#define BAD_BASE (0xA000000ull)
#define BAD_END (0xA001000ull)
#define MMIO_BASE (0xFEE00000ull)
#define MMIO_END (0xFEE01000ull)
#define IMAGE_BASE (0x4000000ull)
#define IMAGE_SIZE (0x3000ull)
#define STACK_TOP (0x4100000ull)
#define POOL_BASE (0x4200000ull)
#define POOL_SIZE ((128ull * 0x1000ull))
#define FB_BASE (0xFD000000ull)

static uint64_t align_down(uint64_t v)
{
	return v & ~(PAGE - 1);
}

static uint64_t align_up(uint64_t v)
{
	return align_down(v + PAGE - 1);
}

static void add_region(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	JPMemoryRegion *r = &context.memory_map.regions[context.memory_map.region_count++];

	r->base_address = base;
	r->length = length;
	r->type = type;
}

static uint64_t context_base(void)
{
	return align_down((uint64_t)(uintptr_t)&context);
}

static uint64_t context_end(void)
{
	return align_up((uint64_t)(uintptr_t)&context + sizeof context);
}

/* A context that Stage 1 could have produced. */
static void setup(void)
{
	memset(&context, 0, sizeof context);
	memset(&context.stage2, 0xA5, sizeof context.stage2);   /* garbage: initialize must clear it */
	fake_address_bits = 48;

	context.memory_map.state = JP_DETECTION_STATE_DETECTED;
	add_region(LOW_BASE, LOW_END - LOW_BASE, JP_MEMORY_REGION_USABLE);
	add_region(MAIN_BASE, MAIN_END - MAIN_BASE, JP_MEMORY_REGION_USABLE);
	add_region(RES_BASE, RES_END - RES_BASE, JP_MEMORY_REGION_RESERVED);
	add_region(HIGH_BASE, HIGH_END - HIGH_BASE, JP_MEMORY_REGION_USABLE);
	add_region(ACPI_BASE, ACPI_END - ACPI_BASE, JP_MEMORY_REGION_ACPI);
	add_region(BAD_BASE, BAD_END - BAD_BASE, JP_MEMORY_REGION_UNUSABLE);
	add_region(MMIO_BASE, MMIO_END - MMIO_BASE, JP_MEMORY_REGION_MMIO);
	add_region(context_base(), context_end() - context_base(), JP_MEMORY_REGION_RESERVED);

	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.kernel.segment_count = 1;
	context.kernel.segments[0].physical_address = IMAGE_BASE;
	context.kernel.segments[0].memory_size = IMAGE_SIZE;

	context.handoff.stack_pointer = STACK_TOP;
	context.handoff.page_table_memory_base = POOL_BASE;
	context.handoff.page_table_memory_size = POOL_SIZE;
	context.handoff.page_table_memory_used = 5;

	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.base_address = FB_BASE;
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 768;
}

static JPKernelMemoryOwner owner_of(uint64_t address)
{
	JPKernelMemoryOwner owner = JP_KERNEL_MEMORY_AVAILABLE;

	assert(jp_kernel_memory_query(&context, address, 1, &owner) == JP_BOOT_SUCCESS);
	return owner;
}

static void ready(void)
{
	setup();
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
}

/* ---- initialize: checks ---- */

static void test_initialize_checks(void)
{
	setup();
	assert(jp_kernel_memory_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	setup();
	context.memory_map.state = JP_DETECTION_STATE_UNKNOWN;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
	setup();
	context.memory_map.region_count = 0;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
	setup();
	context.memory_map.region_count = JP_MAX_MEMORY_REGIONS + 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
	setup();
	context.kernel.load_state = JP_KERNEL_LOAD_NOT_INITIALIZED;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
	setup();
	context.kernel.segment_count = 0;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
}

/* Appends one more region to a good map: a damaged region that comes last must still be found. */
static void add_last(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	add_region(base, length, type);
}

static void test_initialize_map_errors(void)
{
	setup();
	add_last(0x20000000, 0, JP_MEMORY_REGION_USABLE);                      /* zero length */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	add_last(UINT64_MAX - 0x1000, 0x2000, JP_MEMORY_REGION_USABLE);        /* base + length overflows */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	add_last(MAIN_BASE + PAGE, PAGE, JP_MEMORY_REGION_USABLE);             /* inside the region before it */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	add_last(0x8F00000, 0x101000, JP_MEMORY_REGION_USABLE);                /* ends inside the next region */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	add_last(MAIN_BASE, PAGE, JP_MEMORY_REGION_USABLE);                    /* the same base as another region */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	fake_address_bits = 47;
	add_last(1ull << 47, PAGE, JP_MEMORY_REGION_USABLE);                   /* one page above the limit */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	fake_address_bits = 47;
	add_last((1ull << 47) - PAGE, PAGE, JP_MEMORY_REGION_USABLE);          /* ends exactly at the limit */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);

	setup();
	fake_address_bits = 64;                                                 /* no limit at all */
	add_last(0xFFFF000000000000ull, 0x10000, JP_MEMORY_REGION_USABLE);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);

	setup();
	fake_address_bits = 28;                                                 /* 256 MiB: the MMIO region is above */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();
	fake_address_bits = 0;                                                  /* unknown width */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);
}

/* The limits of the counts. */
static void test_initialize_limits(void)
{
	size_t i;

	/* a map of one region that holds everything */
	setup();
	context.memory_map.region_count = 0;
	add_region(PAGE, (1ull << 47) - PAGE, JP_MEMORY_REGION_RESERVED);
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(IMAGE_BASE) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(context_base()) == JP_KERNEL_MEMORY_STAGE2_RUNTIME);
	assert(owner_of(PAGE) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	setup();
	for (i = context.memory_map.region_count; i < JP_MAX_MEMORY_REGIONS; i++) {
		add_region(0x20000000 + i * 0x2000, 0x1000, JP_MEMORY_REGION_USABLE);
	}
	assert(context.memory_map.region_count == JP_MAX_MEMORY_REGIONS);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);

	/* the maximum number of kernel segments, one page each */
	setup();
	context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS;
	for (i = 0; i < JP_MAX_KERNEL_SEGMENTS; i++) {
		context.kernel.segments[i].physical_address = IMAGE_BASE + i * PAGE;
		context.kernel.segments[i].memory_size = PAGE;
	}
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(IMAGE_BASE + JP_MAX_KERNEL_SEGMENTS * PAGE - 1) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + JP_MAX_KERNEL_SEGMENTS * PAGE) == JP_KERNEL_MEMORY_STAGE1_RESERVED);
	setup();
	context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS + 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_FAILED);

	/* the stack at the very bottom of memory: its range starts at address 0 */
	setup();
	context.memory_map.regions[0].base_address = 0;
	context.memory_map.regions[0].length = LOW_END;
	context.memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	context.handoff.stack_pointer = JP_BOOT_KERNEL_STACK_SIZE;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(0) == JP_KERNEL_MEMORY_KERNEL_STACK);
}

/* ---- initialize: the table ---- */

static void test_initial_owners(void)
{
	ready();

	assert(owner_of(LOW_BASE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(MAIN_BASE + 0x500000) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(HIGH_BASE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(ACPI_BASE) == JP_KERNEL_MEMORY_FIRMWARE_RESERVED);
	assert(owner_of(MMIO_BASE) == JP_KERNEL_MEMORY_FIRMWARE_RESERVED);
	assert(owner_of(BAD_BASE) == JP_KERNEL_MEMORY_UNAVAILABLE);

	assert(owner_of(IMAGE_BASE) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + IMAGE_SIZE - 1) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + IMAGE_SIZE) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	assert(owner_of(STACK_TOP - JP_BOOT_KERNEL_STACK_SIZE) == JP_KERNEL_MEMORY_KERNEL_STACK);
	assert(owner_of(STACK_TOP - 1) == JP_KERNEL_MEMORY_KERNEL_STACK);
	assert(owner_of(STACK_TOP - JP_BOOT_KERNEL_STACK_SIZE - 1) == JP_KERNEL_MEMORY_STAGE1_RESERVED);
	assert(owner_of(STACK_TOP) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	assert(owner_of(POOL_BASE) == JP_KERNEL_MEMORY_PAGE_TABLES);
	assert(owner_of(POOL_BASE + 4 * PAGE) == JP_KERNEL_MEMORY_PAGE_TABLES);       /* a used page */
	assert(owner_of(POOL_BASE + POOL_SIZE - 1) == JP_KERNEL_MEMORY_PAGE_TABLES);  /* an unused page */
	assert(owner_of(POOL_BASE + POOL_SIZE) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	assert(owner_of(context_base()) == JP_KERNEL_MEMORY_STAGE2_RUNTIME);
	assert(owner_of(context_end() - 1) == JP_KERNEL_MEMORY_STAGE2_RUNTIME);

	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
}

static void test_table_shape(void)
{
	size_t i;

	ready();
	assert(context.stage2.entry_count > 0);
	assert(context.stage2.entry_count <= JP_KERNEL_MEMORY_MAX_ENTRIES);
	for (i = 0; i < context.stage2.entry_count; i++) {
		const JPBootStage2MemoryEntry *e = &context.stage2.entries[i];

		assert(e->length > 0);
		if (i > 0) {
			const JPBootStage2MemoryEntry *p = &context.stage2.entries[i - 1];

			assert(p->base_address + p->length <= e->base_address);
			if (p->base_address + p->length == e->base_address) {
				assert(p->owner != e->owner);
			}
		}
	}
}

static void test_initialize_clears_the_area(void)
{
	JPBootStage2MemoryEntry *entries;

	ready();
	entries = context.stage2.entries;
	/* An entry after the last one must be zero: the garbage of setup() is gone. */
	assert(entries[context.stage2.entry_count].length == 0);
	assert(entries[JP_KERNEL_MEMORY_MAX_ENTRIES - 1].owner == 0);
}

static void test_segment_rounding_and_sharing(void)
{
	setup();
	context.kernel.segments[0].memory_size = 0x2800;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(IMAGE_BASE + 0x2FFF) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + 0x3000) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	/* Two segments in one page, and a segment right after the first. */
	setup();
	context.kernel.segment_count = 3;
	context.kernel.segments[0].memory_size = 0x800;
	context.kernel.segments[1].physical_address = IMAGE_BASE + 0x800;
	context.kernel.segments[1].memory_size = 0x800;
	context.kernel.segments[2].physical_address = IMAGE_BASE + 0x1000;
	context.kernel.segments[2].memory_size = 0x1000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(IMAGE_BASE) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + 0x1FFF) == JP_KERNEL_MEMORY_KERNEL_IMAGE);
	assert(owner_of(IMAGE_BASE + 0x2000) == JP_KERNEL_MEMORY_STAGE1_RESERVED);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
}

static void test_cut_errors(void)
{
	setup();   /* a segment inside available memory */
	context.kernel.segments[0].physical_address = MAIN_BASE;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* a segment with zero size */
	context.kernel.segments[0].memory_size = 0;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* a segment whose end overflows */
	context.kernel.segments[0].physical_address = UINT64_MAX - 0x100;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* a segment whose end overflows only when it is rounded up to a page */
	context.kernel.segments[0].physical_address = UINT64_MAX - 0x2000;
	context.kernel.segments[0].memory_size = 0x1800;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the stack inside available memory */
	context.handoff.stack_pointer = MAIN_BASE + 0x100000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the stack below its size: the range would start below zero */
	context.handoff.stack_pointer = JP_BOOT_KERNEL_STACK_SIZE - PAGE;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the pool is empty */
	context.handoff.page_table_memory_size = 0;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the pool is not page aligned */
	context.handoff.page_table_memory_base = POOL_BASE + 0x10;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the pool overlaps the stack */
	context.handoff.page_table_memory_base = STACK_TOP - 0x2000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	setup();   /* the pool leaves the reserved region */
	context.handoff.page_table_memory_base = RES_END - 0x2000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_framebuffer_cases(void)
{
	JPKernelMemoryOwner owner;

	/* (a) outside the table: skipped */
	ready();
	assert(jp_kernel_memory_query(&context, FB_BASE, 1, &owner) == JP_BOOT_NOT_DETECTED);

	/* (b) inside reserved memory: cut out */
	setup();
	context.framebuffer.base_address = RES_BASE + 0x300000;
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 16;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(RES_BASE + 0x300000) == JP_KERNEL_MEMORY_FRAMEBUFFER);
	assert(owner_of(RES_BASE + 0x300000 + 16 * 4096 - 1) == JP_KERNEL_MEMORY_FRAMEBUFFER);
	assert(owner_of(RES_BASE + 0x300000 + 16 * 4096) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	/* (c) in available memory, or half in the table: refused */
	setup();
	context.framebuffer.base_address = MAIN_BASE + 0x200000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	setup();
	context.framebuffer.base_address = RES_END - 0x1000;   /* the rest is in available memory */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* inside a firmware region (ACPI, MMIO, firmware): it stays owned by the firmware */
	setup();
	context.framebuffer.base_address = ACPI_BASE;
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 16;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(ACPI_BASE) == JP_KERNEL_MEMORY_FIRMWARE_RESERVED);
	assert(owner_of(ACPI_BASE + 16 * 4096 - 1) == JP_KERNEL_MEMORY_FIRMWARE_RESERVED);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
	setup();
	context.framebuffer.base_address = MMIO_BASE;
	context.framebuffer.pitch = 1024;
	context.framebuffer.height = 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(MMIO_BASE) == JP_KERNEL_MEMORY_FIRMWARE_RESERVED);
	/* partly in a firmware region and partly in an unusable one, or running out of the firmware region */
	setup();
	context.framebuffer.base_address = ACPI_END - PAGE;      /* the rest is in a hole: outside the table, but part is inside */
	context.framebuffer.pitch = 8192;
	context.framebuffer.height = 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	setup();
	context.framebuffer.base_address = BAD_BASE;              /* unusable memory */
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* touching an entry without overlapping it: still outside the table */
	setup();
	context.framebuffer.base_address = MMIO_END;      /* right after the last entry */
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 4;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_query(&context, MMIO_END, 1, &owner) == JP_BOOT_NOT_DETECTED);
	setup();
	context.framebuffer.base_address = BAD_BASE - 4 * PAGE;   /* ends where an entry begins */
	context.framebuffer.pitch = 4096;
	context.framebuffer.height = 4;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_query(&context, BAD_BASE - 1, 1, &owner) == JP_BOOT_NOT_DETECTED);

	/* not detected: ignored even inside reserved memory */
	setup();
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	context.framebuffer.base_address = RES_BASE + 0x300000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(RES_BASE + 0x300000) == JP_KERNEL_MEMORY_STAGE1_RESERVED);

	/* a detected framebuffer of zero size is a broken handoff */
	setup();
	context.framebuffer.height = 0;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_region_order_does_not_matter(void)
{
	static JPBootStage2Area first;
	JPMemoryRegion tmp;
	size_t i;
	size_t count;

	ready();
	memcpy(&first, &context.stage2, sizeof first);

	setup();
	count = context.memory_map.region_count;
	for (i = 0; i < count / 2; i++) {
		tmp = context.memory_map.regions[i];
		context.memory_map.regions[i] = context.memory_map.regions[count - 1 - i];
		context.memory_map.regions[count - 1 - i] = tmp;
	}
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&first, &context.stage2, sizeof first) == 0);
}

static void test_initialize_twice(void)
{
	static JPBootStage2Area first;

	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	memcpy(&first, &context.stage2, sizeof first);
	/* A second initialize starts again from the Stage 1 state: the reservation is gone. */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(memcmp(&first, &context.stage2, sizeof first) != 0);
}

/* ---- operations before initialize ---- */

static void test_not_initialized(void)
{
	JPKernelMemoryOwner owner;
	uint64_t base;

	setup();
	context.stage2.entry_count = 0;
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_kernel_memory_query(&context, MAIN_BASE, 1, &owner) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_NOT_INITIALIZED);
}

/* ---- reserve ---- */

static void test_reserve(void)
{
	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE + 0x10000, 0x20000, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE + 0x10000 - 1) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(MAIN_BASE + 0x10000) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + 0x30000 - 1) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + 0x30000) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);

	/* a duplicate is refused, even by the same owner */
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE + 0x10000, 0x20000, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	/* two reservations that touch with the same owner become one entry */
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE + 0x30000, 0x1000, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE + 0x30000) == JP_KERNEL_MEMORY_EARLY_HEAP);
}

static void test_reserve_whole_region_and_edges(void)
{
	/* The first page of a region and the last page of a region. */
	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_reserve(&context, MAIN_END - PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + PAGE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(MAIN_END - 1) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
}

static void test_reserve_refusals(void)
{
	static JPBootStage2Area before;

	ready();
	memcpy(&before, &context.stage2, sizeof before);

	assert(jp_kernel_memory_reserve(NULL, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, 0, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE + 1, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE + 1, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, UINT64_MAX - PAGE + 1, PAGE * 2, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	/* owners that cannot be requested */
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_AVAILABLE) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_FIRMWARE_RESERVED) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_UNAVAILABLE) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_STAGE1_RESERVED) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, PAGE, (JPKernelMemoryOwner)99) == JP_BOOT_ERROR_INVALID_PARAM);
	/* not available: reserved, firmware, unavailable, outside the table, partly available */
	assert(jp_kernel_memory_reserve(&context, RES_BASE + 0x300000, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_reserve(&context, ACPI_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_reserve(&context, BAD_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_reserve(&context, 0x20000000, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_reserve(&context, MAIN_END - PAGE, 2 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_reserve(&context, IMAGE_BASE, PAGE, JP_KERNEL_MEMORY_KERNEL_IMAGE) == JP_BOOT_FAILED);

	/* nothing was committed by any refused request */
	assert(memcmp(&before, &context.stage2, sizeof before) == 0);
}

/* ---- allocate ---- */

static void test_allocate(void)
{
	uint64_t base = 0;

	ready();
	/* the lowest address at or above 1 MiB, not the low region below it */
	assert(jp_kernel_memory_allocate(&context, MIB, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_SUCCESS);
	assert(base == MAIN_BASE);
	assert(owner_of(MAIN_BASE) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + MIB - 1) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + MIB) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(LOW_BASE) == JP_KERNEL_MEMORY_AVAILABLE);

	/* the next one follows */
	assert(jp_kernel_memory_allocate(&context, 2 * PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME, &base) == JP_BOOT_SUCCESS);
	assert(base == MAIN_BASE + MIB);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
}

static void test_allocate_alignment_and_gaps(void)
{
	uint64_t base = 0;

	ready();
	/* alignment 2 MiB: the first multiple of 2 MiB at or above 1 MiB with room */
	assert(jp_kernel_memory_allocate(&context, PAGE, 2 * MIB, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_SUCCESS);
	assert(base == 2 * MIB);

	/* a hole that is too small is skipped */
	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE + 0x2000, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(jp_kernel_memory_allocate(&context, 4 * PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME, &base) == JP_BOOT_SUCCESS);
	assert(base == MAIN_BASE + 0x3000);
	/* a hole of exactly the right size is used */
	assert(jp_kernel_memory_allocate(&context, 2 * PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME, &base) == JP_BOOT_SUCCESS);
	assert(base == MAIN_BASE);

	/* a range never crosses into reserved memory: the available memory of one region ends at RES_BASE */
	ready();
	assert(jp_kernel_memory_allocate(&context, MAIN_END - MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_SUCCESS);
	assert(base == MAIN_BASE);
	assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_SUCCESS);
	assert(base == HIGH_BASE);   /* the main region is full; the next available memory is after the reserved one */
}

static void test_allocate_refusals(void)
{
	static JPBootStage2Area before;
	uint64_t base = 0x1234;

	ready();
	memcpy(&before, &context.stage2, sizeof before);

	assert(jp_kernel_memory_allocate(NULL, PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, 0, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, PAGE + 1, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, PAGE, 0, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, PAGE, 3 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);   /* not a power of two */
	assert(jp_kernel_memory_allocate(&context, PAGE, 0x800, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_ERROR_INVALID_PARAM);       /* below the page size */
	assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_AVAILABLE, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_STAGE1_RESERVED, &base) == JP_BOOT_ERROR_INVALID_PARAM);
	/* too big for any available range */
	assert(jp_kernel_memory_allocate(&context, 0x100000000ull, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_FAILED);
	/* an alignment so large that the search would overflow */
	assert(jp_kernel_memory_allocate(&context, PAGE, 1ull << 63, JP_KERNEL_MEMORY_EARLY_HEAP, &base) == JP_BOOT_FAILED);

	assert(base == 0x1234);   /* never written on failure */
	assert(memcmp(&before, &context.stage2, sizeof before) == 0);
}

/* ---- query ---- */

static void test_query(void)
{
	JPKernelMemoryOwner owner = JP_KERNEL_MEMORY_AVAILABLE;

	ready();
	assert(jp_kernel_memory_query(NULL, MAIN_BASE, 1, &owner) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_query(&context, MAIN_BASE, 1, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_query(&context, MAIN_BASE, 0, &owner) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_query(&context, UINT64_MAX, 2, &owner) == JP_BOOT_ERROR_INVALID_PARAM);

	/* one owner over a long range: success */
	assert(jp_kernel_memory_query(&context, MAIN_BASE, MAIN_END - MAIN_BASE, &owner) == JP_BOOT_SUCCESS);
	assert(owner == JP_KERNEL_MEMORY_AVAILABLE);

	/* two owners: not detected, and the output is not written */
	owner = JP_KERNEL_MEMORY_UNAVAILABLE;
	assert(jp_kernel_memory_query(&context, IMAGE_BASE - PAGE, 2 * PAGE, &owner) == JP_BOOT_NOT_DETECTED);
	assert(owner == JP_KERNEL_MEMORY_UNAVAILABLE);
	/* partly outside the table */
	assert(jp_kernel_memory_query(&context, HIGH_END - 1, 2, &owner) == JP_BOOT_NOT_DETECTED);
	/* in a hole between regions */
	assert(jp_kernel_memory_query(&context, 0x8000000, 1, &owner) == JP_BOOT_NOT_DETECTED);
	/* a range that spans a hole: usable, hole, usable */
	assert(jp_kernel_memory_query(&context, HIGH_END - PAGE, 0x2000, &owner) == JP_BOOT_NOT_DETECTED);
	/* two touching regions of the same owner (low and main are both available) */
	assert(jp_kernel_memory_query(&context, LOW_END - PAGE, 2 * PAGE, &owner) == JP_BOOT_SUCCESS);
	assert(owner == JP_KERNEL_MEMORY_AVAILABLE);
}

/* ---- release ---- */

static void test_release(void)
{
	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, 4 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);

	/* a part of a heap range can be released */
	assert(jp_kernel_memory_release(&context, MAIN_BASE + PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(owner_of(MAIN_BASE + PAGE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(owner_of(MAIN_BASE + 2 * PAGE) == JP_KERNEL_MEMORY_EARLY_HEAP);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);

	/* it is available again: an allocation can take it */
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	assert(owner_of(MAIN_BASE) == JP_KERNEL_MEMORY_AVAILABLE);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
}

static void test_release_refusals(void)
{
	static JPBootStage2Area before;

	ready();
	assert(jp_kernel_memory_reserve(&context, MAIN_BASE, 2 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
	memcpy(&before, &context.stage2, sizeof before);

	assert(jp_kernel_memory_release(NULL, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, MAIN_BASE, 0, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE, (JPKernelMemoryOwner)99) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE, (JPKernelMemoryOwner)10) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, UINT64_MAX - PAGE + 1, 2 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, MAIN_BASE + 1, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE + 1, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_ERROR_INVALID_PARAM);
	/* the other owners are never given back */
	assert(jp_kernel_memory_release(&context, IMAGE_BASE, PAGE, JP_KERNEL_MEMORY_KERNEL_IMAGE) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_release(&context, STACK_TOP - PAGE, PAGE, JP_KERNEL_MEMORY_KERNEL_STACK) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_release(&context, POOL_BASE, PAGE, JP_KERNEL_MEMORY_PAGE_TABLES) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_release(&context, context_base(), PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME) == JP_BOOT_FAILED);
	/* a range that is not (entirely) owned by the heap */
	assert(jp_kernel_memory_release(&context, MAIN_BASE + PAGE, 2 * PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_release(&context, MAIN_BASE + 0x10000, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	assert(jp_kernel_memory_release(&context, 0x20000000, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_FAILED);
	/* the owner given must be the owner of the range */
	assert(jp_kernel_memory_release(&context, MAIN_BASE, PAGE, JP_KERNEL_MEMORY_PAGE_TABLES) == JP_BOOT_FAILED);

	assert(memcmp(&before, &context.stage2, sizeof before) == 0);
}

/* ---- capacity ---- */

static void run_capacity(bool flip_parity)
{
	static JPBootStage2Area before;
	uint64_t address = MAIN_BASE;
	size_t guard = 0;

	ready();
	if (flip_parity) {   /* one more entry, so that the table fills up on the other parity */
		assert(jp_kernel_memory_reserve(&context, HIGH_END - PAGE, PAGE, JP_KERNEL_MEMORY_KERNEL_STACK) == JP_BOOT_SUCCESS);
	}
	/* every second page: each reservation adds two entries until the table is full */
	for (;;) {
		JPBootResult result;
		size_t count = context.stage2.entry_count;

		memcpy(&before, &context.stage2, sizeof before);
		result = jp_kernel_memory_reserve(&context, address, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP);
		if (result != JP_BOOT_SUCCESS) {
			assert(result == JP_BOOT_FAILED);
			assert(memcmp(&before, &context.stage2, sizeof before) == 0);
			assert(count + 2 > JP_KERNEL_MEMORY_MAX_ENTRIES);
			break;
		}
		assert(context.stage2.entry_count <= JP_KERNEL_MEMORY_MAX_ENTRIES);
		address += 2 * PAGE;
		assert(++guard < 1000);
	}
	assert(context.stage2.entry_count >= JP_KERNEL_MEMORY_MAX_ENTRIES - 2);
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
	{
		/* A change that cuts only one entry needs one place; a change that cuts none needs none. */
		JPKernelMemoryOwner tail_owner;
		size_t count = context.stage2.entry_count;
		const JPBootStage2MemoryEntry *last_available = NULL;
		size_t i;
		uint64_t last_end;
		uint64_t found;

		for (i = 0; i < count; i++) {
			if (context.stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_AVAILABLE &&
			    context.stage2.entries[i].base_address > address) {
				last_available = &context.stage2.entries[i];
			}
		}
		assert(last_available != NULL);
		last_end = last_available->base_address + last_available->length;
		/* the tail of an available entry: one cut (at its start), none at its end */
		if (count + 1 <= JP_KERNEL_MEMORY_MAX_ENTRIES) {
			assert(jp_kernel_memory_reserve(&context, last_end - PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME) == JP_BOOT_SUCCESS);
			assert(context.stage2.entry_count == count + 1);
			assert(jp_kernel_memory_query(&context, last_end - 1, 1, &tail_owner) == JP_BOOT_SUCCESS);
			assert(tail_owner == JP_KERNEL_MEMORY_STAGE2_RUNTIME);
		} else {
			assert(jp_kernel_memory_reserve(&context, last_end - PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME) == JP_BOOT_FAILED);
		}
		/* the full table is valid, and the whole single page between two reservations can still be taken */
		assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
		assert(jp_kernel_memory_allocate(&context, PAGE, PAGE, JP_KERNEL_MEMORY_STAGE2_RUNTIME, &found) == JP_BOOT_SUCCESS);
		assert(found == MAIN_BASE + PAGE);
	}
	/* release gives room back: merging brings the count down, and the next reservation fits again */
	{
		size_t before_release = context.stage2.entry_count;

		assert(jp_kernel_memory_release(&context, MAIN_BASE + 4 * PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
		assert(jp_kernel_memory_release(&context, MAIN_BASE + 6 * PAGE, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
		assert(context.stage2.entry_count < before_release);
		assert(jp_kernel_memory_reserve(&context, address, PAGE, JP_KERNEL_MEMORY_EARLY_HEAP) == JP_BOOT_SUCCESS);
		assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_SUCCESS);
	}
}

static void test_capacity(void)
{
	run_capacity(false);
	run_capacity(true);
}

/* ---- validate_ownership ---- */

static void test_validate_detects_damage(void)
{
	size_t n;

	ready();
	assert(jp_kernel_memory_validate_ownership(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	n = context.stage2.entry_count;

	/* a zero length */
	ready();
	context.stage2.entries[2].length = 0;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* out of order */
	ready();
	{
		JPBootStage2MemoryEntry t = context.stage2.entries[1];

		context.stage2.entries[1] = context.stage2.entries[2];
		context.stage2.entries[2] = t;
	}
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* an overlap */
	ready();
	context.stage2.entries[1].length += PAGE;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* a hole where the map has memory (sum mismatch) */
	ready();
	context.stage2.entries[1].length -= PAGE;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* a bad owner value */
	ready();
	context.stage2.entries[1].owner = 77;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	ready();
	context.stage2.entries[1].owner = 10;   /* the first value after the last owner */
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* touching neighbours with the same owner: split the first (available) entry in two and keep the owner */
	ready();
	{
		size_t i;

		for (i = n; i > 1; i--) {
			context.stage2.entries[i] = context.stage2.entries[i - 1];
		}
		context.stage2.entries[1].base_address = context.stage2.entries[0].base_address + PAGE;
		context.stage2.entries[1].length = context.stage2.entries[0].length - PAGE;
		context.stage2.entries[1].owner = context.stage2.entries[0].owner;
		context.stage2.entries[0].length = PAGE;
		context.stage2.entry_count = n + 1;
	}
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* an available entry over a region that is not usable */
	ready();
	{
		size_t i;

		for (i = 0; i < n; i++) {
			if (context.stage2.entries[i].base_address == ACPI_BASE) {
				context.stage2.entries[i].owner = JP_KERNEL_MEMORY_AVAILABLE;
			}
		}
	}
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* the heap over an unusable region */
	ready();
	{
		size_t i;

		for (i = 0; i < n; i++) {
			if (context.stage2.entries[i].base_address == BAD_BASE) {
				context.stage2.entries[i].owner = JP_KERNEL_MEMORY_EARLY_HEAP;
			}
		}
	}
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* more entries than the table has */
	ready();
	context.stage2.entry_count = JP_KERNEL_MEMORY_MAX_ENTRIES + 1;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	/* the map changed after initialization: a region the table does not cover */
	ready();
	context.memory_map.regions[0].length += PAGE;
	assert(jp_kernel_memory_validate_ownership(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

int main(void)
{
	context_object = jp_test_map_context(sizeof *context_object);
	test_initialize_checks();
	test_initialize_map_errors();
	test_initialize_limits();
	test_initial_owners();
	test_table_shape();
	test_initialize_clears_the_area();
	test_segment_rounding_and_sharing();
	test_cut_errors();
	test_framebuffer_cases();
	test_region_order_does_not_matter();
	test_initialize_twice();
	test_not_initialized();
	test_reserve();
	test_reserve_whole_region_and_edges();
	test_reserve_refusals();
	test_allocate();
	test_allocate_alignment_and_gaps();
	test_allocate_refusals();
	test_query();
	test_release();
	test_release_refusals();
	test_capacity();
	test_validate_detects_damage();
	return 0;
}
