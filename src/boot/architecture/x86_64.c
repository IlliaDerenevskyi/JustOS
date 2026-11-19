/*
 * x86_64.c - the x86_64 architecture operations of Stage 1.
 *
 * Purpose:
 *   Machine-dependent work for x86_64: the boot page tables and the
 *   descriptor table, the kernel entry state, and the jump into the kernel.
 *
 * Responsibilities:
 *   - the table memory (one block, see "Decided rules (x86_64 ...)" in
 *     milestone0.md): created by the first mapping, reserved with the firmware;
 *   - kernel segment mapping, the identity map, and the framebuffer mapping;
 *   - the descriptor table with the code and data segments;
 *   - CPU discovery (CPUID), entry-state checks, and enter_kernel.
 *
 * Limitations:
 *   The instructions that need assembly (cli, reading control registers, the
 *   jump) are in x86_64_enter.S behind x86_64_cpu.h. No firmware logic. The
 *   code is freestanding: no libc.
 *
 * Depends on:
 *   justos/boot/architecture/x86_64.h, justos/boot/boot_environment.h,
 *   x86_64_cpu.h.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/architecture/x86_64.h"

#include "justos/boot/boot_environment.h"
#include "x86_64_cpu.h"

/* ---- Public API ---- */

JPBootResult jp_x86_64_validate_entry_state(const JPBootContext *context);
JPBootResult jp_x86_64_setup_paging(JPBootContext *context);
JPBootResult jp_x86_64_prepare_entry(JPBootContext *context);
JPBootResult jp_x86_64_validate_elf_machine(const JPKernelHeader *header);
JPBootResult jp_x86_64_discover_cpu(JPCpuInfo *cpu);
JPBootResult jp_x86_64_map_segment(JPBootContext *context, size_t index);
void *jp_x86_64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length);
_Noreturn void jp_x86_64_enter_kernel(JPBootContext *context);

/* ---- Private constants ---- */

enum {
	JP_ENTRIES_PER_TABLE = 512,
	JP_INDEX_BITS = 9,
	JP_PAGE_SHIFT = 12,
	JP_LEVEL_TOP = 3,                    /* PML4 */
	JP_LEVEL_PD = 1,
	JP_GDT_ENTRIES = 3,
	JP_GDT_PAGE = 1,                     /* page 1 of the pool */
	JP_PML4_PAGE = 0,                    /* page 0 of the pool */
	JP_CANONICAL_SHIFT = 47,
	JP_CPUID_MAX_LEAF_MIN = 1,
	JP_CPUID_COUNT_SHIFT = 16,
	JP_CPUID_COUNT_MASK = 0xFF,
	JP_CPUID_HTT_BIT = 28,
	JP_PF_X = 1,
	JP_PF_W = 2
};

#define JP_PTE_PRESENT 0x1ULL
#define JP_PTE_WRITABLE 0x2ULL
#define JP_PTE_CACHE_DISABLE 0x10ULL
#define JP_PTE_LARGE 0x80ULL
#define JP_PTE_NO_EXECUTE (1ULL << 63)
#define JP_PTE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define JP_LARGE_PAGE_SIZE 0x200000ULL
#define JP_GDT_CODE64 0x00AF9A000000FFFFULL
#define JP_GDT_DATA 0x00CF92000000FFFFULL
#define JP_POOL_BYTES ((uint64_t)JP_X86_64_TABLE_POOL_PAGES * JP_BOOT_PAGE_SIZE)

/* ---- Private helpers: physical memory ---- */

#ifdef JP_X86_64_TESTING
/* Host tests turn a physical address into a pointer into a host buffer. */
void *(*jp_x86_64_test_phys_to_pointer)(uint64_t physical);
#endif

/* In Milestone 0 all memory that Stage 1 touches is reachable at its physical address. */
static void *physical_pointer(uint64_t physical)
{
#ifdef JP_X86_64_TESTING
	if (jp_x86_64_test_phys_to_pointer != NULL) {
		return jp_x86_64_test_phys_to_pointer(physical);
	}
#endif
	return (void *)(uintptr_t)physical;
}

