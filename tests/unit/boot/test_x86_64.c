/*
 * Unit test (level U) of src/boot/architecture/x86_64.c.
 * "Physical memory" is a host buffer; the three processor instructions that
 * cannot be written in C are fakes. A small page-table walker in this file
 * reads the tables back, so the test checks the tables themselves.
 * Rules: "Decided rules (x86_64: paging, descriptor table, and the jump)".
 */

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "fake_environment.h"
#include "justos/boot/architecture/x86_64.h"
#include "x86_64_cpu.h"

/* ---- the hook that turns a physical address into a host pointer ---- */

extern void *(*jp_x86_64_test_phys_to_pointer)(uint64_t physical);

enum {
	SIM_SIZE = 16 * 1024 * 1024,
	PAGE = 4096,
	POOL_BYTES = JP_X86_64_TABLE_POOL_PAGES * PAGE
};

#define SIM_BASE 0x20000000ULL
#define KERNEL_VIRTUAL 0xFFFFFFFF80000000ULL
#define PTE_PRESENT 0x1ULL
#define PTE_WRITABLE 0x2ULL
#define PTE_CACHE_DISABLE 0x10ULL
#define PTE_LARGE 0x80ULL
#define PTE_NX (1ULL << 63)
#define PTE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL

static uint8_t *simulated_memory;

static void *host_pointer(uint64_t physical)
{
	if (physical < SIM_BASE || physical >= SIM_BASE + SIM_SIZE) {
		return NULL;
	}
	return simulated_memory + (physical - SIM_BASE);
}

/* ---- fakes of the three assembly functions ---- */

static JPX86CpuState fake_cpu_state;
static unsigned int disable_calls;

void jp_x86_64_disable_interrupts(void) JP_X86_64_ASM_ABI;
void jp_x86_64_disable_interrupts(void)
{
	disable_calls++;
}

void jp_x86_64_read_cpu_state(JPX86CpuState *state) JP_X86_64_ASM_ABI;
void jp_x86_64_read_cpu_state(JPX86CpuState *state)
{
	*state = fake_cpu_state;
}

/* The jump and the park end the test case with a longjmp, after recording what they were given. */
static jmp_buf jump_buffer;
static struct {
	unsigned int jumps;
	unsigned int parks;
	uint64_t root, gdt, stack, entry;
	const JPBootContext *context;
} jump_record;

_Noreturn void jp_x86_64_jump_to_kernel(uint64_t root, uint64_t gdt, uint64_t stack,
	uint64_t entry, const JPBootContext *context) JP_X86_64_ASM_ABI;
_Noreturn void jp_x86_64_jump_to_kernel(uint64_t root, uint64_t gdt, uint64_t stack,
	uint64_t entry, const JPBootContext *context)
{
	jump_record.jumps++;
	jump_record.root = root;
	jump_record.gdt = gdt;
	jump_record.stack = stack;
	jump_record.entry = entry;
	jump_record.context = context;
	longjmp(jump_buffer, 1);
}

_Noreturn void jp_x86_64_park(void) JP_X86_64_ASM_ABI;
_Noreturn void jp_x86_64_park(void)
{
	jump_record.parks++;
	longjmp(jump_buffer, 2);
}

/* ---- a page-table walker ---- */

typedef struct Translation {
	bool present;
	uint64_t physical;
	bool writable;
	bool no_execute;
	bool cache_disabled;
	bool large;
} Translation;

static uint64_t read_entry(uint64_t table_physical, unsigned int index)
{
	const uint64_t *table = (const uint64_t *)host_pointer(table_physical);

	assert(table != NULL);
	return table[index];
}

static Translation translate(uint64_t root, uint64_t virtual_address)
{
	Translation result;
	uint64_t entry;
	uint64_t table = root;
	bool writable = true;
	bool no_execute = false;
	int level;

	memset(&result, 0, sizeof result);
	for (level = 3; level >= 0; level--) {
		unsigned int index = (unsigned int)((virtual_address >> (12 + 9 * level)) & 0x1FF);

		entry = read_entry(table, index);
		if ((entry & PTE_PRESENT) == 0) {
			return result;
		}
		writable = writable && (entry & PTE_WRITABLE) != 0;
		no_execute = no_execute || (entry & PTE_NX) != 0;
		if (level == 1 && (entry & PTE_LARGE) != 0) {
			result.present = true;
			result.large = true;
			result.physical = (entry & PTE_ADDRESS_MASK & ~0x1FFFFFULL) | (virtual_address & 0x1FFFFFULL);
			result.writable = writable;
			result.no_execute = no_execute;
			result.cache_disabled = (entry & PTE_CACHE_DISABLE) != 0;
			return result;
		}
		if (level == 0) {
			result.present = true;
			result.physical = (entry & PTE_ADDRESS_MASK) | (virtual_address & 0xFFF);
			result.writable = writable;
			result.no_execute = no_execute;
			result.cache_disabled = (entry & PTE_CACHE_DISABLE) != 0;
			return result;
		}
		table = entry & PTE_ADDRESS_MASK;
	}
	return result;
}

