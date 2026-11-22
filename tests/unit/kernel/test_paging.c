/*
 * Unit test (level U) of src/kernel/paging.c, for both architectures. The
 * machine is a fake: a buffer stands for the table pool, and the root
 * register, WXN, and the sequence of register writes are recorded. The real
 * memory.c builds the ownership table. The tables that paging.c writes are
 * read back by a decoder written here, independent of paging.c.
 */

#include <assert.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"
#include "paging_hw.h"

#define PAGE 0x1000ull
#define LARGE 0x200000ull

/* ---- The fake machine ---- */

#define POOL_BASE 0x4200000ull
#define POOL_PAGES 128u
#define POOL_SIZE (POOL_PAGES * PAGE)

static uint64_t pool_memory[POOL_PAGES * 512];
static uint64_t fake_root;
static unsigned int root_writes;
static unsigned int wxn_calls;
static bool ignore_root_write;
static bool ignore_wxn;
static uint64_t fake_sctlr;
static unsigned int table_at_calls;
static unsigned int table_at_fail_at;   /* the call number that returns NULL (0: never) */
static char events[16];
static size_t event_count;

static void record(char event)
{
	assert(event_count < sizeof events);
	events[event_count++] = event;
}

uint64_t *jp_kernel_paging_table_at(uint64_t physical)
{
	table_at_calls++;
	if (table_at_fail_at != 0 && table_at_calls == table_at_fail_at) {
		return NULL;
	}
	if (physical < POOL_BASE || physical >= POOL_BASE + POOL_SIZE || (physical & (PAGE - 1)) != 0) {
		return NULL;
	}
	return &pool_memory[(physical - POOL_BASE) / 8];
}

uint64_t jp_kernel_paging_read_root(void)
{
	return fake_root;
}

void jp_kernel_paging_write_root(uint64_t root)
{
	root_writes++;
	record('R');
	if (!ignore_root_write) {
		fake_root = root;
	}
}

void jp_kernel_aarch64_set_wxn(void)
{
	wxn_calls++;
	record('W');
	if (!ignore_wxn) {
		fake_sctlr |= 1ull << 19;
	}
}

void jp_kernel_aarch64_read_registers(JPKernelAArch64Registers *registers)
{
	memset(registers, 0, sizeof *registers);
	registers->sctlr_el1 = fake_sctlr;
}

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	return 48;
}

/* ---- The layout ---- */

#define LOW_BASE 0x1000ull
#define LOW_END 0x100000ull
#define MAIN_BASE 0x100000ull
#define MAIN_END 0x4000000ull
#define RES_BASE 0x4000000ull
#define RES_END 0x4400000ull
#define HIGH_BASE 0x4400000ull
#define HIGH_END 0x8000000ull
#define ACPI_BASE 0x9000000ull
#define ACPI_END 0x9100000ull
#define BAD_BASE 0xA000000ull
#define BAD_END 0xA001000ull
#define MMIO_BASE 0xFEE00000ull
#define MMIO_END 0xFEE01000ull
#define BIG_BASE 0x100000000ull           /* a reserved region of 1 GiB: not mapped */
#define BIG_END 0x140000000ull
#define NEAR_BASE 0x300000000ull
#define NEAR_SIZE 0x3FE00000ull
#define MMIO_BIG_BASE 0x200000000ull
#define MID_BASE 0x8100000ull
#define MID_SIZE 0x300000ull
#define STACK_TOP 0x4100000ull
#define FB_BASE 0xFD000000ull
#define FB_PITCH 4096ull
#define FB_HEIGHT 4ull

#define SEG0 0x4000000ull                 /* text: execute */
#define SEG1 0x4001000ull                 /* rodata: read */
#define SEG2 0x4002000ull                 /* data: read and write, 0x1800 bytes */

static JPBootContext context;

static uint64_t virtual_base(void)
{
	return context.handoff.architecture == JP_ARCH_X86_64 ? 0xFFFFFFFF80000000ull : 0xFFFF000000000000ull;
}

/* ---- The decoder ---- */

typedef struct Translation {
	uint64_t physical;
	unsigned int level;        /* 0: a 4 KiB leaf, 1: a 2 MiB leaf */
	bool writable;
	bool executable;
	bool special;              /* x86_64: cache disabled; AArch64: device memory */
} Translation;

static uint64_t entry_address(uint64_t entry)
{
	return entry & 0x0000FFFFFFFFF000ull;
}