static uint64_t align_down(uint64_t value, uint64_t alignment)
{
	return value & ~(alignment - 1);
}

static bool region_contains(const JPMemoryRegion *region, uint64_t physical, uint64_t length)
{
	return region->length <= UINT64_MAX - region->base_address &&
	       physical >= region->base_address &&
	       physical - region->base_address <= region->length &&
	       length <= region->length - (physical - region->base_address);
}

static bool is_canonical(uint64_t address)
{
	uint64_t top = address >> JP_CANONICAL_SHIFT;

	return top == 0 || top == (UINT64_MAX >> JP_CANONICAL_SHIFT);
}

/* ---- Private helpers: the table memory (the pool) ---- */

static uint64_t *pool_page(const JPBootContext *context, uint64_t page)
{
	return (uint64_t *)physical_pointer(context->handoff.page_table_memory_base + page * JP_BOOT_PAGE_SIZE);
}

/*
 * The highest candidate for the pool whose end is at most `limit`: in each
 * usable region the candidate is the top part (page aligned, above the
 * protected low memory); the highest candidate of all regions wins.
 */
static bool find_pool_address(const JPBootContext *context, uint64_t limit, uint64_t *address)
{
	uint64_t best = 0;
	bool found = false;
	size_t i;

	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		uint64_t end;
		uint64_t candidate;

		if (region->type != JP_MEMORY_REGION_USABLE ||
		    region->length > UINT64_MAX - region->base_address) {
			continue;
		}
		end = region->base_address + region->length;
		if (end > limit) {
			end = limit;
		}
		if (end < JP_POOL_BYTES) {
			continue;
		}
		candidate = align_down(end - JP_POOL_BYTES, JP_BOOT_PAGE_SIZE);
		if (candidate < region->base_address || candidate < JP_BOOT_PROTECTED_LOW_MEMORY) {
			continue;
		}
		if (!found || candidate > best) {
			best = candidate;
			found = true;
		}
	}
	*address = best;
	return found;
}

/*
 * Creates the pool at the first need: placed, reserved, zeroed, and described
 * in the handoff. The memory map is the early one and the firmware keeps
 * allocating (from the top downward), so a reservation can be refused: the
 * next lower candidate is then tried, at most JP_X86_64_POOL_ATTEMPTS times.
 */
static JPBootResult ensure_pool(JPBootContext *context)
{
	uint64_t address = 0;
	uint64_t limit = UINT64_MAX;
	uint64_t *memory;
	uint64_t i;
	int attempt;
	bool reserved = false;

	if (context->handoff.page_table_memory_base != 0) {
		return JP_BOOT_SUCCESS;
	}
	for (attempt = 0; attempt < JP_X86_64_POOL_ATTEMPTS; attempt++) {
		if (!find_pool_address(context, limit, &address)) {
			return JP_BOOT_FAILED;
		}
		if (jp_boot_environment_reserve_memory(context, address, JP_POOL_BYTES) == JP_BOOT_SUCCESS) {
			reserved = true;
			break;
		}
		limit = address;          /* the next candidate ends at or below the refused one */
	}
	if (!reserved) {
		return JP_BOOT_FAILED;
	}
	memory = (uint64_t *)physical_pointer(address);
	if (memory == NULL) {
		return JP_BOOT_FAILED;
	}
	for (i = 0; i < JP_POOL_BYTES / sizeof(uint64_t); i++) {
		memory[i] = 0;
	}
	context->handoff.page_table_memory_base = address;
	context->handoff.page_table_memory_size = JP_POOL_BYTES;
	/* Page 0 is the top-level table and page 1 the descriptor table. */
	context->handoff.page_table_memory_used = JP_GDT_PAGE + 1;
	context->handoff.page_table_root = address + JP_PML4_PAGE * JP_BOOT_PAGE_SIZE;
	return JP_BOOT_SUCCESS;
}

