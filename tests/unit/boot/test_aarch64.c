/*
 * Unit test (level U) of src/boot/architecture/aarch64.c.
 * "Physical memory" is a host buffer; the four processor instructions that
 * cannot be written in C are fakes, so this test runs on any host. A small
 * walker reads the translation tables back, so the test checks the tables
 * themselves. Rules: "Decided rules (AArch64: exception level, translation,
 * and the jump)" in milestone0.md.
 */

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "aarch64_cpu.h"
#include "fake_environment.h"
#include "justos/boot/architecture/aarch64.h"

extern void *(*jp_aarch64_test_phys_to_pointer)(uint64_t physical);

enum {
	SIM_SIZE = 16 * 1024 * 1024,
	PAGE = 4096,
	POOL_BYTES = JP_AARCH64_TABLE_POOL_PAGES * PAGE
};

#define SIM_BASE 0x40000000ULL
#define KERNEL_VIRTUAL 0xFFFF000000000000ULL
#define DESC_VALID 0x1ULL
#define DESC_TABLE_OR_PAGE 0x2ULL
#define DESC_ATTR_SHIFT 2
#define DESC_AP_READ_ONLY (1ULL << 7)
#define DESC_AF (1ULL << 10)
#define DESC_PXN (1ULL << 53)
#define DESC_UXN (1ULL << 54)
#define DESC_ADDRESS_MASK 0x0000FFFFFFFFF000ULL

static uint8_t *simulated_memory;

static void *host_pointer(uint64_t physical)
{
	if (physical < SIM_BASE || physical >= SIM_BASE + SIM_SIZE) {
		return NULL;
	}
	return simulated_memory + (physical - SIM_BASE);
}

static void *any_address_pointer(uint64_t physical)
{
	(void)physical;
	return simulated_memory;
}

/* ---- fakes of the four assembly functions ---- */

static JPA64CpuState fake_cpu_state;
static unsigned int disable_calls;

void jp_aarch64_disable_interrupts(void)
{
	disable_calls++;
}

void jp_aarch64_read_cpu_state(JPA64CpuState *state)
{
	*state = fake_cpu_state;
}

static jmp_buf jump_buffer;
static struct {
	unsigned int jumps;
	unsigned int parks;
	uint64_t ttbr1, ttbr0, stack, entry;
	const JPBootContext *context;
} jump_record;

_Noreturn void jp_aarch64_jump_to_kernel(uint64_t ttbr1, uint64_t ttbr0, uint64_t stack,
	uint64_t entry, const JPBootContext *context)
{
	jump_record.jumps++;
	jump_record.ttbr1 = ttbr1;
	jump_record.ttbr0 = ttbr0;
	jump_record.stack = stack;
	jump_record.entry = entry;
	jump_record.context = context;
	longjmp(jump_buffer, 1);
}

_Noreturn void jp_aarch64_park(void)
{
	jump_record.parks++;
	longjmp(jump_buffer, 2);
}

/* ---- a translation-table walker ---- */

typedef struct Translation {
	bool present;
	bool block;
	uint64_t physical;
	unsigned int attr_index;
	bool read_only;
	bool pxn;
	bool uxn;
	bool access_flag;
} Translation;

static uint64_t read_entry(uint64_t table_physical, unsigned int index)
{
	const uint64_t *table = (const uint64_t *)host_pointer(table_physical);

	assert(table != NULL);
	return table[index];
}

static Translation decode(uint64_t entry, uint64_t address, uint64_t offset_mask)
{
	Translation t;

	memset(&t, 0, sizeof t);
	t.present = true;
	t.physical = (entry & DESC_ADDRESS_MASK & ~offset_mask) | (address & offset_mask);
	t.attr_index = (unsigned int)((entry >> DESC_ATTR_SHIFT) & 0x7);
	t.read_only = (entry & DESC_AP_READ_ONLY) != 0;
	t.pxn = (entry & DESC_PXN) != 0;
	t.uxn = (entry & DESC_UXN) != 0;
	t.access_flag = (entry & DESC_AF) != 0;
	return t;
}