static bool walk(uint64_t root, uint64_t address, Translation *out)
{
	uint64_t table = root;
	unsigned int shift;
	unsigned int level;
	bool x86 = context.handoff.architecture == JP_ARCH_X86_64;

	for (level = 3; ; level--) {
		uint64_t *entries = jp_kernel_paging_table_at(table);
		uint64_t entry;

		shift = 12 + 9 * level;
		if (entries == NULL) {
			return false;
		}
		entry = entries[(address >> shift) & 511];
		if (x86) {
			if ((entry & 1) == 0) {
				return false;
			}
			if (level == 0 || (level == 1 && (entry & 0x80) != 0)) {
				out->physical = entry_address(entry) | (address & ((1ull << shift) - 1));
				out->level = level;
				out->writable = (entry & 2) != 0;
				out->executable = (entry & (1ull << 63)) == 0;
				out->special = (entry & 0x10) != 0;
				return true;
			}
			if (level == 2 && (entry & 0x80) != 0) {
				return false;     /* a 1 GiB page: not used */
			}
		} else {
			if ((entry & 1) == 0) {
				return false;
			}
			if (level == 0 && (entry & 3) == 3) {
				out->physical = entry_address(entry) | (address & ((1ull << shift) - 1));
				out->level = 0;
				out->writable = (entry & (1ull << 7)) == 0;
				out->executable = (entry & (1ull << 53)) == 0;
				out->special = ((entry >> 2) & 7) == 0;
				assert((entry & (1ull << 54)) != 0);     /* UXN is always set */
				assert((entry & (1ull << 10)) != 0);     /* access flag */
				return true;
			}
			if (level == 1 && (entry & 3) == 1) {
				out->physical = entry_address(entry) | (address & ((1ull << shift) - 1));
				out->level = 1;
				out->writable = (entry & (1ull << 7)) == 0;
				out->executable = (entry & (1ull << 53)) == 0;
				out->special = ((entry >> 2) & 7) == 0;
				assert((entry & (1ull << 54)) != 0);
				assert((entry & (1ull << 10)) != 0);
				return true;
			}
			if (level == 2 && (entry & 3) == 1) {
				return false;
			}
			if ((entry & 3) != 3 || level == 0) {
				return false;
			}
		}
		table = entry_address(entry);
		if (level == 0) {
			return false;
		}
	}
}

/* The root of the identity map in the fake machine, and of the kernel half. */
static uint64_t kernel_root(void)
{
	return context.handoff.architecture == JP_ARCH_X86_64 ? fake_root : context.handoff.page_table_root;
}

/* ---- Building the Stage 1 state (the old tables) ---- */

static uint64_t take_page(void)
{
	uint64_t page = context.handoff.page_table_memory_used++;

	assert(page < POOL_PAGES);
	return POOL_BASE + page * PAGE;
}

static uint64_t *page_pointer(uint64_t physical)
{
	return jp_kernel_paging_table_at(physical);
}

/* Maps one page at a virtual address the way Stage 1 does for a kernel segment (a test helper). */
static void stage1_map(uint64_t root, uint64_t virtual_address, uint64_t physical, uint32_t flags)
{
	bool x86 = context.handoff.architecture == JP_ARCH_X86_64;
	uint64_t table = root;
	unsigned int level;

	for (level = 3; level > 0; level--) {
		uint64_t *entries = page_pointer(table);
		uint64_t *entry = &entries[(virtual_address >> (12 + 9 * level)) & 511];

		if ((*entry & 1) == 0) {
			uint64_t next = take_page();

			*entry = x86 ? (next | 3) : (next | 3);
		}
		table = entry_address(*entry);
	}
	{
		uint64_t *entries = page_pointer(table);
		uint64_t leaf;

		if (x86) {
			leaf = physical | 1 | ((flags & JP_ELF_PF_W) != 0 ? 2 : 0) | ((flags & JP_ELF_PF_X) != 0 ? 0 : (1ull << 63));
		} else {
			leaf = physical | 3 | (1ull << 2) | (3ull << 8) | (1ull << 10) | (1ull << 54) |
				((flags & JP_ELF_PF_W) != 0 ? 0 : (1ull << 7)) | ((flags & JP_ELF_PF_X) != 0 ? 0 : (1ull << 53));
		}
		entries[(virtual_address >> 12) & 511] = leaf;
	}
}

static void add_region(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	JPMemoryRegion *r = &context.memory_map.regions[context.memory_map.region_count++];

	r->base_address = base;
	r->length = length;
	r->type = type;
}

static void set_segment(size_t i, uint64_t physical, uint64_t size, uint32_t flags, uint64_t virtual_offset)
{
	context.kernel.segments[i].physical_address = physical;
	context.kernel.segments[i].virtual_address = virtual_base() + virtual_offset;
	context.kernel.segments[i].memory_size = size;
	context.kernel.segments[i].flags = flags;
}