/* ---- a context with a loaded kernel ---- */

static JPFakeEnvironment env;

static void make_context(JPBootContext *c)
{
	memset(c, 0, sizeof *c);
	memset(&env, 0, sizeof env);
	memset(simulated_memory, 0xAA, SIM_SIZE);          /* "dirty" memory: the pool must be zeroed */
	c->environment.platform_context = &env;
	c->handoff.architecture = JP_ARCH_X86_64;

	c->memory_map.state = JP_DETECTION_STATE_DETECTED;
	c->memory_map.region_count = 3;
	c->memory_map.regions[0].base_address = 0;
	c->memory_map.regions[0].length = 0xA0000;
	c->memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	c->memory_map.regions[1].base_address = SIM_BASE;
	c->memory_map.regions[1].length = SIM_SIZE;
	c->memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	c->memory_map.regions[2].base_address = 0xFEE00000ULL;
	c->memory_map.regions[2].length = 0x1000;
	c->memory_map.regions[2].type = JP_MEMORY_REGION_MMIO;

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

	fake_cpu_state.rflags = 0x2;                           /* IF clear */
	fake_cpu_state.cr0 = JP_X86_64_CR0_PG;
	fake_cpu_state.efer = JP_X86_64_EFER_LMA;
	disable_calls = 0;
}

/* A hook that finds a host pointer for every address (to show that a check is not the hook's doing). */
static void *any_address_pointer(uint64_t physical)
{
	(void)physical;
	return simulated_memory;
}

static uint64_t expected_pool_base(void)
{
	return SIM_BASE + SIM_SIZE - POOL_BYTES;
}

static void *any_address_pointer(uint64_t physical);

/* ================= tests ================= */

