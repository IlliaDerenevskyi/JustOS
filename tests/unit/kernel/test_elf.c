/*
 * Unit test (level U) of src/kernel/elf.c, for both architectures. The real
 * memory.c builds the ownership table; the physical-address width and the
 * paging check are fakes.
 */

#include <assert.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/elf.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"

#define PAGE 0x1000ull

static JPBootResult fake_paging_result;
static unsigned int paging_calls;

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	return 48;
}

JPBootResult jp_kernel_paging_validate(JPBootContext *context)
{
	(void)context;
	paging_calls++;
	return fake_paging_result;
}

#define MAIN_BASE 0x100000ull
#define MAIN_END 0x4000000ull
#define RES_BASE 0x4000000ull
#define RES_END 0x4400000ull
#define STACK_TOP 0x4100000ull
#define POOL_BASE 0x4200000ull
#define POOL_SIZE (128ull * PAGE)
#define SEG0 0x4000000ull
#define SEG1 0x4001000ull
#define SEG2 0x4002000ull

static _Alignas(4096) JPBootContext context;

static uint64_t virtual_base(void)
{
	return context.handoff.architecture == JP_ARCH_X86_64 ? 0xFFFFFFFF80000000ull : 0xFFFF000000000000ull;
}

static void add_region(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	JPMemoryRegion *r = &context.memory_map.regions[context.memory_map.region_count++];

	r->base_address = base;
	r->length = length;
	r->type = type;
}

static void set_segment(size_t i, uint64_t physical, uint64_t size, uint64_t file_size, uint32_t flags, uint64_t virtual_offset)
{
	JPKernelSegment *segment = &context.kernel.segments[i];

	segment->physical_address = physical;
	segment->virtual_address = virtual_base() + virtual_offset;
	segment->memory_size = size;
	segment->file_size = file_size;
	segment->flags = flags;
	segment->state = JP_KERNEL_SEGMENT_PERMISSIONED;
}

static void setup_for(JPArchitectureType architecture)
{
	uint64_t context_base;
	uint64_t context_end;

	memset(&context, 0, sizeof context);
	paging_calls = 0;
	fake_paging_result = JP_BOOT_SUCCESS;
	context.handoff.architecture = architecture;
	context.memory_map.state = JP_DETECTION_STATE_DETECTED;
	add_region(MAIN_BASE, MAIN_END - MAIN_BASE, JP_MEMORY_REGION_USABLE);
	add_region(RES_BASE, RES_END - RES_BASE, JP_MEMORY_REGION_RESERVED);
	context_base = (uint64_t)(uintptr_t)&context & ~(PAGE - 1);
	context_end = ((uint64_t)(uintptr_t)&context + sizeof context + PAGE - 1) & ~(PAGE - 1);
	add_region(context_base, context_end - context_base, JP_MEMORY_REGION_RESERVED);

	context.kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.kernel.header.magic = JP_ELF_MAGIC;
	context.kernel.header.elf_class = JP_ELF_CLASS_64;
	context.kernel.header.type = JP_ELF_TYPE_EXEC;
	context.kernel.header.machine = architecture == JP_ARCH_X86_64 ? JP_ELF_MACHINE_X86_64 : JP_ELF_MACHINE_AARCH64;
	context.kernel.header.program_header_entry_size = 56;
	context.kernel.header.program_header_count = 4;           /* a PT_NOTE or PT_GNU_STACK besides the three */
	context.kernel.segment_count = 3;
	set_segment(0, SEG0, 0x1000, 0x1000, JP_ELF_PF_X | JP_ELF_PF_R, 0);
	set_segment(1, SEG1, 0x1000, 0x1000, JP_ELF_PF_R, 0x1000);
	set_segment(2, SEG2, 0x1800, 0x400, JP_ELF_PF_W | JP_ELF_PF_R, 0x2000);
	context.kernel.entry_point = virtual_base() + 0x10;
	context.kernel.header.entry_point = virtual_base() + 0x10;
	context.handoff.kernel_entry = virtual_base() + 0x10;

	context.handoff.stack_pointer = STACK_TOP;
	context.handoff.page_table_memory_base = POOL_BASE;
	context.handoff.page_table_memory_size = POOL_SIZE;
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
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

#define EXPECT_FAILS(statement) \
	do { \
		int arch_; \
		for (arch_ = 0; arch_ < 2; arch_++) { \
			if (arch_ == 0) { setup_x86(); } else { setup_arm(); } \
			statement; \
			assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED); \
		} \
	} while (0)