static void setup_for(JPArchitectureType architecture)
{
	uint64_t high_root;
	size_t i;

	memset(&context, 0, sizeof context);
	memset(pool_memory, 0, sizeof pool_memory);
	root_writes = 0;
	wxn_calls = 0;
	ignore_root_write = false;
	ignore_wxn = false;
	fake_sctlr = 1;
	event_count = 0;
	table_at_calls = 0;
	table_at_fail_at = 0;

	context.handoff.architecture = architecture;
	context.handoff.paging_enabled = true;
	context.handoff.state = JP_HANDOFF_STATE_READY;

	context.memory_map.state = JP_DETECTION_STATE_DETECTED;
	add_region(LOW_BASE, LOW_END - LOW_BASE, JP_MEMORY_REGION_USABLE);
	add_region(MAIN_BASE, MAIN_END - MAIN_BASE, JP_MEMORY_REGION_USABLE);
	add_region(RES_BASE, RES_END - RES_BASE, JP_MEMORY_REGION_RESERVED);
	add_region(HIGH_BASE, HIGH_END - HIGH_BASE, JP_MEMORY_REGION_USABLE);
	add_region(ACPI_BASE, ACPI_END - ACPI_BASE, JP_MEMORY_REGION_ACPI);
	add_region(BAD_BASE, BAD_END - BAD_BASE, JP_MEMORY_REGION_UNUSABLE);
	add_region(MMIO_BASE, MMIO_END - MMIO_BASE, JP_MEMORY_REGION_MMIO);
	add_region(BIG_BASE, BIG_END - BIG_BASE, JP_MEMORY_REGION_RESERVED);
	add_region(NEAR_BASE, NEAR_SIZE, JP_MEMORY_REGION_RESERVED);              /* just under 1 GiB: mapped */
	add_region(MMIO_BIG_BASE, 0x40000000ull, JP_MEMORY_REGION_MMIO);         /* exactly 1 GiB: not mapped */
	add_region(MID_BASE, MID_SIZE, JP_MEMORY_REGION_USABLE);                  /* starts inside a block, ends after it */
	{
		uint64_t context_base = (uint64_t)(uintptr_t)&context & ~(PAGE - 1);
		uint64_t context_end = ((uint64_t)(uintptr_t)&context + sizeof context + PAGE - 1) & ~(PAGE - 1);

		add_region(context_base, context_end - context_base, JP_MEMORY_REGION_RESERVED);
	}

	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.kernel.segment_count = 3;
	set_segment(0, SEG0, 0x1000, JP_ELF_PF_X | JP_ELF_PF_R, 0);
	set_segment(1, SEG1, 0x1000, JP_ELF_PF_R, 0x1000);
	set_segment(2, SEG2, 0x1800, JP_ELF_PF_W | JP_ELF_PF_R, 0x2000);

	context.handoff.stack_pointer = STACK_TOP;
	context.handoff.page_table_memory_base = POOL_BASE;
	context.handoff.page_table_memory_size = POOL_SIZE;

	context.framebuffer.state = JP_DETECTION_STATE_DETECTED;
	context.framebuffer.base_address = FB_BASE;
	context.framebuffer.pitch = FB_PITCH;
	context.framebuffer.height = FB_HEIGHT;

	/* Stage 1 tables: the kernel half (and on AArch64 an empty old identity root). */
	high_root = take_page();
	context.handoff.page_table_root = high_root;
	if (architecture == JP_ARCH_AARCH64) {
		fake_root = take_page();               /* the old TTBR0 root */
	} else {
		fake_root = high_root;                 /* CR3 */
	}
	for (i = 0; i < 3; i++) {
		uint64_t pages = (context.kernel.segments[i].memory_size + PAGE - 1) / PAGE;
		uint64_t p;

		for (p = 0; p < pages; p++) {
			stage1_map(high_root, context.kernel.segments[i].virtual_address + p * PAGE,
				context.kernel.segments[i].physical_address + p * PAGE, context.kernel.segments[i].flags);
		}
	}
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
}

static void setup_x86(void)
{
	setup_for(JP_ARCH_X86_64);
}

static void setup_arm(void)
{
	setup_for(JP_ARCH_AARCH64);
}

/* ---- Expectations ---- */

static Translation must_walk(uint64_t address)
{
	Translation t;

	assert(walk(fake_root, address, &t));
	assert(t.physical == address);     /* the identity map */
	return t;
}

static void expect_unmapped(uint64_t address)
{
	Translation t;

	assert(!walk(fake_root, address, &t));
}

static void expect_normal(uint64_t address)
{
	Translation t = must_walk(address);

	assert(t.writable && !t.executable && !t.special);
}