static void test_validate_elf_machine(void)
{
	JPKernelHeader header;

	memset(&header, 0, sizeof header);
	header.machine = JP_ELF_MACHINE_X86_64;
	assert(jp_x86_64_validate_elf_machine(&header) == JP_BOOT_SUCCESS);
	header.machine = JP_ELF_MACHINE_AARCH64;
	assert(jp_x86_64_validate_elf_machine(&header) == JP_BOOT_ERROR_CORRUPTED_DATA);
	header.machine = 0;
	assert(jp_x86_64_validate_elf_machine(&header) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_x86_64_validate_elf_machine(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_discover_cpu(void)
{
	JPCpuInfo cpu;

	memset(&cpu, 0, sizeof cpu);
	assert(jp_x86_64_discover_cpu(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_x86_64_discover_cpu(&cpu) == JP_BOOT_SUCCESS);   /* the host is a real x86_64 processor */
	assert(cpu.state == JP_DETECTION_STATE_DETECTED);
	assert(cpu.logical_cores >= 1);
}

static void test_phys_access(void)
{
	JPBootContext c;

	make_context(&c);
	/* Without the test hook the address itself is the pointer. */
	{
		void *(*saved)(uint64_t) = jp_x86_64_test_phys_to_pointer;

		jp_x86_64_test_phys_to_pointer = NULL;
		assert(jp_x86_64_phys_access(&c, SIM_BASE + 0x1000, 0x100) == (void *)(uintptr_t)(SIM_BASE + 0x1000));
		jp_x86_64_test_phys_to_pointer = saved;
	}
	assert(jp_x86_64_phys_access(&c, SIM_BASE, 0x1000) != NULL);
	assert(jp_x86_64_phys_access(&c, SIM_BASE + SIM_SIZE - 0x1000, 0x1000) != NULL);     /* the last bytes */
	assert(jp_x86_64_phys_access(&c, SIM_BASE + SIM_SIZE - 0x1000, 0x1001) == NULL);     /* one byte too far */
	assert(jp_x86_64_phys_access(&c, SIM_BASE, 0) == NULL);                              /* empty range */
	assert(jp_x86_64_phys_access(&c, 0, 0x1000) == NULL);                                /* address zero */
	assert(jp_x86_64_phys_access(&c, UINT64_MAX - 0x10, 0x100) == NULL);                 /* overflow */
	assert(jp_x86_64_phys_access(&c, 0x10000000ULL, 0x1000) == NULL);                    /* in no region */
	/* A range that spans two regions is not inside one region. */
	c.memory_map.regions[0].base_address = SIM_BASE - 0x1000;
	c.memory_map.regions[0].length = 0x1000;
	assert(jp_x86_64_phys_access(&c, SIM_BASE - 0x800, 0x1000) == NULL);
	assert(jp_x86_64_phys_access(NULL, SIM_BASE, 0x1000) == NULL);

	/* Address zero is refused by the function itself, not only by a missing pointer. */
	{
		void *(*saved)(uint64_t) = jp_x86_64_test_phys_to_pointer;

		jp_x86_64_test_phys_to_pointer = any_address_pointer;
		make_context(&c);
		assert(jp_x86_64_phys_access(&c, 0, 0x1000) == NULL);          /* region 0 contains it */
		assert(jp_x86_64_phys_access(&c, 0x1000, 0x1000) != NULL);
		jp_x86_64_test_phys_to_pointer = saved;
	}
}

static void test_first_mapping_creates_the_pool(void)
{
	JPBootContext c;
	uint64_t base = expected_pool_base();
	size_t i;

	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);

	/* The pool: at the top of the highest usable region, reserved, described in the handoff. */
	assert(c.handoff.page_table_memory_base == base);
	assert(c.handoff.page_table_memory_size == POOL_BYTES);
	assert(c.handoff.page_table_root == base);
	assert(env.reserve_calls == 1);
	assert(env.reserved_base[0] == base && env.reserved_length[0] == POOL_BYTES);

	/* The pool was zeroed before use (the memory was 0xAA) and only used pages hold entries. */
	for (i = 3 * PAGE; i < POOL_BYTES; i++) {
		if (i >= c.handoff.page_table_memory_used * PAGE) {
			assert(simulated_memory[base - SIM_BASE + i] == 0);
		}
	}
	/* PML4, GDT, and the tables of the high half (PDPT, PD, PT): five pages. */
	assert(c.handoff.page_table_memory_used == 5);
}

static void test_segment_mapping_and_permissions(void)
{
	JPBootContext c;
	uint64_t root;
	Translation t;
	uint64_t page;

	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_x86_64_map_segment(&c, 1) == JP_BOOT_SUCCESS);
	root = c.handoff.page_table_root;

	/* Segment 0: three pages, read + execute: not writable, executable. */
	for (page = 0; page < 3; page++) {
		t = translate(root, KERNEL_VIRTUAL + page * PAGE + 0x10);
		assert(t.present && !t.large);
		assert(t.physical == SIM_BASE + 0x100000 + page * PAGE + 0x10);
		assert(!t.writable && !t.no_execute);
	}
	assert(!translate(root, KERNEL_VIRTUAL + 3 * PAGE).present);        /* nothing beyond the segment */
	assert(!translate(root, KERNEL_VIRTUAL - PAGE).present);

	/* Segment 1: read + write, no execute. */
	t = translate(root, KERNEL_VIRTUAL + 0x10000);
	assert(t.present && t.physical == SIM_BASE + 0x200000);
	assert(t.writable && t.no_execute);

	/* The second segment shares the tables of the first: no new table pages. */
	assert(c.handoff.page_table_memory_used == 5);
	assert(env.reserve_calls == 1);                                      /* the pool is reserved once */
}

static void test_segment_flag_combinations(void)
{
	static const struct { uint32_t flags; bool writable; bool no_execute; } cases[] = {
		{ 4, false, true },     /* R */
		{ 5, false, false },    /* R X */
		{ 6, true, true },      /* R W */
		{ 2, true, true },      /* W only */
		{ 1, false, false }     /* X only */
	};
	size_t i;

	for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
		JPBootContext c;
		Translation t;

		make_context(&c);
		c.kernel.segments[0].flags = cases[i].flags;
		assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
		t = translate(c.handoff.page_table_root, KERNEL_VIRTUAL);
		assert(t.present);
		assert(t.writable == cases[i].writable);
		assert(t.no_execute == cases[i].no_execute);
	}
}

static void test_map_segment_preconditions(void)
{
	JPBootContext c;

	make_context(&c);
	assert(jp_x86_64_map_segment(NULL, 0) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_x86_64_map_segment(&c, 2) == JP_BOOT_ERROR_INVALID_PARAM);       /* no such segment */
	assert(jp_x86_64_map_segment(&c, JP_MAX_KERNEL_SEGMENTS) == JP_BOOT_ERROR_INVALID_PARAM);
	c.kernel.segments[0].state = JP_KERNEL_SEGMENT_ALLOCATED;               /* not loaded yet */
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_NOT_INITIALIZED);
	assert(env.reserve_calls == 0);                                           /* nothing was done */
	assert(c.handoff.page_table_root == 0);
}