static Translation translate(uint64_t ttbr1, uint64_t ttbr0, uint64_t virtual_address)
{
	Translation none;
	uint64_t table = (virtual_address >> 48) == 0xFFFF ? ttbr1 : ttbr0;
	int level;

	memset(&none, 0, sizeof none);
	for (level = 0; level <= 3; level++) {
		unsigned int index = (unsigned int)((virtual_address >> (12 + 9 * (3 - level))) & 0x1FF);
		uint64_t entry = read_entry(table, index);

		if ((entry & DESC_VALID) == 0) {
			return none;
		}
		if (level == 2 && (entry & DESC_TABLE_OR_PAGE) == 0) {
			Translation t = decode(entry, virtual_address, 0x1FFFFFULL);

			t.block = true;
			return t;
		}
		if (level == 3) {
			assert((entry & DESC_TABLE_OR_PAGE) != 0);
			return decode(entry, virtual_address, 0xFFFULL);
		}
		assert((entry & DESC_TABLE_OR_PAGE) != 0);            /* a table descriptor */
		table = entry & DESC_ADDRESS_MASK;
	}
	return none;
}

/* ---- a context with a loaded kernel ---- */

static JPFakeEnvironment env;

static void make_context(JPBootContext *c)
{
	memset(c, 0, sizeof *c);
	memset(&env, 0, sizeof env);
	memset(simulated_memory, 0xAA, SIM_SIZE);          /* "dirty" memory: the pool must be zeroed */
	c->environment.platform_context = &env;
	c->handoff.architecture = JP_ARCH_AARCH64;

	c->memory_map.state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.region_count = 3;
	c->memory_map.regions[0].base_address = 0x08000000ULL;
	c->memory_map.regions[0].length = 0x10000000ULL;
	c->memory_map.regions[0].type = JP_MEMORY_REGION_MMIO;
	c->memory_map.regions[1].base_address = SIM_BASE;
	c->memory_map.regions[1].length = SIM_SIZE;
	c->memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	c->memory_map.regions[2].base_address = 0x3F000000ULL;
	c->memory_map.regions[2].length = 0x1000000;
	c->memory_map.regions[2].type = JP_MEMORY_REGION_RESERVED;

	c->kernel.entry_point = KERNEL_VIRTUAL;
	c->kernel.segment_count = 2;
	c->kernel.segments[0].virtual_address = KERNEL_VIRTUAL;
	c->kernel.segments[0].physical_address = SIM_BASE + 0x100000;
	c->kernel.segments[0].memory_size = 0x2800;            /* three pages */
	c->kernel.segments[0].flags = 5;                       /* R X */
	c->kernel.segments[0].state = JP_KERNEL_SEGMENT_LOADED;
	c->kernel.segments[1].virtual_address = KERNEL_VIRTUAL + 0x10000;
	c->kernel.segments[1].physical_address = SIM_BASE + 0x200000;
	c->kernel.segments[1].memory_size = 0x1000;
	c->kernel.segments[1].flags = 6;                       /* R W */
	c->kernel.segments[1].state = JP_KERNEL_SEGMENT_LOADED;

	fake_cpu_state.current_el = 1;
	fake_cpu_state.daif = JP_AARCH64_DAIF_ALL_MASKED;
	fake_cpu_state.sctlr = JP_AARCH64_SCTLR_MMU;
	fake_cpu_state.midr = 0x410FD083ULL;
	disable_calls = 0;
}

static uint64_t expected_pool_base(void)
{
	return SIM_BASE + SIM_SIZE - POOL_BYTES;
}

static uint64_t ttbr0_of(const JPBootContext *c)
{
	return c->handoff.page_table_memory_base + PAGE;
}

static Translation walk(const JPBootContext *c, uint64_t virtual_address)
{
	return translate(c->handoff.page_table_root, ttbr0_of(c), virtual_address);
}

/* ================= tests ================= */