static void check_identity_map(void)
{
	bool x86 = context.handoff.architecture == JP_ARCH_X86_64;
	Translation t;

	/* kernel segments: the permissions of their flags, 4 KiB pages */
	t = must_walk(SEG0);
	assert(!t.writable && t.executable && t.level == 0);
	t = must_walk(SEG0 + 0xFFF);
	assert(!t.writable && t.executable);
	t = must_walk(SEG1);
	assert(!t.writable && !t.executable && t.level == 0);
	t = must_walk(SEG2);
	assert(t.writable && !t.executable && t.level == 0);
	t = must_walk(SEG2 + PAGE);          /* the second page of the 0x1800 bytes */
	assert(t.writable && !t.executable);
	expect_normal(SEG2 + 2 * PAGE);      /* after the segment: ordinary reserved memory */

	/* ordinary memory: read and write, no execute; whole blocks are 2 MiB leaves */
	expect_normal(RES_BASE + 0x300000);
	expect_normal(MAIN_BASE + 0x500000);
	assert(must_walk(MAIN_BASE + 0x500000).level == 1);
	assert(must_walk(HIGH_BASE + 0x1000000).level == 1);
	expect_normal(LOW_BASE);
	expect_normal(STACK_TOP - 1);
	expect_normal(POOL_BASE);
	expect_normal(POOL_BASE + POOL_SIZE - 1);
	expect_normal(ACPI_BASE);
	expect_normal((uint64_t)(uintptr_t)&context);
	assert(must_walk(RES_BASE + 0x300000).level == 1);     /* a clean block in the reserved region */

	/* the framebuffer: outside the map, uncached */
	t = must_walk(FB_BASE);
	assert(t.writable && !t.executable && t.special);
	t = must_walk(FB_BASE + FB_PITCH * FB_HEIGHT - 1);
	assert(t.special);

	/* what is not mapped */
	expect_normal(0);                     /* page 0 is in a block that the first region touches, as in Stage 1 */
	expect_unmapped(0x20000000);          /* a hole between regions */
	expect_unmapped(BAD_BASE);
	expect_unmapped(BIG_BASE);            /* a reserved region of 1 GiB or more */
	expect_unmapped(MMIO_BIG_BASE);       /* an MMIO region of 1 GiB or more, on both architectures */
	expect_normal(NEAR_BASE + 0x20000000);   /* a reserved region just under 1 GiB is mapped */
	expect_unmapped(FB_BASE + 0x100000);  /* after the framebuffer, in the same 2 MiB block */
	expect_normal(MID_BASE - PAGE);       /* a region is rounded outwards to whole 2 MiB blocks */
	expect_normal(MID_BASE);
	expect_normal(MID_BASE + MID_SIZE - 1);
	expect_unmapped(HIGH_END + 0x400000);
	if (x86) {
		expect_unmapped(MMIO_BASE);
	} else {
		t = must_walk(MMIO_BASE);
		assert(t.writable && !t.executable && t.special);
		t = must_walk(MMIO_BASE + 0x100000);       /* the whole 2 MiB block around a small MMIO region */
		assert(t.writable && !t.executable && t.special && t.level == 1);
	}
}

static void check_kernel_half(void)
{
	Translation t;
	uint64_t root = kernel_root();

	assert(walk(root, virtual_base(), &t));
	assert(t.physical == SEG0 && t.executable && !t.writable);
	assert(walk(root, virtual_base() + 0x1000, &t));
	assert(t.physical == SEG1 && !t.executable && !t.writable);
	assert(walk(root, virtual_base() + 0x2000, &t));
	assert(t.physical == SEG2 && !t.executable && t.writable);
}

/* ---- Tests: success ---- */

static void test_x86_success(void)
{
	uint64_t old_root;
	uint64_t high_entries[256];
	uint64_t used_before;
	uint64_t *old;

	setup_x86();
	old_root = fake_root;
	used_before = context.handoff.page_table_memory_used;
	old = page_pointer(old_root);
	memcpy(high_entries, &old[256], sizeof high_entries);

	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);

	/* a new root from the pool, written once, and no WXN */
	assert(root_writes == 1 && wxn_calls == 0);
	assert(fake_root == POOL_BASE + used_before * PAGE);
	assert(fake_root != old_root);
	assert(context.handoff.page_table_memory_used > used_before);
	assert(context.handoff.page_table_memory_used <= POOL_PAGES);

	/* the kernel half of the old root is in the new root, the lower half is the new map */
	assert(memcmp(high_entries, &page_pointer(fake_root)[256], sizeof high_entries) == 0);
	check_identity_map();
	check_kernel_half();
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_SUCCESS);
}