/* ---- Success ---- */

static void test_success(void)
{
	static JPBootContext copy;

	setup_x86();
	memcpy(&copy, &context, sizeof copy);
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	assert(paging_calls == 1);
	assert(memcmp(&copy, &context, sizeof copy) == 0);            /* only reads */
	setup_arm();
	memcpy(&copy, &context, sizeof copy);
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
}

static void test_refusals(void)
{
	assert(jp_kernel_elf_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	setup_x86();
	context.stage2.entry_count = 0;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(paging_calls == 0);
	EXPECT_FAILS(context.handoff.architecture = JP_ARCH_UNKNOWN);
	EXPECT_FAILS(context.kernel.validation_state = JP_KERNEL_VALIDATION_UNKNOWN);
	EXPECT_FAILS(context.kernel.validation_state = JP_KERNEL_VALIDATION_INVALID);
	EXPECT_FAILS(context.kernel.load_state = JP_KERNEL_LOAD_NOT_INITIALIZED);
	EXPECT_FAILS(context.kernel.load_state = JP_KERNEL_LOAD_FAILED);
}

/* ---- Header ---- */

static void test_header(void)
{
	EXPECT_FAILS(context.kernel.header.magic = 0x464C4570);
	EXPECT_FAILS(context.kernel.header.magic = 0);
	EXPECT_FAILS(context.kernel.header.elf_class = 1);
	EXPECT_FAILS(context.kernel.header.elf_class = 0);
	EXPECT_FAILS(context.kernel.header.type = JP_ELF_TYPE_DYN);
	EXPECT_FAILS(context.kernel.header.type = 1);
	EXPECT_FAILS(context.kernel.header.type = 0);
	EXPECT_FAILS(context.kernel.header.machine = 3);
	EXPECT_FAILS(context.kernel.header.machine = 0);
	EXPECT_FAILS(context.kernel.header.program_header_entry_size = 32);
	EXPECT_FAILS(context.kernel.header.program_header_entry_size = 57);
	EXPECT_FAILS(context.kernel.header.program_header_count = 2);     /* fewer than the segments */
	EXPECT_FAILS(context.kernel.header.program_header_count = 0);

	/* the machine must be the one of the context, not just a supported one */
	setup_x86();
	context.kernel.header.machine = JP_ELF_MACHINE_AARCH64;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	setup_arm();
	context.kernel.header.machine = JP_ELF_MACHINE_X86_64;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);

	/* the count of program headers may be larger than the count of segments, and equal */
	setup_x86();
	context.kernel.header.program_header_count = 3;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.header.program_header_count = 65535;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
}

/* ---- Segments ---- */

static void test_segment_counts(void)
{
	EXPECT_FAILS(context.kernel.segment_count = 0);
	EXPECT_FAILS(context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS + 1);
}

static void test_segment_state(void)
{
	JPKernelSegmentState state;

	for (state = JP_KERNEL_SEGMENT_UNINITIALIZED; state <= JP_KERNEL_SEGMENT_FAILED; state++) {
		size_t i;

		for (i = 0; i < 3; i++) {
			setup_x86();
			context.kernel.segments[i].state = state;
			assert(jp_kernel_elf_initialize(&context) ==
			       (state == JP_KERNEL_SEGMENT_PERMISSIONED ? JP_BOOT_SUCCESS : JP_BOOT_FAILED));
		}
	}
}

