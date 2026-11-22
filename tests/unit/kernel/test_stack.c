/*
 * Unit test (level U) of src/kernel/stack.c, for both architectures. The real
 * memory.c builds the ownership table; the stack pointer, the physical-address
 * width, and the paging check are replaced by fakes.
 */

#include <assert.h>
#include <string.h>

#include "cpu_regs.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"
#include "justos/kernel/stack.h"
#include "stack_hw.h"

#define PAGE 0x1000ull

/* ---- The fakes ---- */

static uint64_t fake_stack_pointer;
static JPBootResult fake_paging_result;
static unsigned int paging_calls;

uint64_t jp_kernel_stack_current_pointer(void)
{
	return fake_stack_pointer;
}

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

/* ---- The layout ---- */

#define MAIN_BASE 0x100000ull
#define MAIN_END 0x4000000ull
#define RES_BASE 0x4000000ull
#define RES_END 0x4400000ull
#define STACK_TOP 0x4100000ull
#define STACK_BASE (STACK_TOP - JP_BOOT_KERNEL_STACK_SIZE)
#define IMAGE_SIZE 0x3000ull
#define POOL_BASE 0x4200000ull
#define POOL_SIZE (128ull * PAGE)

static _Alignas(4096) JPBootContext context;

static void add_region(uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	JPMemoryRegion *r = &context.memory_map.regions[context.memory_map.region_count++];

	r->base_address = base;
	r->length = length;
	r->type = type;
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
	add_region(RES_END, 0x1000000, JP_MEMORY_REGION_USABLE);
	context_base = (uint64_t)(uintptr_t)&context & ~(PAGE - 1);
	context_end = ((uint64_t)(uintptr_t)&context + sizeof context + PAGE - 1) & ~(PAGE - 1);
	add_region(context_base, context_end - context_base, JP_MEMORY_REGION_RESERVED);
	context.framebuffer.state = JP_DETECTION_STATE_NOT_DETECTED;
	context.kernel.load_state = JP_KERNEL_LOAD_LOADED;
	context.kernel.segment_count = 1;
	context.kernel.segments[0].physical_address = RES_BASE;
	context.kernel.segments[0].memory_size = IMAGE_SIZE;
	context.handoff.stack_pointer = STACK_TOP;
	context.handoff.page_table_memory_base = POOL_BASE;
	context.handoff.page_table_memory_size = POOL_SIZE;
	assert(jp_kernel_memory_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP - 0x100;
}

static void setup_x86(void)
{
	setup_for(JP_ARCH_X86_64);
}

static void setup_arm(void)
{
	setup_for(JP_ARCH_AARCH64);
}

/* The table entry of the stack. */
static JPBootStage2MemoryEntry *stack_entry(void)
{
	size_t i;

	for (i = 0; i < context.stage2.entry_count; i++) {
		if (context.stage2.entries[i].owner == (uint32_t)JP_KERNEL_MEMORY_KERNEL_STACK) {
			return &context.stage2.entries[i];
		}
	}
	assert(0);
	return NULL;
}

/* ---- Tests ---- */

static void test_success(void)
{
	setup_x86();
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	assert(paging_calls == 1);
	setup_arm();
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	assert(paging_calls == 1);
}

static void test_stack_pointer_range(void)
{
	/* both ends of the range are inside */
	setup_x86();
	fake_stack_pointer = STACK_BASE;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	setup_arm();
	fake_stack_pointer = STACK_BASE;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);

	/* one step outside, on both sides */
	setup_x86();
	fake_stack_pointer = STACK_BASE - 8;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	fake_stack_pointer = STACK_TOP + 8;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	fake_stack_pointer = 0;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	setup_arm();
	fake_stack_pointer = STACK_BASE - 16;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	fake_stack_pointer = STACK_TOP + 16;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
}

static void test_alignment(void)
{
	/* x86_64: a multiple of 8 is enough (a call pushed 8 bytes), 4 is not */
	setup_x86();
	fake_stack_pointer = STACK_TOP - 0x108;      /* 8 modulo 16 */
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP - 0x100;      /* 0 modulo 16 */
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP - 0x104;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	fake_stack_pointer = STACK_TOP - 0x101;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);

	/* AArch64: a multiple of 16 */
	setup_arm();
	fake_stack_pointer = STACK_TOP - 0x100;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	fake_stack_pointer = STACK_TOP - 0x108;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	fake_stack_pointer = STACK_TOP - 0x104;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
}

static void test_top_alignment(void)
{
	/* The top is a multiple of 8 but not of 16: the table is patched to keep the stack range exact. */
	int arch;

	for (arch = 0; arch < 2; arch++) {
		JPBootStage2MemoryEntry *entry;

		if (arch == 0) {
			setup_x86();
		} else {
			setup_arm();
		}
		entry = stack_entry();
		entry->base_address += 8;
		context.handoff.stack_pointer += 8;
		fake_stack_pointer = context.handoff.stack_pointer - 0x100;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	}
}