static void test_arm_success(void)
{
	uint64_t old_ttbr0;
	uint64_t used_before;
	uint64_t ttbr1;
	uint64_t ttbr1_page[512];

	setup_arm();
	old_ttbr0 = fake_root;
	ttbr1 = context.handoff.page_table_root;
	used_before = context.handoff.page_table_memory_used;
	memcpy(ttbr1_page, page_pointer(ttbr1), sizeof ttbr1_page);

	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);

	assert(root_writes == 1 && wxn_calls == 1);
	assert(event_count == 2 && events[0] == 'R' && events[1] == 'W');   /* the root first, then WXN */
	assert(fake_root == POOL_BASE + used_before * PAGE);
	assert(fake_root != old_ttbr0);
	assert((fake_sctlr & (1ull << 19)) != 0);
	/* the kernel half is untouched */
	assert(context.handoff.page_table_root == ttbr1);
	assert(memcmp(ttbr1_page, page_pointer(ttbr1), sizeof ttbr1_page) == 0);
	check_identity_map();
	check_kernel_half();
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_SUCCESS);
}

static void test_deterministic(void)
{
	static uint64_t first[POOL_PAGES * 512];

	setup_x86();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	memcpy(first, pool_memory, sizeof first);
	setup_x86();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(first, pool_memory, sizeof first) == 0);

	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	memcpy(first, pool_memory, sizeof first);
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(first, pool_memory, sizeof first) == 0);
}

static void test_segment_permission_variants(void)
{
	Translation t;

	/* a segment without any flag is read only; read and execute together; write alone */
	setup_x86();
	context.kernel.segments[1].flags = 0;
	context.kernel.segments[0].flags = JP_ELF_PF_X;
	context.kernel.segments[2].flags = JP_ELF_PF_W;
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	t = must_walk(SEG1);
	assert(!t.writable && !t.executable);
	t = must_walk(SEG0);
	assert(!t.writable && t.executable);
	t = must_walk(SEG2);
	assert(t.writable && !t.executable);

	/* two segments that share a page with the same permission are accepted */
	setup_x86();
	context.kernel.segment_count = 4;
	set_segment(3, SEG2 + 0x1000, 0x800, JP_ELF_PF_W | JP_ELF_PF_R, 0x3000);   /* shares page 2 of segment 2 */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
}

/* ---- Tests: refusals ---- */

static void expect_refused(JPBootResult expected)
{
	uint64_t root_before = fake_root;

	assert(jp_kernel_paging_initialize(&context) == expected);
	assert(root_writes == 0 && wxn_calls == 0);   /* a refused call never writes a register */
	assert(fake_root == root_before);
}

static void test_refusals(void)
{
	assert(jp_kernel_paging_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	setup_x86();
	context.stage2.entry_count = 0;     /* memory not initialized */
	expect_refused(JP_BOOT_NOT_INITIALIZED);

	setup_x86();
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	expect_refused(JP_BOOT_FAILED);

	setup_x86();
	context.handoff.paging_enabled = false;
	expect_refused(JP_BOOT_FAILED);

	setup_x86();
	fake_root = 0;                       /* an old root that is zero */
	expect_refused(JP_BOOT_FAILED);
	setup_x86();
	fake_root = POOL_BASE + 0x10;        /* not page aligned */
	expect_refused(JP_BOOT_FAILED);

	setup_arm();
	context.handoff.page_table_root = 0;
	expect_refused(JP_BOOT_FAILED);

	/* the pool that the handoff names is not what the ownership table says */
	setup_x86();
	context.handoff.page_table_memory_base += PAGE;
	expect_refused(JP_BOOT_FAILED);
	setup_arm();
	context.handoff.page_table_memory_size = 0;
	expect_refused(JP_BOOT_FAILED);
	setup_x86();
	context.handoff.page_table_memory_used = POOL_PAGES + 1;
	expect_refused(JP_BOOT_FAILED);

	/* a pool with no free page */
	setup_x86();
	context.handoff.page_table_memory_used = POOL_PAGES;
	expect_refused(JP_BOOT_FAILED);
	setup_arm();
	context.handoff.page_table_memory_used = POOL_PAGES;
	expect_refused(JP_BOOT_FAILED);
	/* a pool that runs out in the middle of the build */
	setup_x86();
	context.handoff.page_table_memory_used = POOL_PAGES - 3;
	expect_refused(JP_BOOT_FAILED);
	setup_arm();
	context.handoff.page_table_memory_used = POOL_PAGES - 3;
	expect_refused(JP_BOOT_FAILED);

	/* a segment that is writable and executable */
	setup_x86();
	context.kernel.segments[2].flags = JP_ELF_PF_W | JP_ELF_PF_X;
	expect_refused(JP_BOOT_FAILED);
	setup_arm();
	context.kernel.segments[2].flags = JP_ELF_PF_W | JP_ELF_PF_X | JP_ELF_PF_R;
	expect_refused(JP_BOOT_FAILED);

	/* two segments with different permissions in one page */
	setup_x86();
	context.kernel.segment_count = 4;
	set_segment(3, SEG2 + 0x1000, 0x800, JP_ELF_PF_R, 0x3000);        /* read only; the page also has segment 2 (write) */
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	expect_refused(JP_BOOT_FAILED);

	/* the framebuffer overlapping a segment (memory.c would have refused it; the ownership table is already built) */
	setup_arm();
	context.framebuffer.base_address = SEG1;
	expect_refused(JP_BOOT_FAILED);
	/* a framebuffer at address zero or of an impossible size */
	setup_arm();
	context.framebuffer.base_address = 0;
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);
	/* a segment that overflows */
	setup_x86();
	context.kernel.segments[0].physical_address = UINT64_MAX - 0x100;
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_register_readback(void)
{
	/* the write instruction ran but the root did not change */
	setup_x86();
	ignore_root_write = true;
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_FAILED);
	assert(root_writes == 1);

	setup_arm();
	ignore_root_write = true;
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_FAILED);
	assert(root_writes == 1 && wxn_calls == 0);     /* WXN is not set on a map that is not active */

	/* WXN did not come on */
	setup_arm();
	ignore_wxn = true;
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_FAILED);
	assert(wxn_calls == 1);
}