static void test_validate_elf_machine(void)
{
	JPKernelHeader header;

	memset(&header, 0, sizeof header);
	header.machine = JP_ELF_MACHINE_AARCH64;
	assert(jp_aarch64_validate_elf_machine(&header) == JP_BOOT_SUCCESS);
	header.machine = JP_ELF_MACHINE_X86_64;
	assert(jp_aarch64_validate_elf_machine(&header) == JP_BOOT_ERROR_CORRUPTED_DATA);
	header.machine = 0;
	assert(jp_aarch64_validate_elf_machine(&header) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_aarch64_validate_elf_machine(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_discover_cpu(void)
{
	JPBootContext c;
	JPCpuInfo cpu;

	make_context(&c);
	memset(&cpu, 0, sizeof cpu);
	assert(jp_aarch64_discover_cpu(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_aarch64_discover_cpu(&cpu) == JP_BOOT_SUCCESS);
	assert(cpu.state == JP_DETECTION_STATE_DETECTED);
	assert(cpu.logical_cores == 1);                  /* only the boot processor is counted */

	/* An identification register that reads as zero was not really read. */
	memset(&cpu, 0, sizeof cpu);
	fake_cpu_state.midr = 0;
	assert(jp_aarch64_discover_cpu(&cpu) == JP_BOOT_FAILED);
	assert(cpu.state != JP_DETECTION_STATE_DETECTED);
}

static void test_phys_access(void)
{
	JPBootContext c;

	make_context(&c);
	{
		void *(*saved)(uint64_t) = jp_aarch64_test_phys_to_pointer;

		jp_aarch64_test_phys_to_pointer = NULL;
		assert(jp_aarch64_phys_access(&c, SIM_BASE + 0x1000, 0x100) == (void *)(uintptr_t)(SIM_BASE + 0x1000));
		jp_aarch64_test_phys_to_pointer = saved;
	}
	assert(jp_aarch64_phys_access(&c, SIM_BASE, 0x1000) != NULL);
	assert(jp_aarch64_phys_access(&c, SIM_BASE + SIM_SIZE - 0x1000, 0x1000) != NULL);
	assert(jp_aarch64_phys_access(&c, SIM_BASE + SIM_SIZE - 0x1000, 0x1001) == NULL);
	assert(jp_aarch64_phys_access(&c, SIM_BASE, 0) == NULL);
	assert(jp_aarch64_phys_access(&c, UINT64_MAX - 0x10, 0x100) == NULL);
	assert(jp_aarch64_phys_access(&c, 0x10000000ULL, 0x1000) == NULL);
	assert(jp_aarch64_phys_access(NULL, SIM_BASE, 0x1000) == NULL);

	/* Address zero is refused by the function itself, not only by a missing pointer. */
	{
		void *(*saved)(uint64_t) = jp_aarch64_test_phys_to_pointer;

		jp_aarch64_test_phys_to_pointer = any_address_pointer;
		make_context(&c);
		c.memory_map.regions[2].base_address = 0;
		c.memory_map.regions[2].length = 0x10000;
		assert(jp_aarch64_phys_access(&c, 0, 0x1000) == NULL);
		assert(jp_aarch64_phys_access(&c, 0x1000, 0x1000) != NULL);
		jp_aarch64_test_phys_to_pointer = saved;
	}
}

static void test_first_mapping_creates_the_pool(void)
{
	JPBootContext c;
	uint64_t base = expected_pool_base();
	size_t i;

	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);

	assert(c.handoff.page_table_memory_base == base);
	assert(c.handoff.page_table_memory_size == POOL_BYTES);
	assert(c.handoff.page_table_root == base);                      /* the TTBR1 tree: page 0 */
	assert(ttbr0_of(&c) == base + PAGE);                            /* the TTBR0 tree: page 1 */
	assert(env.reserve_calls == 1);
	assert(env.reserved_base[0] == base && env.reserved_length[0] == POOL_BYTES);

	for (i = 0; i < POOL_BYTES; i++) {
		if (i >= c.handoff.page_table_memory_used * PAGE) {
			assert(simulated_memory[base - SIM_BASE + i] == 0);    /* zeroed before use */
		}
	}
	/* Two roots and the three tables below the first root for one segment: five pages. */
	assert(c.handoff.page_table_memory_used == 5);
}

static void test_segment_mapping_and_permissions(void)
{
	JPBootContext c;
	Translation t;
	uint64_t page;

	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_aarch64_map_segment(&c, 1) == JP_BOOT_SUCCESS);

	/* Segment 0: read + execute: read only, privileged execute allowed, user execute never. */
	for (page = 0; page < 3; page++) {
		t = walk(&c, KERNEL_VIRTUAL + page * PAGE + 0x10);
		assert(t.present && !t.block);
		assert(t.physical == SIM_BASE + 0x100000 + page * PAGE + 0x10);
		assert(t.read_only && !t.pxn && t.uxn);
		assert(t.attr_index == 1 && t.access_flag);
	}
	assert(!walk(&c, KERNEL_VIRTUAL + 3 * PAGE).present);
	assert(!walk(&c, KERNEL_VIRTUAL - PAGE).present);

	/* Segment 1: read + write: writable, never executable. */
	t = walk(&c, KERNEL_VIRTUAL + 0x10000);
	assert(t.present && t.physical == SIM_BASE + 0x200000);
	assert(!t.read_only && t.pxn && t.uxn);

	assert(c.handoff.page_table_memory_used == 5);                   /* shared tables */
	assert(env.reserve_calls == 1);
}

static void test_segment_flag_combinations(void)
{
	static const struct { uint32_t flags; bool read_only; bool pxn; } cases[] = {
		{ 4, true, true },      /* R */
		{ 5, true, false },     /* R X */
		{ 6, false, true },     /* R W */
		{ 2, false, true },     /* W only */
		{ 1, true, false }      /* X only */
	};
	size_t i;

	for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
		JPBootContext c;
		Translation t;

		make_context(&c);
		c.kernel.segments[0].flags = cases[i].flags;
		assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
		t = walk(&c, KERNEL_VIRTUAL);
		assert(t.present && t.uxn);
		assert(t.read_only == cases[i].read_only);
		assert(t.pxn == cases[i].pxn);
	}
}

static void test_the_half_of_an_address_chooses_the_tree(void)
{
	JPBootContext c;
	Translation t;

	/* A low-half address goes to the TTBR0 tree. */
	make_context(&c);
	c.kernel.segments[0].virtual_address = 0x0000000010000000ULL;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	t = walk(&c, 0x0000000010000000ULL);
	assert(t.present && t.physical == SIM_BASE + 0x100000);
	assert(!translate(c.handoff.page_table_root, ttbr0_of(&c), 0xFFFF000010000000ULL).present);

	/* An address in neither half is refused. */
	make_context(&c);
	c.kernel.segments[0].virtual_address = 0x0001000000000000ULL;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	make_context(&c);
	c.kernel.segments[0].virtual_address = 0xFFFE000000000000ULL;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
}

static void test_map_segment_preconditions(void)
{
	JPBootContext c;

	make_context(&c);
	assert(jp_aarch64_map_segment(NULL, 0) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_aarch64_map_segment(&c, 2) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_aarch64_map_segment(&c, JP_MAX_KERNEL_SEGMENTS) == JP_BOOT_ERROR_INVALID_PARAM);
	c.kernel.segments[0].state = JP_KERNEL_SEGMENT_ALLOCATED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_NOT_INITIALIZED);
	assert(env.reserve_calls == 0);
	assert(c.handoff.page_table_root == 0);
}