static void test_pool_placement_failures(void)
{
	JPBootContext c;

	/* The reservation is refused. */
	make_context(&c);
	env.reserve_result = JP_BOOT_FAILED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(c.handoff.page_table_root == 0 && c.handoff.page_table_memory_base == 0);

	/* No usable region can hold 512 KiB. */
	make_context(&c);
	c.memory_map.regions[1].length = 0x40000;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 0);

	/* The highest usable region that is large enough is used; a higher small one is skipped. */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = SIM_BASE + SIM_SIZE + 0x10000;
	c.memory_map.regions[3].length = 0x10000;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_USABLE;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	/* A higher region that is not usable is never chosen. */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = SIM_BASE + SIM_SIZE;
	c.memory_map.regions[3].length = 2 * 1024 * 1024;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_RESERVED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	/* A tiny usable region near the bottom must not confuse the placement (no wrap-around). */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = 0x1000;
	c.memory_map.regions[3].length = 0x1000;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_USABLE;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	/* Of two usable regions that can hold the pool, the higher one is used, in either order of the map. */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = 0x10000000ULL;           /* below SIM_BASE: a lower candidate */
	c.memory_map.regions[3].length = 0x1000000;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_USABLE;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3] = c.memory_map.regions[1];               /* the high region comes last... */
	c.memory_map.regions[1].base_address = 0x10000000ULL;           /* ...and the low one first */
	c.memory_map.regions[1].length = 0x1000000;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());

	/* A region below the protected low memory is never used (even if a pointer exists for it). */
	make_context(&c);
	c.memory_map.region_count = 1;
	c.memory_map.regions[0].base_address = 0x1000;
	c.memory_map.regions[0].length = 0x90000;
	c.memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 0);
}

/* The firmware refuses a reservation: the next lower place is tried. */
static void test_refused_pool_reservation_tries_lower_places(void)
{
	JPBootContext c;

	make_context(&c);
	env.reserve_refusals = 2;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(env.reserve_calls == 3);
	/* Every try is one pool lower than the one before. */
	assert(env.reserved_base[0] == expected_pool_base());
	assert(env.reserved_base[1] == expected_pool_base() - POOL_BYTES);
	assert(env.reserved_base[2] == expected_pool_base() - 2 * POOL_BYTES);
	assert(c.handoff.page_table_memory_base == expected_pool_base() - 2 * POOL_BYTES);
	assert(c.handoff.page_table_root == c.handoff.page_table_memory_base);
	assert(translate(c.handoff.page_table_root, KERNEL_VIRTUAL).present);   /* and it works there */

	/* After the allowed number of refusals it gives up and nothing is recorded. */
	make_context(&c);
	env.reserve_refusals = JP_X86_64_POOL_ATTEMPTS;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == JP_X86_64_POOL_ATTEMPTS);
	assert(c.handoff.page_table_memory_base == 0 && c.handoff.page_table_root == 0);

	/* One attempt less than the limit is refused: the last try works. */
	make_context(&c);
	env.reserve_refusals = JP_X86_64_POOL_ATTEMPTS - 1;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(env.reserve_calls == JP_X86_64_POOL_ATTEMPTS);

	/* The search stops when no lower place exists: a region that holds the pool only once. */
	make_context(&c);
	c.memory_map.regions[1].length = POOL_BYTES;
	env.reserve_refusals = 1;
	env.reserve_refusal_result = JP_BOOT_FAILED;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(env.reserve_calls == 1);                       /* no second candidate existed */
}