static void test_segment_sizes(void)
{
	EXPECT_FAILS(context.kernel.segments[1].memory_size = 0);
	EXPECT_FAILS(context.kernel.segments[2].file_size = context.kernel.segments[2].memory_size + 1);
	/* file size equal to the memory size, and zero, are fine */
	setup_x86();
	context.kernel.segments[2].file_size = context.kernel.segments[2].memory_size;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.segments[2].file_size = 0;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_segment_ranges(void)
{
	/* an overflowing virtual range */
	EXPECT_FAILS(context.kernel.segments[1].virtual_address = UINT64_MAX - 0x100);
	/* an overflowing physical range (memory.c would refuse it; the table is already built) */
	EXPECT_FAILS(context.kernel.segments[1].physical_address = UINT64_MAX - 0x100);

	/* canonical addresses */
	setup_x86();
	context.kernel.segments[1].virtual_address = 0x0000800000000000ull;         /* the first non-canonical address */
	context.kernel.entry_point = virtual_base() + 0x10;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	setup_x86();
	context.kernel.segments[1].virtual_address = 0x00007FFFFFFFF000ull;         /* the last canonical page of the lower half */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	setup_x86();
	context.kernel.segments[1].virtual_address = 0xFFFF800000000000ull;         /* the first canonical address of the upper half */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	setup_x86();
	context.kernel.segments[1].virtual_address = 0xFFFF7FFFFFFFF000ull;         /* just below it: not canonical */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	/* the end of the range crosses from canonical to non-canonical */
	setup_x86();
	context.kernel.segments[1].virtual_address = 0x00007FFFFFFFF800ull;
	context.kernel.segments[1].memory_size = 0x1000;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);

	setup_arm();
	context.kernel.segments[1].virtual_address = 0x0001000000000000ull;         /* above 48 bits */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	setup_arm();
	context.kernel.segments[1].virtual_address = 0x0000FFFFFFFFF000ull;         /* the last page of the lower half */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	setup_arm();
	context.kernel.segments[1].virtual_address = 0xFFFF000000000000ull + 0x1000;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	setup_arm();
	context.kernel.segments[1].virtual_address = 0xFFFE000000001000ull;         /* just below the upper half */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	setup_arm();
	context.kernel.segments[1].virtual_address = 0x0000FFFFFFFFF800ull;         /* the end crosses 48 bits */
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	/* x86_64 addresses between 2^47 and 2^48 are valid on AArch64 (lower half is 48 bits) but not on x86_64 */
	setup_arm();
	context.kernel.segments[1].virtual_address = 0x0000800000001000ull;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_segment_ownership(void)
{
	JPBootStage2MemoryEntry *entry = NULL;
	size_t i;
	int arch;

	/* a segment whose physical pages are not owned by the kernel image */
	EXPECT_FAILS(context.kernel.segments[1].physical_address = MAIN_BASE);
	EXPECT_FAILS(context.kernel.segments[1].physical_address = RES_BASE + 0x100000);   /* reserved, but not image */
	/* a segment that is longer than its owned pages */
	EXPECT_FAILS(context.kernel.segments[2].memory_size = 0x4000);
	/* the owned range is split between two owners */
	for (arch = 0; arch < 2; arch++) {
		if (arch == 0) {
			setup_x86();
		} else {
			setup_arm();
		}
		for (i = 0; i < context.stage2.entry_count; i++) {
			if (context.stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_KERNEL_IMAGE) {
				entry = &context.stage2.entries[i];
			}
		}
		assert(entry != NULL);
		entry->owner = (uint32_t)JP_KERNEL_MEMORY_STAGE1_RESERVED;
		assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	}
	/* a segment that is not page aligned is checked by its pages: it still passes when they are owned */
	setup_x86();
	context.kernel.segments[1].physical_address = SEG1 + 0x100;
	context.kernel.segments[1].memory_size = 0x200;
	context.kernel.segments[1].file_size = 0x200;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_segments_overlap(void)
{
	/* the virtual ranges overlap by one byte */
	EXPECT_FAILS(context.kernel.segments[1].virtual_address = context.kernel.segments[0].virtual_address + 0xFFF);
	/* one inside another */
	EXPECT_FAILS(context.kernel.segments[2].virtual_address = context.kernel.segments[0].virtual_address + 0x10);
	/* touching is fine */
	setup_x86();
	context.kernel.segments[1].virtual_address = context.kernel.segments[0].virtual_address + 0x1000;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	/* the same pair in the other order of the array */
	setup_x86();
	context.kernel.segments[0].virtual_address = context.kernel.segments[1].virtual_address + 0xFFF;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
}

static void test_segment_array_order(void)
{
	/* a segment that comes earlier in the array may lie after another one in memory; touching is fine */
	setup_x86();
	context.kernel.segments[0].virtual_address = context.kernel.segments[2].virtual_address + 0x1800;   /* right after segment 2 */
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry =
		context.kernel.segments[0].virtual_address + 0x10;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
}

static void test_maximum_segments(void)
{
	size_t i;
	int arch;

	for (arch = 0; arch < 2; arch++) {
		if (arch == 0) {
			setup_x86();
		} else {
			setup_arm();
		}
		context.kernel.segment_count = JP_MAX_KERNEL_SEGMENTS;
		context.kernel.header.program_header_count = JP_MAX_KERNEL_SEGMENTS;
		for (i = 0; i < JP_MAX_KERNEL_SEGMENTS; i++) {
			set_segment(i, SEG0 + i * PAGE, 0x1000, 0x1000, i == 0 ? (JP_ELF_PF_X | JP_ELF_PF_R) : JP_ELF_PF_R, i * PAGE);
		}
		assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
		assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	}
}

/* ---- Entry ---- */

static void test_entry(void)
{
	/* the three numbers must be the same */
	EXPECT_FAILS(context.kernel.entry_point += 16);
	EXPECT_FAILS(context.kernel.header.entry_point += 16);
	EXPECT_FAILS(context.handoff.kernel_entry += 16);
	/* an entry in a segment that is not executable */
	EXPECT_FAILS(context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry =
		virtual_base() + 0x1010);
	EXPECT_FAILS(context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry =
		virtual_base() + 0x2010);
	/* an entry outside every segment */
	EXPECT_FAILS(context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry =
		virtual_base() + 0x5000);
	EXPECT_FAILS(context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = 0);
	/* the last and the first byte of the executable segment are inside; the byte after is not */
	setup_x86();
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = virtual_base();
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = virtual_base() + 0xFFF;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = virtual_base() + 0x1000;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	/* a non-canonical entry */
	setup_x86();
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = 0x0000800000000010ull;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	/* any executable segment will do, not just the first */
	setup_x86();
	context.kernel.segments[0].flags = JP_ELF_PF_R;
	context.kernel.segments[2].flags = JP_ELF_PF_X | JP_ELF_PF_R;
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = virtual_base() + 0x2010;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_SUCCESS);
	context.kernel.entry_point = context.kernel.header.entry_point = context.handoff.kernel_entry = virtual_base() + 0x10;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
}

/* ---- Paging ---- */

static void test_paging_check(void)
{
	setup_x86();
	fake_paging_result = JP_BOOT_FAILED;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	assert(paging_calls == 1);
	setup_arm();
	fake_paging_result = JP_BOOT_NOT_INITIALIZED;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	/* the paging check comes last: a broken header never reaches it */
	setup_x86();
	context.kernel.header.magic = 0;
	assert(jp_kernel_elf_initialize(&context) == JP_BOOT_FAILED);
	assert(paging_calls == 0);
}

int main(void)
{
	test_success();
	test_refusals();
	test_header();
	test_segment_counts();
	test_segment_state();
	test_segment_sizes();
	test_segment_ranges();
	test_segment_ownership();
	test_segments_overlap();
	test_segment_array_order();
	test_maximum_segments();
	test_entry();
	test_paging_check();
	return 0;
}