static void test_pool_placement(void)
{
	JPBootContext c;

	make_context(&c);
	env.reserve_result = JP_BOOT_FAILED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(c.handoff.page_table_root == 0 && c.handoff.page_table_memory_base == 0);

	make_context(&c);
	c.memory_map.regions[1].length = 0x40000;                    /* too small for 512 KiB */
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 0);

	/* A higher region that is not usable, a tiny usable one, and two big ones. */
	make_context(&c);
	c.memory_map.region_count = 5;
	c.memory_map.regions[3].base_address = SIM_BASE + SIM_SIZE;
	c.memory_map.regions[3].length = 2 * 1024 * 1024;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_RESERVED;
	c.memory_map.regions[4].base_address = 0x1000;
	c.memory_map.regions[4].length = 0x1000;
	c.memory_map.regions[4].type = JP_MEMORY_REGION_USABLE;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3] = c.memory_map.regions[1];
	c.memory_map.regions[1].base_address = 0x20000000ULL;          /* the lower candidate comes first */
	c.memory_map.regions[1].length = 0x1000000;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	/* A region below the protected low memory is never used. */
	make_context(&c);
	c.memory_map.region_count = 1;
	c.memory_map.regions[0].base_address = 0x1000;
	c.memory_map.regions[0].length = 0x90000;
	c.memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 0);
}