/* Hands out the next zeroed page of the pool; returns its physical address, or 0 if the pool is full. */
static uint64_t allocate_table(JPBootContext *context)
{
	uint64_t page = context->handoff.page_table_memory_used;

	if (page >= JP_X86_64_TABLE_POOL_PAGES) {
		return 0;
	}
	context->handoff.page_table_memory_used = page + 1;
	return context->handoff.page_table_memory_base + page * JP_BOOT_PAGE_SIZE;
}

static unsigned int table_index(uint64_t virtual_address, int level)
{
	return (unsigned int)((virtual_address >> (JP_PAGE_SHIFT + JP_INDEX_BITS * level)) &
		(JP_ENTRIES_PER_TABLE - 1));
}

/*
 * Walks down from the top-level table to the table of the given level (3 is
 * the top, 1 is the page directory, 0 is the page table), creating missing
 * tables. Returns the table or NULL (pool full, a large page in the way).
 */
static uint64_t *table_for(JPBootContext *context, uint64_t virtual_address, int stop_level)
{
	uint64_t *table = (uint64_t *)physical_pointer(context->handoff.page_table_root);
	int level;

	for (level = JP_LEVEL_TOP; level > stop_level; level--) {
		uint64_t *entry;

		if (table == NULL) {
			return NULL;
		}
		entry = &table[table_index(virtual_address, level)];
		if ((*entry & JP_PTE_PRESENT) == 0) {
			uint64_t next = allocate_table(context);

			if (next == 0) {
				return NULL;
			}
			*entry = next | JP_PTE_PRESENT | JP_PTE_WRITABLE;
		} else if ((*entry & JP_PTE_LARGE) != 0 && level == JP_LEVEL_PD) {
			return NULL;
		}
		table = (uint64_t *)physical_pointer(*entry & JP_PTE_ADDRESS_MASK);
	}
	return table;
}

/* One 4 KiB page. A page that is already mapped is a conflict. */
static bool map_small_page(JPBootContext *context, uint64_t virtual_address, uint64_t physical, uint32_t flags)
{
	uint64_t *table = table_for(context, virtual_address, 0);
	uint64_t *entry;
	uint64_t value;

	if (table == NULL) {
		return false;
	}
	entry = &table[table_index(virtual_address, 0)];
	if ((*entry & JP_PTE_PRESENT) != 0) {
		return false;
	}
	value = physical | JP_PTE_PRESENT;
	if ((flags & JP_PF_W) != 0) {
		value |= JP_PTE_WRITABLE;
	}
	if ((flags & JP_PF_X) == 0) {
		value |= JP_PTE_NO_EXECUTE;
	}
	*entry = value;
	return true;
}

/*
 * One 2 MiB identity page. An entry that already is the same identity page
 * with the same attributes is accepted (two regions share a page). Another
 * identity page is replaced only when replace_identity is set (the framebuffer
 * changes the attributes). Any other entry in its place, for example the tables
 * of a kernel segment, is a conflict.
 */
static bool map_identity_large_page(JPBootContext *context, uint64_t address, uint64_t flags_bits, bool replace_identity)
{
	uint64_t *table = table_for(context, address, JP_LEVEL_PD);
	uint64_t *entry;
	uint64_t wanted = address | JP_PTE_PRESENT | JP_PTE_LARGE | flags_bits;

	if (table == NULL) {
		return false;
	}
	entry = &table[table_index(address, JP_LEVEL_PD)];
	if ((*entry & JP_PTE_PRESENT) != 0) {
		bool same_identity = (*entry & JP_PTE_LARGE) != 0 &&
			(*entry & JP_PTE_ADDRESS_MASK) == address;

		if (same_identity && *entry == wanted) {
			return true;
		}
		if (!(replace_identity && same_identity)) {
			return false;
		}
	}
	*entry = wanted;
	return true;
}

static void fill_descriptor_table(const JPBootContext *context)
{
	uint64_t *gdt = pool_page(context, JP_GDT_PAGE);

	gdt[0] = 0;                 /* the null entry */
	gdt[1] = JP_GDT_CODE64;     /* selector 0x08: 64-bit code */
	gdt[2] = JP_GDT_DATA;       /* selector 0x10: data */
}

