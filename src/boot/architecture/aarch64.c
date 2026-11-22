/*
 * aarch64.c - the AArch64 architecture operations of Stage 1.
 *
 * Purpose:
 *   Machine-dependent work for AArch64: the boot translation tables (a tree
 *   for the kernel half and a tree for the identity map), the kernel entry
 *   state, and the jump into the kernel.
 *
 * Responsibilities:
 *   - the table memory (one block, see "Decided rules (AArch64 ...)" in
 *     milestone0.md): created by the first mapping, reserved with the firmware;
 *   - kernel segment mapping, the identity map, and the framebuffer mapping;
 *   - CPU discovery (MIDR_EL1), entry-state checks, and enter_kernel.
 *
 * Limitations:
 *   The instructions that need assembly (masking interrupts, reading system
 *   registers, the jump with the drop from EL2 to EL1) are in aarch64_enter.S
 *   behind aarch64_cpu.h. No firmware logic. Freestanding: no libc.
 *
 * Depends on:
 *   justos/boot/architecture/aarch64.h, justos/boot/boot_environment.h,
 *   aarch64_cpu.h.
 *
 * Must not depend on:
 *   Provider headers.
 */

#include "justos/boot/architecture/aarch64.h"

#include "justos/boot/boot_environment.h"
#include "aarch64_cpu.h"

/* ---- Public API ---- */

JPBootResult jp_aarch64_validate_entry_state(const JPBootContext *context);
JPBootResult jp_aarch64_setup_paging(JPBootContext *context);
JPBootResult jp_aarch64_prepare_entry(JPBootContext *context);
JPBootResult jp_aarch64_validate_elf_machine(const JPKernelHeader *header);
JPBootResult jp_aarch64_discover_cpu(JPCpuInfo *cpu);
JPBootResult jp_aarch64_map_segment(JPBootContext *context, size_t index);
void *jp_aarch64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length);
_Noreturn void jp_aarch64_enter_kernel(JPBootContext *context);

/* ---- Private constants ---- */

enum {
	JP_ENTRIES_PER_TABLE = 512,
	JP_INDEX_BITS = 9,
	JP_PAGE_SHIFT = 12,
	JP_LEVEL_BLOCK = 2,                  /* the level whose entries can be 2 MiB blocks */
	JP_LEVEL_PAGE = 3,
	JP_TTBR1_PAGE = 0,                   /* page 0 of the pool: the kernel half */
	JP_TTBR0_PAGE = 1,                   /* page 1 of the pool: the identity map */
	JP_ADDRESS_TOP_SHIFT = 48,
	JP_ADDRESS_TOP_ZERO = 0,
	JP_ADDRESS_TOP_ONES = 0xFFFF,
	JP_ATTR_DEVICE = 0,
	JP_ATTR_NORMAL = 1,
	JP_ATTR_SHIFT = 2,
	JP_SHAREABLE_INNER = 3,
	JP_SHAREABLE_SHIFT = 8,
	JP_PF_X = 1,
	JP_PF_W = 2,
	JP_EL1 = 1,
	JP_EL2 = 2
};

#define JP_DESC_VALID 0x1ULL
#define JP_DESC_TABLE_OR_PAGE 0x3ULL         /* bits 1:0 */
#define JP_DESC_BLOCK 0x1ULL                 /* bits 1:0 */
#define JP_DESC_TYPE_MASK 0x3ULL
#define JP_DESC_AF (1ULL << 10)
#define JP_DESC_AP_READ_ONLY (1ULL << 7)
#define JP_DESC_PXN (1ULL << 53)
#define JP_DESC_UXN (1ULL << 54)
#define JP_DESC_ADDRESS_MASK 0x0000FFFFFFFFF000ULL
#define JP_BLOCK_SIZE 0x200000ULL
#define JP_POOL_BYTES ((uint64_t)JP_AARCH64_TABLE_POOL_PAGES * JP_BOOT_PAGE_SIZE)

/* ---- Private helpers: physical memory ---- */

#ifdef JP_AARCH64_TESTING
/* Host tests turn a physical address into a pointer into a host buffer. */
void *(*jp_aarch64_test_phys_to_pointer)(uint64_t physical);
#endif