/* ---- Tests: validate ---- */

static uint64_t *leaf_entry(uint64_t address, unsigned int *level)
{
	uint64_t table = fake_root;
	unsigned int l;

	for (l = 3; ; l--) {
		uint64_t *entries = jp_kernel_paging_table_at(table);
		uint64_t *entry = &entries[(address >> (12 + 9 * l)) & 511];
		bool leaf;

		if (context.handoff.architecture == JP_ARCH_X86_64) {
			leaf = l == 0 || (l == 1 && (*entry & 0x80) != 0);
		} else {
			leaf = (l == 0) || (l == 1 && (*entry & 3) == 1);
		}
		if (leaf) {
			*level = l;
			return entry;
		}
		table = entry_address(*entry);
	}
}

#define DAMAGE_X86_WRITABLE 2ull
#define DAMAGE_X86_NX (1ull << 63)
#define DAMAGE_ARM_READ_ONLY (1ull << 7)
#define DAMAGE_ARM_PXN (1ull << 53)

static void test_validate(void)
{
	unsigned int level;
	uint64_t *entry;
	bool x86;
	int arch;

	assert(jp_kernel_paging_validate(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	for (arch = 0; arch < 2; arch++) {
		uint64_t writable_bit;
		uint64_t no_exec_bit;
		bool writable_is_set_bit;

		x86 = arch == 0;
		writable_bit = x86 ? DAMAGE_X86_WRITABLE : DAMAGE_ARM_READ_ONLY;
		no_exec_bit = x86 ? DAMAGE_X86_NX : DAMAGE_ARM_PXN;
		writable_is_set_bit = x86;

#define FRESH() do { if (x86) setup_x86(); else setup_arm(); assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS); } while (0)

		FRESH();
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_SUCCESS);

		/* a text page that became writable */
		FRESH();
		entry = leaf_entry(SEG0, &level);
		if (writable_is_set_bit) { *entry |= writable_bit; } else { *entry &= ~writable_bit; }
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* a text page that lost execute */
		FRESH();
		entry = leaf_entry(SEG0, &level);
		*entry |= no_exec_bit;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* a data page that became executable */
		FRESH();
		entry = leaf_entry(SEG2, &level);
		*entry &= ~no_exec_bit;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* a rodata page that became writable */
		FRESH();
		entry = leaf_entry(SEG1, &level);
		if (writable_is_set_bit) { *entry |= writable_bit; } else { *entry &= ~writable_bit; }
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* the stack became executable */
		FRESH();
		entry = leaf_entry(STACK_TOP - PAGE, &level);
		*entry &= ~no_exec_bit;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* the stack became read only */
		FRESH();
		entry = leaf_entry(STACK_TOP - PAGE, &level);
		if (writable_is_set_bit) { *entry &= ~writable_bit; } else { *entry |= writable_bit; }
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* a table page became executable */
		FRESH();
		entry = leaf_entry(POOL_BASE + 10 * PAGE, &level);
		*entry &= ~no_exec_bit;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* available memory is no longer mapped */
		FRESH();
		entry = leaf_entry(MAIN_BASE + 0x500000, &level);
		assert(level == 1);
		*entry = 0;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* the last page of an available block is gone */
		FRESH();
		entry = leaf_entry(HIGH_END - PAGE, &level);
		*entry &= ~1ull;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* a segment page that maps to another physical page */
		FRESH();
		entry = leaf_entry(SEG1, &level);
		*entry += PAGE;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* the kernel half: a page of the virtual address of a segment is gone, or maps elsewhere */
		FRESH();
		{
			uint64_t root = kernel_root();
			uint64_t table = root;
			unsigned int l;
			uint64_t *e = NULL;

			for (l = 3; ; l--) {
				e = &jp_kernel_paging_table_at(table)[((virtual_base() + 0x1000) >> (12 + 9 * l)) & 511];
				if (l == 0) {
					break;
				}
				table = entry_address(*e);
			}
			*e += PAGE;
			assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
			*e -= PAGE;
			assert(jp_kernel_paging_validate(&context) == JP_BOOT_SUCCESS);
			*e &= ~1ull;
			assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
		}

		/* the root register points outside the pool, is not aligned, or is zero */
		FRESH();
		fake_root = 0x1000;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
		fake_root = POOL_BASE + 0x800;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
		fake_root = 0;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

		/* the ownership table is gone */
		FRESH();
		context.stage2.entry_count = 0;
		assert(jp_kernel_paging_validate(&context) == JP_BOOT_NOT_INITIALIZED);
	}

	/* WXN is off */
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	fake_sctlr &= ~(1ull << 19);
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

	/* the old Stage 1 map (not replaced yet) is not a valid Stage 2 map: its root is inside the pool but the
	 * kernel half has no identity map */
	setup_x86();
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
	setup_arm();
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
}


/* ---- More tests (found by changing paging.c and watching which changes the tests miss) ---- */

/* The entry of `address` at the given level (3 is the top), following the tables; NULL if the path is missing. */
static uint64_t *entry_at_level(uint64_t root, uint64_t address, unsigned int wanted_level)
{
	uint64_t table = root;
	unsigned int level;

	for (level = 3; ; level--) {
		uint64_t *entries = jp_kernel_paging_table_at(table);
		uint64_t *entry;

		if (entries == NULL) {
			return NULL;
		}
		entry = &entries[(address >> (12 + 9 * level)) & 511];
		if (level == wanted_level) {
			return entry;
		}
		table = entry_address(*entry);
	}
}

/* Every table access in turn fails: the call must fail, write no register, and never crash. */
static void test_unreachable_tables(void)
{
	int arch;

	for (arch = 0; arch < 2; arch++) {
		unsigned int k;

		for (k = 1; k < 100000; k++) {
			JPBootResult result;

			if (arch == 0) {
				setup_x86();
			} else {
				setup_arm();
			}
			table_at_calls = 0;
			table_at_fail_at = k;
			result = jp_kernel_paging_initialize(&context);
			if (result == JP_BOOT_SUCCESS) {
				assert(k > 20);                 /* many accesses happen */
				break;
			}
			assert(result == JP_BOOT_FAILED);
			assert(root_writes == 0 && wxn_calls == 0);
		}
		assert(k < 100000);

		for (k = 1; k < 100000; k++) {
			JPBootResult result;

			if (arch == 0) {
				setup_x86();
			} else {
				setup_arm();
			}
			assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
			table_at_calls = 0;
			table_at_fail_at = k;
			result = jp_kernel_paging_validate(&context);
			if (result == JP_BOOT_SUCCESS) {
				assert(k > 20);
				break;
			}
			assert(result == JP_BOOT_FAILED);
		}
		assert(k < 100000);
	}
}

static void test_input_limits(void)
{
	/* no segment, too many segments (after memory.c built its table) */
	setup_x86();
	context.kernel.segment_count = 0;
	expect_refused(JP_BOOT_FAILED);
	setup_x86();
	context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS + 1;
	expect_refused(JP_BOOT_FAILED);
	setup_x86();
	context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS;      /* the empty slots have no size */
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);

	/* a segment whose end overflows only when it is rounded up to a page */
	setup_x86();
	context.kernel.segments[0].physical_address = UINT64_MAX - 0x2000;
	context.kernel.segments[0].memory_size = 0x1800;
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);
	/* the same for a region */
	setup_arm();
	context.memory_map.regions[0].base_address = UINT64_MAX - 0x2000;
	context.memory_map.regions[0].length = 0x1800;
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);
	/* the same for the framebuffer */
	setup_arm();
	context.framebuffer.base_address = UINT64_MAX - 0x2000;
	context.framebuffer.pitch = 0x1800;
	context.framebuffer.height = 1;
	expect_refused(JP_BOOT_ERROR_CORRUPTED_DATA);

	/* a framebuffer that is not detected is not mapped, whatever its address says */
	setup_x86();
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	expect_unmapped(FB_BASE);
}