static void test_refused_pool_reservation_tries_lower_places(void)
{
	JPBootContext c;

	make_context(&c);
	env.reserve_refusals = 2;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(env.reserve_calls == 3);
	assert(env.reserved_base[1] == expected_pool_base() - POOL_BYTES);
	assert(env.reserved_base[2] == expected_pool_base() - 2 * POOL_BYTES);
	assert(c.handoff.page_table_memory_base == expected_pool_base() - 2 * POOL_BYTES);
	assert(walk(&c, KERNEL_VIRTUAL).present);

	make_context(&c);
	env.reserve_refusals = JP_AARCH64_POOL_ATTEMPTS;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == JP_AARCH64_POOL_ATTEMPTS);
	assert(c.handoff.page_table_memory_base == 0);

	make_context(&c);
	env.reserve_refusals = JP_AARCH64_POOL_ATTEMPTS - 1;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(env.reserve_calls == JP_AARCH64_POOL_ATTEMPTS);

	make_context(&c);
	c.memory_map.regions[1].length = POOL_BYTES;
	env.reserve_refusals = 1;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 1);
}

static void test_pool_exhaustion_and_conflicts(void)
{
	JPBootContext c;

	make_context(&c);
	c.kernel.segments[0].memory_size = 1ULL << 30;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(c.handoff.page_table_memory_used == JP_AARCH64_TABLE_POOL_PAGES);

	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	c.kernel.segments[1].virtual_address = KERNEL_VIRTUAL + PAGE;
	assert(jp_aarch64_map_segment(&c, 1) == JP_BOOT_FAILED);
}

static void test_setup_paging_identity(void)
{
	JPBootContext c;
	Translation t;

	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_aarch64_map_segment(&c, 1) == JP_BOOT_SUCCESS);
	assert(!c.handoff.paging_enabled);
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(c.handoff.paging_enabled);

	/* Normal memory: AttrIdx 1, read/write, privileged execute allowed, user execute never. */
	t = walk(&c, SIM_BASE + 0x123456);
	assert(t.present && t.block && t.physical == SIM_BASE + 0x123456);
	assert(t.attr_index == 1 && !t.read_only && !t.pxn && t.uxn && t.access_flag);
	t = walk(&c, SIM_BASE + SIM_SIZE - 0x1000);
	assert(t.present && t.physical == SIM_BASE + SIM_SIZE - 0x1000);
	assert(!walk(&c, SIM_BASE + SIM_SIZE + 0x400000).present);       /* beyond the memory */
	assert(!walk(&c, 0x20000000ULL).present);                        /* a hole between regions */

	/* Memory-mapped I/O: device memory, never executable. */
	t = walk(&c, 0x09000000ULL);
	assert(t.present && t.block && t.physical == 0x09000000ULL);
	assert(t.attr_index == 0 && t.pxn && t.uxn);

	/* The kernel map is still there and untouched. */
	t = walk(&c, KERNEL_VIRTUAL);
	assert(t.present && !t.block && t.physical == SIM_BASE + 0x100000);

	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_FAILED);           /* once only */
	assert(jp_aarch64_setup_paging(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_setup_paging_without_segments_mapped_before(void)
{
	JPBootContext c;

	make_context(&c);
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());
	assert(c.handoff.paging_enabled);
	assert(walk(&c, SIM_BASE).present);
}

static void test_identity_map_follows_the_regions(void)
{
	JPBootContext c;

	make_context(&c);
	c.memory_map.region_count = 8;
	/* An address range kept by the firmware: not mapped (reserved and 1 GiB or larger). */
	c.memory_map.regions[3].base_address = 0xFD00000000ULL;
	c.memory_map.regions[3].length = 12ULL << 30;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_RESERVED;
	/* A huge MMIO range is not mapped either. */
	c.memory_map.regions[4].base_address = 0x8000000000ULL;
	c.memory_map.regions[4].length = JP_AARCH64_IDENTITY_RESERVED_MAX;
	c.memory_map.regions[4].type = JP_MEMORY_REGION_MMIO;
	/* Unusable memory is not mapped. */
	c.memory_map.regions[5].base_address = 0x70000000ULL;
	c.memory_map.regions[5].length = 0x400000;
	c.memory_map.regions[5].type = JP_MEMORY_REGION_UNUSABLE;
	/* A reserved region smaller than the limit is memory in use: mapped as normal memory. */
	c.memory_map.regions[6].base_address = 0x80000000ULL;
	c.memory_map.regions[6].length = JP_AARCH64_IDENTITY_RESERVED_MAX - 0x200000;
	c.memory_map.regions[6].type = JP_MEMORY_REGION_RESERVED;
	/* ACPI memory is mapped. */
	c.memory_map.regions[7].base_address = 0x60000000ULL;
	c.memory_map.regions[7].length = 0x200000;
	c.memory_map.regions[7].type = JP_MEMORY_REGION_ACPI;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);

	assert(!walk(&c, 0xFD00000000ULL).present);
	assert(!walk(&c, 0x8000000000ULL).present);
	assert(!walk(&c, 0x70000000ULL).present);
	assert(walk(&c, 0x80000000ULL + 0x1000).present && walk(&c, 0x80000000ULL + 0x1000).attr_index == 1);
	assert(walk(&c, 0x60000000ULL + 0x1000).present);
	assert(walk(&c, 0x08000000ULL).attr_index == 0);                 /* the MMIO region of the context */
}