/* ---- Private helpers: entry state ---- */

static bool entry_in_executable_segment(const JPKernelInfo *kernel, uint64_t entry)
{
	size_t i;

	for (i = 0; i < kernel->segment_count; i++) {
		const JPKernelSegment *segment = &kernel->segments[i];

		if (segment->state == JP_KERNEL_SEGMENT_PERMISSIONED &&
		    (segment->flags & JP_PF_X) != 0 &&
		    entry >= segment->virtual_address &&
		    entry - segment->virtual_address < segment->memory_size) {
			return true;
		}
	}
	return false;
}

/* ---- Implementation ---- */

JPBootResult jp_x86_64_validate_elf_machine(const JPKernelHeader *header)
{
	if (header == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	return header->machine == JP_ELF_MACHINE_X86_64 ? JP_BOOT_SUCCESS : JP_BOOT_ERROR_CORRUPTED_DATA;
}

JPBootResult jp_x86_64_discover_cpu(JPCpuInfo *cpu)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	if (cpu == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	__asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0), "c"(0));
	if (eax < JP_CPUID_MAX_LEAF_MIN) {
		return JP_BOOT_FAILED;
	}
	__asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
	cpu->logical_cores = 1;
	if ((edx & (1U << JP_CPUID_HTT_BIT)) != 0) {
		uint32_t count = (ebx >> JP_CPUID_COUNT_SHIFT) & JP_CPUID_COUNT_MASK;

		if (count != 0) {
			cpu->logical_cores = count;
		}
	}
	cpu->state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

void *jp_x86_64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length)
{
	size_t i;

	if (context == NULL || length == 0 || physical == 0 || length > UINT64_MAX - physical) {
		return NULL;
	}
	for (i = 0; i < context->memory_map.region_count; i++) {
		if (region_contains(&context->memory_map.regions[i], physical, length)) {
			return physical_pointer(physical);
		}
	}
	return NULL;
}