static void test_mixed_kinds_in_one_block(void)
{
	/* normal and device memory in one 2 MiB block: a mistake in the map on AArch64; the MMIO region is skipped on x86_64 */
	setup_arm();
	add_region(ACPI_BASE + 0x150000, PAGE, JP_MEMORY_REGION_MMIO);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	expect_refused(JP_BOOT_FAILED);

	setup_x86();
	add_region(ACPI_BASE + 0x150000, PAGE, JP_MEMORY_REGION_MMIO);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	expect_normal(ACPI_BASE + 0x150000);

	/* two regions of the same kind that share a block are fine */
	setup_arm();
	add_region(ACPI_BASE + 0x150000, PAGE, JP_MEMORY_REGION_ACPI);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_identity_limit(void)
{
	/* a region that ends exactly at 2^47 is mapped; one that ends above is refused */
	setup_x86();
	add_region((1ull << 47) - LARGE, LARGE, JP_MEMORY_REGION_USABLE);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	expect_normal((1ull << 47) - PAGE);

	setup_arm();
	add_region((1ull << 47) - LARGE, LARGE, JP_MEMORY_REGION_USABLE);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	expect_normal((1ull << 47) - PAGE);

	setup_x86();
	add_region((1ull << 47), LARGE, JP_MEMORY_REGION_USABLE);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	expect_refused(JP_BOOT_FAILED);
}

/* A block that holds a boundary of only one segment (or of the framebuffer) on one side is a mixed block. */
static void test_single_boundaries(void)
{
	Translation t;

	/* the end of the only segment is inside its block, the start is on the block edge */
	setup_x86();
	context.kernel.segment_count = 1;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	t = must_walk(SEG0);
	assert(t.executable && !t.writable && t.level == 0);
	expect_normal(SEG0 + PAGE);

	/* the start is inside its block, the end is on the block edge */
	setup_arm();
	context.kernel.segment_count = 1;
	set_segment(0, 0x41FF000ull, 0x1000, JP_ELF_PF_X, 0);
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	t = must_walk(0x41FF000ull);
	assert(t.executable && !t.writable && t.level == 0);
	expect_normal(0x41FE000ull);

	/* the framebuffer: start on the block edge, end inside; and start inside, end on the edge */
	setup_x86();
	context.framebuffer.base_address = FB_BASE + 0x1FC000;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	t = must_walk(FB_BASE + 0x1FC000);
	assert(t.special);
	expect_unmapped(FB_BASE + 0x1FB000);
}

/* A heap or runtime range that fills a whole block is checked on its own. */
static void test_validate_each_owner(void)
{
	int arch;

	for (arch = 0; arch < 2; arch++) {
		unsigned int owner_index;
		uint64_t no_exec_bit = arch == 0 ? (1ull << 63) : (1ull << 53);

		for (owner_index = 0; owner_index < 2; owner_index++) {
			JPKernelMemoryOwner owner = owner_index == 0 ? JP_KERNEL_MEMORY_EARLY_HEAP : JP_KERNEL_MEMORY_STAGE2_RUNTIME;
			uint64_t *entry;

			if (arch == 0) {
				setup_x86();
			} else {
				setup_arm();
			}
			assert(jp_kernel_memory_reserve(&context, 0x1000000, LARGE, owner) == JP_BOOT_SUCCESS);
			assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
			assert(jp_kernel_paging_validate(&context) == JP_BOOT_SUCCESS);
			entry = leaf_entry(0x1000000, &(unsigned int){0});
			*entry &= ~no_exec_bit;
			assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
		}
	}
}

static void test_validate_structure(void)
{
	uint64_t *entry;
	Translation t;

	/* x86_64: a 1 GiB page in the directory-pointer level */
	setup_x86();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	entry = entry_at_level(fake_root, MAIN_BASE + 0x500000, 2);
	assert(entry != NULL);
	*entry |= 0x80;
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

	/* AArch64: a 1 GiB block (type 0b01) in the level-1 table, and a block in the top level */
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	entry = entry_at_level(fake_root, MAIN_BASE + 0x500000, 2);
	assert(entry != NULL);
	*entry &= ~2ull;
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	entry = entry_at_level(fake_root, MAIN_BASE + 0x500000, 3);
	assert(entry != NULL);
	*entry &= ~2ull;
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
	/* a level-3 entry of type 0b01 is not a page */
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	entry = entry_at_level(fake_root, SEG1, 0);
	assert(entry != NULL);
	*entry &= ~2ull;
	assert(!walk(fake_root, SEG1, &t));
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);

	/* a kernel virtual address that is not canonical but has the same table indexes */
	setup_x86();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.segments[1].virtual_address ^= 1ull << 50;
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
	setup_arm();
	assert(jp_kernel_paging_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.segments[1].virtual_address ^= 1ull << 50;
	assert(jp_kernel_paging_validate(&context) == JP_BOOT_FAILED);
}

int main(void)
{
	test_x86_success();
	test_arm_success();
	test_deterministic();
	test_segment_permission_variants();
	test_refusals();
	test_register_readback();
	test_validate();
	test_unreachable_tables();
	test_input_limits();
	test_identity_limit();
	test_mixed_kinds_in_one_block();
	test_single_boundaries();
	test_validate_each_owner();
	test_validate_structure();
	return 0;
}