static void test_pool_exhaustion_and_conflicts(void)
{
	JPBootContext c;

	/* A segment so large that its page tables do not fit in the pool. */
	make_context(&c);
	c.kernel.segments[0].memory_size = 1ULL << 30;               /* 1 GiB of virtual pages */
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
	assert(c.handoff.page_table_memory_used == JP_X86_64_TABLE_POOL_PAGES);   /* all handed out, never more */

	/* Two segments may not share a virtual page. */
	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	c.kernel.segments[1].virtual_address = KERNEL_VIRTUAL + PAGE;   /* inside segment 0 */
	assert(jp_x86_64_map_segment(&c, 1) == JP_BOOT_FAILED);
}

static void test_segment_in_an_identity_large_page_conflicts(void)
{
	JPBootContext c;

	/* After setup_paging a 2 MiB identity page covers SIM_BASE + 0x400000: a segment there conflicts. */
	make_context(&c);
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	c.kernel.segments[0].virtual_address = SIM_BASE + 0x400000;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
}

static void test_non_canonical_virtual_address(void)
{
	JPBootContext c;

	make_context(&c);
	c.kernel.segments[0].virtual_address = 0x0000800000000000ULL;     /* bit 47 set, upper bits clear */
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_FAILED);
}

static void test_setup_paging_needs_an_ordinary_region(void)
{
	JPBootContext c;

	make_context(&c);
	c.memory_map.region_count = 3;
	c.memory_map.regions[0].type = JP_MEMORY_REGION_MMIO;
	c.memory_map.regions[1].type = JP_MEMORY_REGION_USABLE;
	c.memory_map.regions[1].base_address = SIM_BASE;
	/* Only memory-mapped I/O and unusable regions besides the pool region: still the pool region counts. */
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);

	/* No region at all that can be identity mapped (the pool needs a usable one, so build the pool first). */
	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);          /* the pool exists */
	c.memory_map.regions[0].type = JP_MEMORY_REGION_MMIO;
	c.memory_map.regions[1].type = JP_MEMORY_REGION_UNUSABLE;
	c.memory_map.regions[2].type = JP_MEMORY_REGION_MMIO;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);
}

static void test_framebuffer_inside_the_identity_map_changes_its_attributes(void)
{
	JPBootContext c;
	Translation t;

	make_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	c.framebuffer.base_address = SIM_BASE + 0x800000ULL;         /* inside a region: an identity page exists */
	c.framebuffer.width = 800;
	c.framebuffer.height = 600;
	c.framebuffer.pitch = 4096;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	t = translate(c.handoff.page_table_root, SIM_BASE + 0x800000ULL + 0x100);
	assert(t.present && t.large && t.physical == SIM_BASE + 0x800000ULL + 0x100);
	assert(t.cache_disabled && t.no_execute && t.writable);
	/* A page next to it keeps the ordinary identity attributes. */
	t = translate(c.handoff.page_table_root, SIM_BASE + 0x400000ULL);
	assert(t.present && !t.cache_disabled && !t.no_execute);
}

static void test_setup_paging_identity_and_gdt(void)
{
	JPBootContext c;
	uint64_t root;
	uint64_t gdt;
	Translation t;
	const uint64_t *entries;

	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_x86_64_map_segment(&c, 1) == JP_BOOT_SUCCESS);
	assert(!c.handoff.paging_enabled);
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(c.handoff.paging_enabled);
	root = c.handoff.page_table_root;

	/* The identity map: from 0 to the end of the highest ordinary region, 2 MiB pages, RWX. */
	t = translate(root, 0);
	assert(t.present && t.large && t.physical == 0 && t.writable && !t.no_execute);
	t = translate(root, 0x123456);
	assert(t.present && t.physical == 0x123456);
	t = translate(root, SIM_BASE + SIM_SIZE - 0x1000);
	assert(t.present && t.physical == SIM_BASE + SIM_SIZE - 0x1000 && t.writable && !t.no_execute);
	assert(!translate(root, SIM_BASE + SIM_SIZE + 0x400000).present);   /* beyond the memory */
	assert(!translate(root, 0xFEE00000ULL).present);                    /* memory-mapped I/O is not mapped */

	/* The kernel map is still there. */
	t = translate(root, KERNEL_VIRTUAL);
	assert(t.present && t.physical == SIM_BASE + 0x100000);

	/* The descriptor table: null, 64-bit code, data. */
	gdt = c.handoff.page_table_memory_base + PAGE;
	entries = (const uint64_t *)host_pointer(gdt);
	assert(entries[0] == 0);
	assert(entries[1] == 0x00AF9A000000FFFFULL);
	assert(entries[2] == 0x00CF92000000FFFFULL);

	/* Only once. */
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_FAILED);
}