static void test_regions_that_share_a_block_are_accepted(void)
{
	JPBootContext c;

	make_context(&c);
	c.memory_map.region_count = 5;
	c.memory_map.regions[3].base_address = 0x70000000ULL;
	c.memory_map.regions[3].length = 0x1000;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_ACPI;
	c.memory_map.regions[4].base_address = 0x70001000ULL;
	c.memory_map.regions[4].length = 0x1000;
	c.memory_map.regions[4].type = JP_MEMORY_REGION_FIRMWARE;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(walk(&c, 0x70001000ULL).present);
}

static void test_setup_paging_maps_the_framebuffer(void)
{
	JPBootContext c;
	Translation t;

	make_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c.framebuffer.base_address = 0x80000000ULL;
	c.framebuffer.width = 1024;
	c.framebuffer.height = 768;
	c.framebuffer.pitch = 4096;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	t = walk(&c, 0x80000000ULL + 0x1234);
	assert(t.present && t.block && t.physical == 0x80000000ULL + 0x1234);
	assert(t.attr_index == 0 && t.pxn && t.uxn);
	assert(walk(&c, 0x80000000ULL + 4096ULL * 768 - 1).present);
	assert(!walk(&c, 0x80000000ULL + 0x400000).present);

	make_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	c.framebuffer.base_address = 0x80000000ULL;
	c.framebuffer.pitch = 4096;
	c.framebuffer.height = 768;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(!walk(&c, 0x80000000ULL).present);

	/* A framebuffer inside an identity block changes the attributes of that block only. */
	make_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c.framebuffer.base_address = SIM_BASE + 0x800000ULL;
	c.framebuffer.width = 800;
	c.framebuffer.height = 600;
	c.framebuffer.pitch = 4096;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	t = walk(&c, SIM_BASE + 0x800000ULL + 0x100);
	assert(t.present && t.attr_index == 0 && t.pxn && t.uxn);
	t = walk(&c, SIM_BASE + 0x400000ULL);
	assert(t.present && t.attr_index == 1 && !t.pxn);
}

static void test_setup_paging_conflicts_and_limits(void)
{
	JPBootContext c;

	/* A low-half kernel segment inside an identity block conflicts with it. */
	make_context(&c);
	c.kernel.segments[0].virtual_address = SIM_BASE + 0x100000;
	c.kernel.entry_point = SIM_BASE + 0x100000;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);

	/* The reverse: after the identity map, a segment in an identity block conflicts. */
	make_context(&c);
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	c.kernel.segments[0].virtual_address = SIM_BASE + 0x400000;
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_FAILED);

	/* Memory so large that the identity map does not fit in the pool. */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = 1ULL << 40;
	c.memory_map.regions[3].length = 130ULL << 30;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_USABLE;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);

	/* No region can be mapped at all. */
	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	c.memory_map.regions[0].type = JP_MEMORY_REGION_UNUSABLE;
	c.memory_map.regions[1].type = JP_MEMORY_REGION_UNUSABLE;
	c.memory_map.regions[2].type = JP_MEMORY_REGION_UNUSABLE;
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);
}