JPBootResult jp_x86_64_map_segment(JPBootContext *context, size_t index)
{
	const JPKernelSegment *segment;
	uint64_t pages;
	uint64_t i;

	if (context == NULL || index >= context->kernel.segment_count || index >= JP_MAX_KERNEL_SEGMENTS) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	segment = &context->kernel.segments[index];
	if (segment->state != JP_KERNEL_SEGMENT_LOADED) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (ensure_pool(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	if (segment->memory_size > UINT64_MAX - (JP_BOOT_PAGE_SIZE - 1)) {
		return JP_BOOT_FAILED;
	}
	pages = (segment->memory_size + JP_BOOT_PAGE_SIZE - 1) / JP_BOOT_PAGE_SIZE;
	for (i = 0; i < pages; i++) {
		uint64_t virtual_address = segment->virtual_address + i * JP_BOOT_PAGE_SIZE;
		uint64_t physical = segment->physical_address + i * JP_BOOT_PAGE_SIZE;

		if (!is_canonical(virtual_address) ||
		    !map_small_page(context, virtual_address, physical, segment->flags)) {
			return JP_BOOT_FAILED;
		}
	}
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_x86_64_setup_paging(JPBootContext *context)
{
	uint64_t address;
	bool mapped_any = false;
	size_t i;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->handoff.paging_enabled) {
		return JP_BOOT_FAILED;
	}
	if (ensure_pool(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}

	/* The identity map follows the regions (see "Decided rules (x86_64 ...)"). */
	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		uint64_t first;
		uint64_t last;

		if (region->type == JP_MEMORY_REGION_MMIO || region->type == JP_MEMORY_REGION_UNUSABLE ||
		    region->length == 0 ||
		    region->length > UINT64_MAX - region->base_address - (JP_LARGE_PAGE_SIZE - 1)) {
			continue;
		}
		if (region->type == JP_MEMORY_REGION_RESERVED && region->length >= JP_X86_64_IDENTITY_RESERVED_MAX) {
			continue;    /* an address range kept by the firmware, not memory in use */
		}
		first = align_down(region->base_address, JP_LARGE_PAGE_SIZE);
		last = align_down(region->base_address + region->length + JP_LARGE_PAGE_SIZE - 1, JP_LARGE_PAGE_SIZE);
		for (address = first; address < last; address += JP_LARGE_PAGE_SIZE) {
			if (!map_identity_large_page(context, address, JP_PTE_WRITABLE, false)) {
				return JP_BOOT_FAILED;
			}
		}
		mapped_any = true;
	}
	if (!mapped_any) {
		return JP_BOOT_FAILED;
	}

	/* The framebuffer: writable, no execute, cache disabled. */
	if (context->framebuffer.state == JP_DETECTION_STATE_DETECTED && context->framebuffer.base_address != 0) {
		uint64_t size = (uint64_t)context->framebuffer.pitch * context->framebuffer.height;
		uint64_t base = context->framebuffer.base_address;
		uint64_t first;
		uint64_t last;

		if (size == 0 || size > UINT64_MAX - base - (JP_LARGE_PAGE_SIZE - 1)) {
			return JP_BOOT_FAILED;
		}
		first = align_down(base, JP_LARGE_PAGE_SIZE);
		last = align_down(base + size + JP_LARGE_PAGE_SIZE - 1, JP_LARGE_PAGE_SIZE);
		for (address = first; address < last; address += JP_LARGE_PAGE_SIZE) {
			if (!map_identity_large_page(context, address,
				JP_PTE_WRITABLE | JP_PTE_NO_EXECUTE | JP_PTE_CACHE_DISABLE, true)) {
				return JP_BOOT_FAILED;
			}
		}
	}

	fill_descriptor_table(context);
	context->handoff.paging_enabled = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_x86_64_prepare_entry(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->handoff.page_table_root == 0 || !context->handoff.paging_enabled ||
	    context->handoff.stack_pointer == 0 ||
	    context->handoff.stack_pointer % JP_X86_64_STACK_ALIGNMENT != 0 ||
	    context->kernel.entry_point == 0 || !is_canonical(context->kernel.entry_point)) {
		return JP_BOOT_FAILED;
	}
	/* The interrupts are really disabled first; only then is the fact recorded. */
	jp_x86_64_disable_interrupts();
	context->handoff.kernel_entry = context->kernel.entry_point;
	context->handoff.architecture = JP_ARCH_X86_64;
	context->handoff.interrupts_disabled = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_x86_64_validate_entry_state(const JPBootContext *context)
{
	JPX86CpuState state;
	const JPHandoffInfo *handoff;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	handoff = &context->handoff;
	jp_x86_64_read_cpu_state(&state);
	if ((state.efer & JP_X86_64_EFER_LMA) == 0 || (state.cr0 & JP_X86_64_CR0_PG) == 0 ||
	    (state.rflags & JP_X86_64_RFLAGS_IF) != 0) {
		return JP_BOOT_FAILED;
	}
	if (handoff->architecture != JP_ARCH_X86_64 || !handoff->paging_enabled ||
	    !handoff->interrupts_disabled ||
	    handoff->page_table_root == 0 || handoff->page_table_root % JP_BOOT_PAGE_SIZE != 0 ||
	    handoff->stack_pointer == 0 || handoff->stack_pointer % JP_X86_64_STACK_ALIGNMENT != 0 ||
	    handoff->kernel_entry != context->kernel.entry_point || !is_canonical(handoff->kernel_entry) ||
	    !entry_in_executable_segment(&context->kernel, handoff->kernel_entry)) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}

_Noreturn void jp_x86_64_enter_kernel(JPBootContext *context)
{
	if (context == NULL || context->handoff.state != JP_HANDOFF_STATE_READY ||
	    !context->handoff.paging_enabled || context->handoff.page_table_root == 0) {
		jp_x86_64_park();
	}
	jp_x86_64_jump_to_kernel(context->handoff.page_table_root,
		context->handoff.page_table_memory_base + JP_GDT_PAGE * JP_BOOT_PAGE_SIZE,
		context->handoff.stack_pointer, context->handoff.kernel_entry, context);
}