static void test_setup_paging_without_segments_mapped_before(void)
{
	JPBootContext c;

	/* No segment was mapped yet: the pool is created here. */
	make_context(&c);
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(c.handoff.page_table_memory_base == expected_pool_base());
	assert(c.handoff.paging_enabled);
	assert(translate(c.handoff.page_table_root, 0).present);
	assert(jp_x86_64_setup_paging(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
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
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	t = translate(c.handoff.page_table_root, 0x80000000ULL + 0x1234);
	assert(t.present && t.large && t.physical == 0x80000000ULL + 0x1234);
	assert(t.writable && t.no_execute && t.cache_disabled);
	/* The last byte of the framebuffer is mapped, the page after the last 2 MiB page is not. */
	t = translate(c.handoff.page_table_root, 0x80000000ULL + 4096ULL * 768 - 1);
	assert(t.present);
	assert(!translate(c.handoff.page_table_root, 0x80000000ULL + 0x400000).present);

	/* Without a detected framebuffer it is not mapped. */
	make_context(&c);
	c.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	c.framebuffer.base_address = 0x80000000ULL;
	c.framebuffer.pitch = 4096;
	c.framebuffer.height = 768;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(!translate(c.handoff.page_table_root, 0x80000000ULL).present);
}

static void test_setup_paging_conflicts_and_limits(void)
{
	JPBootContext c;

	/* A kernel segment that lies inside the identity map (a low-half kernel) conflicts with it. */
	make_context(&c);
	c.kernel.segments[0].virtual_address = 0x100000;       /* inside the 2 MiB page of the first region */
	c.kernel.entry_point = 0x100000;
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);

	/* Memory so large that the identity map does not fit in the pool (130 GiB need 130 page directories). */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = 1ULL << 40;
	c.memory_map.regions[3].length = 130ULL << 30;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_USABLE;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_FAILED);
	assert(!c.handoff.paging_enabled);
}

static void test_identity_map_follows_the_regions(void)
{
	JPBootContext c;
	uint64_t root;

	make_context(&c);
	c.memory_map.region_count = 6;
	/* A reserved address range as large as the firmware reports near 1 TiB: not memory in use. */
	c.memory_map.regions[3].base_address = 0xFD00000000ULL;
	c.memory_map.regions[3].length = 12ULL << 30;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_RESERVED;
	/* A reserved region just below the limit is memory in use (loader data): it is mapped. */
	c.memory_map.regions[4].base_address = 0x80000000ULL;
	c.memory_map.regions[4].length = JP_X86_64_IDENTITY_RESERVED_MAX - 0x200000;
	c.memory_map.regions[4].type = JP_MEMORY_REGION_RESERVED;
	/* ACPI and firmware regions are mapped; a hole is not. */
	c.memory_map.regions[5].base_address = 0x40000000ULL;
	c.memory_map.regions[5].length = 0x200000;
	c.memory_map.regions[5].type = JP_MEMORY_REGION_ACPI;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	root = c.handoff.page_table_root;

	assert(!translate(root, 0xFD00000000ULL).present);             /* the huge reservation */
	assert(!translate(root, 0xFD00000000ULL + (6ULL << 30)).present);
	assert(translate(root, 0x80000000ULL + 0x1000).present);       /* the smaller reserved region */
	assert(translate(root, 0x40000000ULL + 0x1000).present);       /* ACPI */
	assert(!translate(root, 0x60000000ULL).present);               /* a hole between regions */
	assert(translate(root, 0x1000).present);                       /* the low region */

	/* Exactly at the limit: a reserved region of 1 GiB is not mapped. */
	make_context(&c);
	c.memory_map.region_count = 4;
	c.memory_map.regions[3].base_address = 0xC0000000ULL;
	c.memory_map.regions[3].length = JP_X86_64_IDENTITY_RESERVED_MAX;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_RESERVED;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(!translate(c.handoff.page_table_root, 0xC0000000ULL + 0x1000).present);
}

static void test_regions_that_share_a_page_are_accepted(void)
{
	JPBootContext c;

	make_context(&c);
	c.memory_map.region_count = 5;
	/* Two regions inside one 2 MiB page: the page is mapped once and the second region is accepted. */
	c.memory_map.regions[3].base_address = 0x70000000ULL;
	c.memory_map.regions[3].length = 0x1000;
	c.memory_map.regions[3].type = JP_MEMORY_REGION_ACPI;
	c.memory_map.regions[4].base_address = 0x70001000ULL;
	c.memory_map.regions[4].length = 0x1000;
	c.memory_map.regions[4].type = JP_MEMORY_REGION_FIRMWARE;
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	assert(translate(c.handoff.page_table_root, 0x70001000ULL).present);
}