static void test_prepare_entry(void)
{
	JPBootContext c;

	make_context(&c);
	assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS);
	c.handoff.stack_pointer = SIM_BASE + 0x300000;
	c.handoff.architecture = JP_ARCH_UNKNOWN;                /* prepare_entry records it */

	assert(jp_aarch64_prepare_entry(&c) == JP_BOOT_SUCCESS);
	assert(disable_calls == 1);
	assert(c.handoff.kernel_entry == KERNEL_VIRTUAL);
	assert(c.handoff.architecture == JP_ARCH_AARCH64);
	assert(c.handoff.interrupts_disabled);
	assert(jp_aarch64_prepare_entry(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_prepare_entry_refusals(void)
{
	JPBootContext c;

#define PREPARE_FAILS(statement) \
	do { \
		make_context(&c); \
		assert(jp_aarch64_map_segment(&c, 0) == JP_BOOT_SUCCESS); \
		assert(jp_aarch64_setup_paging(&c) == JP_BOOT_SUCCESS); \
		c.handoff.stack_pointer = SIM_BASE + 0x300000; \
		statement; \
		assert(jp_aarch64_prepare_entry(&c) == JP_BOOT_FAILED); \
		assert(disable_calls == 0); \
		assert(!c.handoff.interrupts_disabled && c.handoff.kernel_entry == 0); \
	} while (0)

	PREPARE_FAILS(c.handoff.page_table_root = 0);
	PREPARE_FAILS(c.handoff.paging_enabled = false);
	PREPARE_FAILS(c.handoff.stack_pointer = 0);
	PREPARE_FAILS(c.handoff.stack_pointer += 8);
	PREPARE_FAILS(c.kernel.entry_point = 0);
	PREPARE_FAILS(c.kernel.entry_point = 0x0001000000000000ULL);    /* upper 16 bits neither all 0 nor all 1 */
	PREPARE_FAILS(c.kernel.entry_point = 0xFFFE000000000000ULL);
#undef PREPARE_FAILS
}

static void ready_context(JPBootContext *c)
{
	make_context(c);
	assert(jp_aarch64_map_segment(c, 0) == JP_BOOT_SUCCESS);
	c->kernel.segments[0].state = JP_KERNEL_SEGMENT_PERMISSIONED;
	assert(jp_aarch64_setup_paging(c) == JP_BOOT_SUCCESS);
	c->handoff.stack_pointer = SIM_BASE + 0x300000;
	assert(jp_aarch64_prepare_entry(c) == JP_BOOT_SUCCESS);
}

static void test_validate_entry_state(void)
{
	JPBootContext c;
	JPBootContext before;

	ready_context(&c);
	before = c;
	assert(jp_aarch64_validate_entry_state(&c) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &c, sizeof c) == 0);
	assert(jp_aarch64_validate_entry_state(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	/* EL2 is accepted as well (the jump drops to EL1). */
	ready_context(&c);
	fake_cpu_state.current_el = 2;
	assert(jp_aarch64_validate_entry_state(&c) == JP_BOOT_SUCCESS);

#define VALIDATE_FAILS(statement) \
	do { \
		ready_context(&c); \
		statement; \
		assert(jp_aarch64_validate_entry_state(&c) == JP_BOOT_FAILED); \
	} while (0)

	VALIDATE_FAILS(fake_cpu_state.current_el = 3);
	VALIDATE_FAILS(fake_cpu_state.current_el = 0);
	VALIDATE_FAILS(fake_cpu_state.daif = 0xE);               /* FIQ unmasked */
	VALIDATE_FAILS(fake_cpu_state.daif = 0x7);               /* debug unmasked */
	VALIDATE_FAILS(fake_cpu_state.daif = 0);
	VALIDATE_FAILS(fake_cpu_state.sctlr = 0);                /* MMU off */
	VALIDATE_FAILS(c.handoff.architecture = JP_ARCH_X86_64);
	VALIDATE_FAILS(c.handoff.paging_enabled = false);
	VALIDATE_FAILS(c.handoff.interrupts_disabled = false);
	VALIDATE_FAILS(c.handoff.page_table_root = 0);
	VALIDATE_FAILS(c.handoff.page_table_root += 8);
	VALIDATE_FAILS(c.handoff.stack_pointer = 0);
	VALIDATE_FAILS(c.handoff.stack_pointer += 4);
	VALIDATE_FAILS(c.handoff.kernel_entry += 4);
	VALIDATE_FAILS(c.kernel.segments[0].state = JP_KERNEL_SEGMENT_MAPPED);
	VALIDATE_FAILS(c.kernel.segments[0].flags = 4);
	VALIDATE_FAILS(c.handoff.kernel_entry = KERNEL_VIRTUAL + 0x10000; c.kernel.entry_point = KERNEL_VIRTUAL + 0x10000);
	VALIDATE_FAILS(c.handoff.kernel_entry = KERNEL_VIRTUAL + 0x2800; c.kernel.entry_point = KERNEL_VIRTUAL + 0x2800);
	VALIDATE_FAILS(c.handoff.kernel_entry = 0x0001000000000000ULL; c.kernel.entry_point = 0x0001000000000000ULL);
#undef VALIDATE_FAILS
}

static void test_enter_kernel(void)
{
	JPBootContext c;
	int outcome;

	ready_context(&c);
	c.handoff.state = JP_HANDOFF_STATE_READY;
	c.kernel.entry_point += 0x10;                    /* the jump uses handoff.kernel_entry */
	memset(&jump_record, 0, sizeof jump_record);
	outcome = setjmp(jump_buffer);
	if (outcome == 0) {
		jp_aarch64_enter_kernel(&c);
	}
	assert(outcome == 1 && jump_record.jumps == 1 && jump_record.parks == 0);
	assert(jump_record.ttbr1 == c.handoff.page_table_root);
	assert(jump_record.ttbr0 == c.handoff.page_table_memory_base + PAGE);
	assert(jump_record.stack == c.handoff.stack_pointer);
	assert(jump_record.entry == c.handoff.kernel_entry);
	assert(jump_record.context == &c);

#define ENTER_PARKS(statement) \
	do { \
		ready_context(&c); \
		c.handoff.state = JP_HANDOFF_STATE_READY; \
		statement; \
		memset(&jump_record, 0, sizeof jump_record); \
		outcome = setjmp(jump_buffer); \
		if (outcome == 0) { \
			jp_aarch64_enter_kernel(&c); \
		} \
		assert(outcome == 2 && jump_record.parks == 1 && jump_record.jumps == 0); \
	} while (0)

	ENTER_PARKS(c.handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED);
	ENTER_PARKS(c.handoff.state = JP_HANDOFF_STATE_FAILED);
	ENTER_PARKS(c.handoff.paging_enabled = false);
	ENTER_PARKS(c.handoff.page_table_root = 0);
#undef ENTER_PARKS

	memset(&jump_record, 0, sizeof jump_record);
	outcome = setjmp(jump_buffer);
	if (outcome == 0) {
		jp_aarch64_enter_kernel(NULL);
	}
	assert(outcome == 2 && jump_record.parks == 1);
}

int main(void)
{
	simulated_memory = (uint8_t *)malloc(SIM_SIZE);
	assert(simulated_memory != NULL);
	jp_aarch64_test_phys_to_pointer = host_pointer;

	test_validate_elf_machine();
	test_discover_cpu();
	test_phys_access();
	test_first_mapping_creates_the_pool();
	test_segment_mapping_and_permissions();
	test_segment_flag_combinations();
	test_the_half_of_an_address_chooses_the_tree();
	test_map_segment_preconditions();
	test_pool_placement();
	test_refused_pool_reservation_tries_lower_places();
	test_pool_exhaustion_and_conflicts();
	test_setup_paging_identity();
	test_setup_paging_without_segments_mapped_before();
	test_identity_map_follows_the_regions();
	test_regions_that_share_a_block_are_accepted();
	test_setup_paging_maps_the_framebuffer();
	test_setup_paging_conflicts_and_limits();
	test_prepare_entry();
	test_prepare_entry_refusals();
	test_validate_entry_state();
	test_enter_kernel();

	free(simulated_memory);
	return 0;
}