/* In Milestone 0 all memory that Stage 1 touches is reachable at its physical address. */
static void *physical_pointer(uint64_t physical)
{
#ifdef JP_AARCH64_TESTING
	if (jp_aarch64_test_phys_to_pointer != NULL) {
		return jp_aarch64_test_phys_to_pointer(physical);
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

/* A valid address has its upper 16 bits all zero (the identity half) or all one (the kernel half). */
static bool is_canonical(uint64_t address)
{
	uint64_t top = address >> JP_ADDRESS_TOP_SHIFT;

	return top == JP_ADDRESS_TOP_ZERO || top == JP_ADDRESS_TOP_ONES;
}

static bool is_kernel_half(uint64_t address)
{
	return (address >> JP_ADDRESS_TOP_SHIFT) == JP_ADDRESS_TOP_ONES;
}

/* ---- Private helpers: the table memory (the pool) ---- */

/* The highest candidate for the pool whose end is at most `limit` (the same rule as on x86_64). */
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

/* Creates the pool at the first need (placed, reserved with retries, zeroed, described in the handoff). */
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
	for (attempt = 0; attempt < JP_AARCH64_POOL_ATTEMPTS; attempt++) {
		if (!find_pool_address(context, limit, &address)) {
			return JP_BOOT_FAILED;
		}
		if (jp_boot_environment_reserve_memory(context, address, JP_POOL_BYTES) == JP_BOOT_SUCCESS) {
			reserved = true;
			break;
		}
		limit = address;
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
	/* Page 0 is the TTBR1 root and page 1 the TTBR0 root. */
	context->handoff.page_table_memory_used = JP_TTBR0_PAGE + 1;
	context->handoff.page_table_root = address + JP_TTBR1_PAGE * JP_BOOT_PAGE_SIZE;
	return JP_BOOT_SUCCESS;
}

static uint64_t ttbr0_root(const JPBootContext *context)
{
	return context->handoff.page_table_memory_base + JP_TTBR0_PAGE * JP_BOOT_PAGE_SIZE;
}

/* The root table of the half that an address belongs to. */
static uint64_t root_for(const JPBootContext *context, uint64_t virtual_address)
{
	return is_kernel_half(virtual_address) ? context->handoff.page_table_root : ttbr0_root(context);
}

/* Hands out the next zeroed page of the pool; returns its physical address, or 0 if the pool is full. */
static uint64_t allocate_table(JPBootContext *context)
{
	uint64_t page = context->handoff.page_table_memory_used;

	if (page >= JP_AARCH64_TABLE_POOL_PAGES) {
		return 0;
	}
	context->handoff.page_table_memory_used = page + 1;
	return context->handoff.page_table_memory_base + page * JP_BOOT_PAGE_SIZE;
}

/* The index of an address in the table of the given level (0 is the top level, 3 the page table). */
static unsigned int table_index(uint64_t virtual_address, int level)
{
	return (unsigned int)((virtual_address >> (JP_PAGE_SHIFT + JP_INDEX_BITS * (JP_LEVEL_PAGE - level))) &
		(JP_ENTRIES_PER_TABLE - 1));
}

/*
 * Walks down from the root to the table of the level `stop_level`, creating
 * missing tables. Returns the table, or NULL (pool full, a block in the way).
 */
static uint64_t *table_for(JPBootContext *context, uint64_t root, uint64_t virtual_address, int stop_level)
{
	uint64_t *table = (uint64_t *)physical_pointer(root);
	int level;

	for (level = 0; level < stop_level; level++) {
		uint64_t *entry;

		if (table == NULL) {
			return NULL;
		}
		entry = &table[table_index(virtual_address, level)];
		if ((*entry & JP_DESC_VALID) == 0) {
			uint64_t next = allocate_table(context);

			if (next == 0) {
				return NULL;
			}
			*entry = next | JP_DESC_TABLE_OR_PAGE;
		} else if ((*entry & JP_DESC_TYPE_MASK) != JP_DESC_TABLE_OR_PAGE) {
			return NULL;           /* a block in the way */
		}
		table = (uint64_t *)physical_pointer(*entry & JP_DESC_ADDRESS_MASK);
	}
	return table;
}

/* One 4 KiB page of a kernel segment. A page that is already mapped is a conflict. */
static bool map_small_page(JPBootContext *context, uint64_t virtual_address, uint64_t physical, uint32_t flags)
{
	uint64_t *table = table_for(context, root_for(context, virtual_address), virtual_address, JP_LEVEL_PAGE);
	uint64_t *entry;
	uint64_t value;

	if (table == NULL) {
		return false;
	}
	entry = &table[table_index(virtual_address, JP_LEVEL_PAGE)];
	if ((*entry & JP_DESC_VALID) != 0) {
		return false;
	}
	value = physical | JP_DESC_TABLE_OR_PAGE | ((uint64_t)JP_ATTR_NORMAL << JP_ATTR_SHIFT) |
		((uint64_t)JP_SHAREABLE_INNER << JP_SHAREABLE_SHIFT) | JP_DESC_AF | JP_DESC_UXN;
	if ((flags & JP_PF_W) == 0) {
		value |= JP_DESC_AP_READ_ONLY;
	}
	if ((flags & JP_PF_X) == 0) {
		value |= JP_DESC_PXN;
	}
	*entry = value;
	return true;
}

/*
 * One 2 MiB identity block in the TTBR0 tree. A block that already is the same
 * identity block with the same attributes is accepted (two regions share it);
 * another identity block is replaced only when replace_identity is set (the
 * framebuffer changes the attributes); any other entry is a conflict.
 */
static bool map_identity_block(JPBootContext *context, uint64_t address, uint64_t attributes, bool replace_identity)
{
	uint64_t *table = table_for(context, ttbr0_root(context), address, JP_LEVEL_BLOCK);
	uint64_t *entry;
	uint64_t wanted = address | JP_DESC_BLOCK | attributes;

	if (table == NULL) {
		return false;
	}
	entry = &table[table_index(address, JP_LEVEL_BLOCK)];
	if ((*entry & JP_DESC_VALID) != 0) {
		bool same_identity = (*entry & JP_DESC_TYPE_MASK) == JP_DESC_BLOCK &&
			(*entry & JP_DESC_ADDRESS_MASK) == address;

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

static uint64_t normal_block_attributes(void)
{
	return ((uint64_t)JP_ATTR_NORMAL << JP_ATTR_SHIFT) | ((uint64_t)JP_SHAREABLE_INNER << JP_SHAREABLE_SHIFT) |
		JP_DESC_AF | JP_DESC_UXN;
}

static uint64_t device_block_attributes(void)
{
	return ((uint64_t)JP_ATTR_DEVICE << JP_ATTR_SHIFT) | JP_DESC_AF | JP_DESC_PXN | JP_DESC_UXN;
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

JPBootResult jp_aarch64_validate_elf_machine(const JPKernelHeader *header)
{
	if (header == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	return header->machine == JP_ELF_MACHINE_AARCH64 ? JP_BOOT_SUCCESS : JP_BOOT_ERROR_CORRUPTED_DATA;
}

JPBootResult jp_aarch64_discover_cpu(JPCpuInfo *cpu)
{
	JPA64CpuState state;

	if (cpu == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	jp_aarch64_read_cpu_state(&state);
	if (state.midr == 0) {
		return JP_BOOT_FAILED;       /* the identification register was not really read */
	}
	/* Only the boot processor is counted: AArch64 has no instruction that tells how many exist. */
	cpu->logical_cores = 1;
	cpu->state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

void *jp_aarch64_phys_access(JPBootContext *context, uint64_t physical, uint64_t length)
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

JPBootResult jp_aarch64_map_segment(JPBootContext *context, size_t index)
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

JPBootResult jp_aarch64_setup_paging(JPBootContext *context)
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

	/* The identity map follows the regions; memory-mapped I/O becomes device memory. */
	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		uint64_t first;
		uint64_t last;
		uint64_t attributes;

		if (region->type == JP_MEMORY_REGION_UNUSABLE || region->length == 0 ||
		    region->length > UINT64_MAX - region->base_address - (JP_BLOCK_SIZE - 1)) {
			continue;
		}
		if ((region->type == JP_MEMORY_REGION_RESERVED || region->type == JP_MEMORY_REGION_MMIO) &&
		    region->length >= JP_AARCH64_IDENTITY_RESERVED_MAX) {
			continue;    /* an address range kept by the firmware or by devices, not memory in use */
		}
		attributes = region->type == JP_MEMORY_REGION_MMIO ? device_block_attributes() : normal_block_attributes();
		first = align_down(region->base_address, JP_BLOCK_SIZE);
		last = align_down(region->base_address + region->length + JP_BLOCK_SIZE - 1, JP_BLOCK_SIZE);
		for (address = first; address < last; address += JP_BLOCK_SIZE) {
			if (!map_identity_block(context, address, attributes, false)) {
				return JP_BOOT_FAILED;
			}
		}
		mapped_any = true;
	}
	if (!mapped_any) {
		return JP_BOOT_FAILED;
	}

	/* The framebuffer: device memory, never executable. */
	if (context->framebuffer.state == JP_DETECTION_STATE_DETECTED && context->framebuffer.base_address != 0) {
		uint64_t size = (uint64_t)context->framebuffer.pitch * context->framebuffer.height;
		uint64_t base = context->framebuffer.base_address;
		uint64_t first;
		uint64_t last;

		if (size == 0 || size > UINT64_MAX - base - (JP_BLOCK_SIZE - 1)) {
			return JP_BOOT_FAILED;
		}
		first = align_down(base, JP_BLOCK_SIZE);
		last = align_down(base + size + JP_BLOCK_SIZE - 1, JP_BLOCK_SIZE);
		for (address = first; address < last; address += JP_BLOCK_SIZE) {
			if (!map_identity_block(context, address, device_block_attributes(), true)) {
				return JP_BOOT_FAILED;
			}
		}
	}

	context->handoff.paging_enabled = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_aarch64_prepare_entry(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->handoff.page_table_root == 0 || !context->handoff.paging_enabled ||
	    context->handoff.stack_pointer == 0 ||
	    context->handoff.stack_pointer % JP_AARCH64_STACK_ALIGNMENT != 0 ||
	    context->kernel.entry_point == 0 || !is_canonical(context->kernel.entry_point)) {
		return JP_BOOT_FAILED;
	}
	/* The interrupts are really masked first; only then is the fact recorded. */
	jp_aarch64_disable_interrupts();
	context->handoff.kernel_entry = context->kernel.entry_point;
	context->handoff.architecture = JP_ARCH_AARCH64;
	context->handoff.interrupts_disabled = true;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_aarch64_validate_entry_state(const JPBootContext *context)
{
	JPA64CpuState state;
	const JPHandoffInfo *handoff;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	handoff = &context->handoff;
	jp_aarch64_read_cpu_state(&state);
	if ((state.current_el != JP_EL1 && state.current_el != JP_EL2) ||
	    state.daif != JP_AARCH64_DAIF_ALL_MASKED || (state.sctlr & JP_AARCH64_SCTLR_MMU) == 0) {
		return JP_BOOT_FAILED;
	}
	if (handoff->architecture != JP_ARCH_AARCH64 || !handoff->paging_enabled ||
	    !handoff->interrupts_disabled ||
	    handoff->page_table_root == 0 || handoff->page_table_root % JP_BOOT_PAGE_SIZE != 0 ||
	    handoff->stack_pointer == 0 || handoff->stack_pointer % JP_AARCH64_STACK_ALIGNMENT != 0 ||
	    handoff->kernel_entry != context->kernel.entry_point || !is_canonical(handoff->kernel_entry) ||
	    !entry_in_executable_segment(&context->kernel, handoff->kernel_entry)) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}

_Noreturn void jp_aarch64_enter_kernel(JPBootContext *context)
{
	if (context == NULL || context->handoff.state != JP_HANDOFF_STATE_READY ||
	    !context->handoff.paging_enabled || context->handoff.page_table_root == 0) {
		jp_aarch64_park();
	}
	jp_aarch64_jump_to_kernel(context->handoff.page_table_root, ttbr0_root(context),
		context->handoff.stack_pointer, context->handoff.kernel_entry, context);
}