static void test_prepare_entry(void)
{
	JPBootContext c;

	make_context(&c);
	assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS);
	assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS);
	c.handoff.stack_pointer = SIM_BASE + 0x300000;
	c.handoff.architecture = JP_ARCH_UNKNOWN;                /* prepare_entry records it */

	assert(jp_x86_64_prepare_entry(&c) == JP_BOOT_SUCCESS);
	assert(disable_calls == 1);                              /* cli really ran */
	assert(c.handoff.kernel_entry == KERNEL_VIRTUAL);
	assert(c.handoff.architecture == JP_ARCH_X86_64);
	assert(c.handoff.interrupts_disabled);

	assert(jp_x86_64_prepare_entry(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_prepare_entry_refusals(void)
{
	JPBootContext c;

	/* Each failure writes nothing and does not disable interrupts. */
#define PREPARE_FAILS(setup_statement) \
	do { \
		make_context(&c); \
		assert(jp_x86_64_map_segment(&c, 0) == JP_BOOT_SUCCESS); \
		assert(jp_x86_64_setup_paging(&c) == JP_BOOT_SUCCESS); \
		c.handoff.stack_pointer = SIM_BASE + 0x300000; \
		setup_statement; \
		assert(jp_x86_64_prepare_entry(&c) == JP_BOOT_FAILED); \
		assert(disable_calls == 0); \
		assert(!c.handoff.interrupts_disabled && c.handoff.kernel_entry == 0); \
	} while (0)

	PREPARE_FAILS(c.handoff.page_table_root = 0);
	PREPARE_FAILS(c.handoff.paging_enabled = false);
	PREPARE_FAILS(c.handoff.stack_pointer = 0);
	PREPARE_FAILS(c.handoff.stack_pointer += 8);                       /* not aligned */
	PREPARE_FAILS(c.kernel.entry_point = 0);
	PREPARE_FAILS(c.kernel.entry_point = 0x0000800000000000ULL);       /* not canonical */
	PREPARE_FAILS(c.kernel.entry_point = 0xFFFF000000000000ULL);       /* not canonical (bit 47 clear) */
#undef PREPARE_FAILS
}

static void ready_context(JPBootContext *c)
{
	make_context(c);
	assert(jp_x86_64_map_segment(c, 0) == JP_BOOT_SUCCESS);
	c->kernel.segments[0].state = JP_KERNEL_SEGMENT_PERMISSIONED;
	assert(jp_x86_64_setup_paging(c) == JP_BOOT_SUCCESS);
	c->handoff.stack_pointer = SIM_BASE + 0x300000;
	assert(jp_x86_64_prepare_entry(c) == JP_BOOT_SUCCESS);
}

static void test_validate_entry_state(void)
{
	JPBootContext c;
	JPBootContext before;

	ready_context(&c);
	before = c;
	assert(jp_x86_64_validate_entry_state(&c) == JP_BOOT_SUCCESS);
	assert(memcmp(&before, &c, sizeof c) == 0);                  /* nothing changed */
	assert(jp_x86_64_validate_entry_state(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

#define VALIDATE_FAILS(statement) \
	do { \
		ready_context(&c); \
		statement; \
		assert(jp_x86_64_validate_entry_state(&c) == JP_BOOT_FAILED); \
	} while (0)

	/* The real processor state. */
	VALIDATE_FAILS(fake_cpu_state.rflags |= JP_X86_64_RFLAGS_IF);
	VALIDATE_FAILS(fake_cpu_state.efer = 0);
	VALIDATE_FAILS(fake_cpu_state.cr0 = 0);
	/* The normalized state. */
	VALIDATE_FAILS(c.handoff.architecture = JP_ARCH_AARCH64);
	VALIDATE_FAILS(c.handoff.paging_enabled = false);
	VALIDATE_FAILS(c.handoff.interrupts_disabled = false);
	VALIDATE_FAILS(c.handoff.page_table_root = 0);
	VALIDATE_FAILS(c.handoff.page_table_root += 8);                       /* not page aligned */
	VALIDATE_FAILS(c.handoff.stack_pointer = 0);
	VALIDATE_FAILS(c.handoff.stack_pointer += 4);
	VALIDATE_FAILS(c.handoff.kernel_entry += 4);                          /* differs from the kernel entry */
	VALIDATE_FAILS(c.handoff.kernel_entry = 0x0000800000000000ULL; c.kernel.entry_point = 0x0000800000000000ULL);
	/* The entry must lie in an executable segment that is PERMISSIONED. */
	VALIDATE_FAILS(c.kernel.segments[0].state = JP_KERNEL_SEGMENT_MAPPED);
	VALIDATE_FAILS(c.kernel.segments[0].flags = 4);
	VALIDATE_FAILS(c.handoff.kernel_entry = KERNEL_VIRTUAL + 0x10000; c.kernel.entry_point = KERNEL_VIRTUAL + 0x10000);
	VALIDATE_FAILS(c.handoff.kernel_entry = KERNEL_VIRTUAL + 0x2800; c.kernel.entry_point = KERNEL_VIRTUAL + 0x2800);   /* the first byte after the segment */
#undef VALIDATE_FAILS
}

static void test_enter_kernel(void)
{
	JPBootContext c;
	int outcome;

	/* A context that is READY: the five values reach the jump. */
	ready_context(&c);
	c.handoff.state = JP_HANDOFF_STATE_READY;
	c.kernel.entry_point += 0x10;                     /* the jump uses handoff.kernel_entry, which was validated */
	memset(&jump_record, 0, sizeof jump_record);
	outcome = setjmp(jump_buffer);
	if (outcome == 0) {
		jp_x86_64_enter_kernel(&c);
	}
	assert(outcome == 1 && jump_record.jumps == 1 && jump_record.parks == 0);
	assert(jump_record.root == c.handoff.page_table_root);
	assert(jump_record.gdt == c.handoff.page_table_memory_base + PAGE);
	assert(jump_record.stack == c.handoff.stack_pointer);
	assert(jump_record.entry == c.handoff.kernel_entry);
	assert(jump_record.context == &c);

	/* It does not jump unless the handoff is READY and paging is enabled. */
#define ENTER_PARKS(statement) \
	do { \
		ready_context(&c); \
		c.handoff.state = JP_HANDOFF_STATE_READY; \
		statement; \
		memset(&jump_record, 0, sizeof jump_record); \
		outcome = setjmp(jump_buffer); \
		if (outcome == 0) { \
			jp_x86_64_enter_kernel(&c); \
		} \
		assert(outcome == 2 && jump_record.parks == 1 && jump_record.jumps == 0); \
	} while (0)

	ENTER_PARKS(c.handoff.state = JP_HANDOFF_STATE_PLATFORM_PREPARED);
	ENTER_PARKS(c.handoff.state = JP_HANDOFF_STATE_FAILED);
	ENTER_PARKS(c.handoff.paging_enabled = false);
	ENTER_PARKS(c.handoff.page_table_root = 0);
#undef ENTER_PARKS

	/* A null context parks too. */
	memset(&jump_record, 0, sizeof jump_record);
	outcome = setjmp(jump_buffer);
	if (outcome == 0) {
		jp_x86_64_enter_kernel(NULL);
	}
	assert(outcome == 2 && jump_record.parks == 1);
}

int main(void)
{
	simulated_memory = (uint8_t *)malloc(SIM_SIZE);
	assert(simulated_memory != NULL);
	jp_x86_64_test_phys_to_pointer = host_pointer;

	test_validate_elf_machine();
	test_discover_cpu();
	test_phys_access();
	test_first_mapping_creates_the_pool();
	test_segment_mapping_and_permissions();
	test_segment_flag_combinations();
	test_map_segment_preconditions();
	test_pool_placement_failures();
	test_refused_pool_reservation_tries_lower_places();
	test_pool_exhaustion_and_conflicts();
	test_segment_in_an_identity_large_page_conflicts();
	test_non_canonical_virtual_address();
	test_setup_paging_needs_an_ordinary_region();
	test_framebuffer_inside_the_identity_map_changes_its_attributes();
	test_setup_paging_identity_and_gdt();
	test_setup_paging_without_segments_mapped_before();
	test_setup_paging_maps_the_framebuffer();
	test_setup_paging_conflicts_and_limits();
	test_identity_map_follows_the_regions();
	test_regions_that_share_a_page_are_accepted();
	test_prepare_entry();
	test_prepare_entry_refusals();
	test_validate_entry_state();
	test_enter_kernel();

	free(simulated_memory);
	return 0;
}