static void test_ownership_of_the_stack(void)
{
	JPBootStage2MemoryEntry *entry;
	int arch;

	for (arch = 0; arch < 2; arch++) {
#define FRESH() do { if (arch == 0) setup_x86(); else setup_arm(); } while (0)

		/* the stack is not owned as a stack */
		FRESH();
		stack_entry()->owner = (uint32_t)JP_KERNEL_MEMORY_STAGE1_RESERVED;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);

		/* the owned range is longer than the stack: the byte after the top is owned too */
		FRESH();
		entry = stack_entry();
		entry->length += 8;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);

		/* the owned range starts before the stack: the byte before the base is owned too */
		FRESH();
		entry = stack_entry();
		(entry - 1)->length -= 8;               /* the entry before it gives the 8 bytes up */
		entry->base_address -= 8;
		entry->length += 8;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);

		/* the owned range is shorter than the stack */
		FRESH();
		entry = stack_entry();
		entry->length -= PAGE;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);

		/* a stack that has another owner in the middle: the entry is split, the second half has another owner */
		FRESH();
		{
			size_t at;
			size_t i;

			entry = stack_entry();
			at = (size_t)(entry - context.stage2.entries);
			for (i = context.stage2.entry_count; i > at + 1; i--) {
				context.stage2.entries[i] = context.stage2.entries[i - 1];
			}
			context.stage2.entries[at + 1].base_address = context.stage2.entries[at].base_address + JP_BOOT_KERNEL_STACK_SIZE / 2;
			context.stage2.entries[at + 1].length = JP_BOOT_KERNEL_STACK_SIZE / 2;
			context.stage2.entries[at + 1].owner = (uint32_t)JP_KERNEL_MEMORY_PAGE_TABLES;
			context.stage2.entries[at].length = JP_BOOT_KERNEL_STACK_SIZE / 2;
			context.stage2.entry_count++;
			assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
		}

		/* a top that is smaller than the size of the stack */
		FRESH();
		context.handoff.stack_pointer = JP_BOOT_KERNEL_STACK_SIZE - PAGE;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
#undef FRESH
	}
}

static void test_stack_at_address_zero(void)
{
	/* The smallest possible top: the stack is [0, size). The table is patched to say so. */
	int arch;

	for (arch = 0; arch < 2; arch++) {
		JPBootStage2MemoryEntry *entry;

		if (arch == 0) {
			setup_x86();
		} else {
			setup_arm();
		}
		{
			size_t i;

			stack_entry()->owner = (uint32_t)JP_KERNEL_MEMORY_STAGE1_RESERVED;
			for (i = context.stage2.entry_count; i > 0; i--) {
				context.stage2.entries[i] = context.stage2.entries[i - 1];
			}
			entry = &context.stage2.entries[0];       /* the table is sorted: the new entry goes first */
			entry->base_address = 0;
			entry->length = JP_BOOT_KERNEL_STACK_SIZE;
			entry->owner = (uint32_t)JP_KERNEL_MEMORY_KERNEL_STACK;
			context.stage2.entry_count++;
		}
		context.handoff.stack_pointer = JP_BOOT_KERNEL_STACK_SIZE;
		fake_stack_pointer = JP_BOOT_KERNEL_STACK_SIZE - 0x100;
		assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	}
}

static void test_paging_check(void)
{
	setup_x86();
	fake_paging_result = JP_BOOT_FAILED;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	assert(paging_calls == 1);
	setup_arm();
	fake_paging_result = JP_BOOT_NOT_INITIALIZED;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	setup_arm();
	fake_paging_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
}

static void test_refusals(void)
{
	assert(jp_kernel_stack_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	setup_x86();
	context.stage2.entry_count = 0;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(paging_calls == 0);

	setup_x86();
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_FAILED);
	assert(paging_calls == 0);
}

static void test_nothing_is_changed(void)
{
	static JPBootContext copy;

	setup_x86();
	memcpy(&copy, &context, sizeof copy);
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
	setup_arm();
	memcpy(&copy, &context, sizeof copy);
	assert(jp_kernel_stack_initialize(&context) == JP_BOOT_SUCCESS);
	assert(memcmp(&copy, &context, sizeof copy) == 0);
}

int main(void)
{
	test_success();
	test_stack_pointer_range();
	test_alignment();
	test_top_alignment();
	test_ownership_of_the_stack();
	test_stack_at_address_zero();
	test_paging_check();
	test_refusals();
	test_nothing_is_changed();
	return 0;
}
