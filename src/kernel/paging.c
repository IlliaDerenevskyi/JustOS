/*
 * paging.c - Stage 2 paging.
 *
 * Purpose:
 *   Replaces the writable-and-executable identity map of Stage 1 by a map
 *   with correct permissions at the same addresses, and checks the active map.
 *   The rules are in "Decided rules (replacing the identity map)" of
 *   milestone0.md.
 *
 * Responsibilities:
 *   - decides the permission of every page (kernel segment, framebuffer,
 *     memory region);
 *   - builds the new tree from the unused pages of the Stage 1 table pool;
 *   - switches the root register, and on AArch64 sets WXN;
 *   - jp_kernel_paging_validate walks the active tables and checks them.
 *
 * Limitations:
 *   Owns mappings, not physical pages. Takes no memory except the pool pages.
 *   Table pages are reached through paging_hw.h.
 *
 * Depends on:
 *   justos/kernel/paging.h, justos/kernel/memory.h, arch_dispatch.h,
 *   cpu_regs.h, paging_hw.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/paging.h"

#include "arch_dispatch.h"
#include "cpu_regs.h"
#include "justos/kernel/memory.h"
#include "paging_hw.h"

/* ---- Private constants ---- */

#define JP_PAGING_PAGE_SIZE ((uint64_t)JP_BOOT_PAGE_SIZE)
#define JP_PAGING_PAGE_MASK (JP_PAGING_PAGE_SIZE - 1)
#define JP_PAGING_LARGE_SIZE 0x200000ULL
#define JP_PAGING_ENTRIES 512U
#define JP_PAGING_INDEX_MASK 511ULL
#define JP_PAGING_INDEX_BITS 9U
#define JP_PAGING_PAGE_SHIFT 12U
#define JP_PAGING_LEVELS 4U
#define JP_PAGING_ADDRESS_BITS 48U
#define JP_PAGING_IDENTITY_LIMIT (1ULL << 47)     /* the lower half: the identity map must stay below it */
#define JP_PAGING_ENTRY_ADDRESS 0x0000FFFFFFFFF000ULL
#define JP_PAGING_KERNEL_HALF_FIRST 256U          /* x86_64: the first root entry of the kernel half */
#define JP_PAGING_RESERVED_MAX 0x40000000ULL      /* a RESERVED or MMIO region of 1 GiB or more is not mapped */

/* x86_64 entry bits. */
#define JP_PAGING_X86_PRESENT 0x1ULL
#define JP_PAGING_X86_WRITABLE 0x2ULL
#define JP_PAGING_X86_CACHE_DISABLE 0x10ULL
#define JP_PAGING_X86_LARGE 0x80ULL
#define JP_PAGING_X86_NO_EXECUTE (1ULL << 63)

/* AArch64 descriptor bits. */
#define JP_PAGING_ARM_VALID 0x1ULL
#define JP_PAGING_ARM_TYPE_MASK 0x3ULL
#define JP_PAGING_ARM_BLOCK 0x1ULL
#define JP_PAGING_ARM_TABLE_OR_PAGE 0x3ULL
#define JP_PAGING_ARM_ATTR_INDEX_SHIFT 2U
#define JP_PAGING_ARM_ATTR_DEVICE 0ULL
#define JP_PAGING_ARM_ATTR_NORMAL 1ULL
#define JP_PAGING_ARM_READ_ONLY (1ULL << 7)
#define JP_PAGING_ARM_INNER_SHAREABLE (3ULL << 8)
#define JP_PAGING_ARM_ACCESS_FLAG (1ULL << 10)
#define JP_PAGING_ARM_PXN (1ULL << 53)
#define JP_PAGING_ARM_UXN (1ULL << 54)
#define JP_PAGING_ARM_SCTLR_WXN (1ULL << 19)

typedef enum JPPagePermission {
	JP_PAGE_NONE = 0,
	JP_PAGE_READ_ONLY,
	JP_PAGE_READ_WRITE,
	JP_PAGE_READ_EXECUTE,
	JP_PAGE_UNCACHED,          /* framebuffer: read and write, no execute, uncached */
	JP_PAGE_DEVICE             /* AArch64 MMIO: read and write, no execute, device memory */
} JPPagePermission;

typedef struct JPPagingRange {
	uint64_t base;
	uint64_t end;
} JPPagingRange;

/* ---- Private helpers: arithmetic ---- */

static uint64_t align_down(uint64_t value, uint64_t alignment)
{
	return value & ~(alignment - 1);
}

static bool align_up(uint64_t value, uint64_t alignment, uint64_t *result)
{
	if (value > UINT64_MAX - (alignment - 1)) {
		return false;
	}
	*result = (value + alignment - 1) & ~(alignment - 1);
	return true;
}

/* [base, base + size) rounded outwards to pages. */
static bool page_range(uint64_t base, uint64_t size, JPPagingRange *range)
{
	uint64_t end;

	if (size == 0 || size > UINT64_MAX - base || !align_up(base + size, JP_PAGING_PAGE_SIZE, &end)) {
		return false;
	}
	range->base = align_down(base, JP_PAGING_PAGE_SIZE);
	range->end = end;
	return true;
}

static bool is_x86(JPArchitectureType architecture)
{
	return architecture == JP_ARCH_X86_64;
}

static bool in_pool(const JPHandoffInfo *handoff, uint64_t address)
{
	return address >= handoff->page_table_memory_base &&
	       address - handoff->page_table_memory_base < handoff->page_table_memory_size;
}

static bool root_is_usable(const JPHandoffInfo *handoff, uint64_t root)
{
	return root != 0 && (root & JP_PAGING_PAGE_MASK) == 0 && in_pool(handoff, root);
}

/* ---- Private helpers: permissions ---- */

static bool segment_permission(uint32_t flags, JPPagePermission *permission)
{
	bool write = (flags & JP_ELF_PF_W) != 0;
	bool execute = (flags & JP_ELF_PF_X) != 0;

	if (write && execute) {
		return false;     /* writable and executable at once is never allowed */
	}
	if (write) {
		*permission = JP_PAGE_READ_WRITE;
	} else if (execute) {
		*permission = JP_PAGE_READ_EXECUTE;
	} else {
		*permission = JP_PAGE_READ_ONLY;
	}
	return true;
}

static bool segment_range(const JPKernelSegment *segment, JPPagingRange *range)
{
	return page_range(segment->physical_address, segment->memory_size, range);
}

static bool framebuffer_range(const JPFramebufferInfo *framebuffer, JPPagingRange *range)
{
	if (framebuffer->state != JP_DETECTION_STATE_DETECTED) {
		return false;
	}
	return page_range(framebuffer->base_address, (uint64_t)framebuffer->pitch * framebuffer->height, range);
}

/* The permission of a page of a memory region by the rules of Stage 1 (what is not mapped is JP_PAGE_NONE). */
static JPPagePermission region_permission(JPArchitectureType architecture, const JPMemoryRegion *region)
{
	switch (region->type) {
	case JP_MEMORY_REGION_UNUSABLE:
	case JP_MEMORY_REGION_UNKNOWN:
		return JP_PAGE_NONE;
	case JP_MEMORY_REGION_MMIO:
		if (is_x86(architecture) || region->length >= JP_PAGING_RESERVED_MAX) {
			return JP_PAGE_NONE;
		}
		return JP_PAGE_DEVICE;
	case JP_MEMORY_REGION_RESERVED:
		return region->length >= JP_PAGING_RESERVED_MAX ? JP_PAGE_NONE : JP_PAGE_READ_WRITE;
	default:
		return JP_PAGE_READ_WRITE;
	}
}

/* The checks of the inputs of the permission rules. */
static JPBootResult check_segments(const JPBootContext *context)
{
	size_t i;
	size_t j;

	if (context->kernel.segment_count == 0 || context->kernel.segment_count > JP_MAX_KERNEL_SEGMENTS) {
		return JP_BOOT_FAILED;
	}
	for (i = 0; i < context->kernel.segment_count; i++) {
		JPPagePermission permission;
		JPPagingRange range;

		if (!segment_range(&context->kernel.segments[i], &range)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		if (!segment_permission(context->kernel.segments[i].flags, &permission)) {
			return JP_BOOT_FAILED;
		}
		for (j = 0; j < i; j++) {
			JPPagePermission other;
			JPPagingRange other_range;

			if (!segment_range(&context->kernel.segments[j], &other_range) ||
			    !segment_permission(context->kernel.segments[j].flags, &other)) {
				return JP_BOOT_FAILED;
			}
			if (range.base < other_range.end && other_range.base < range.end && permission != other) {
				return JP_BOOT_FAILED;     /* one page, two permissions */
			}
		}
	}
	return JP_BOOT_SUCCESS;
}

static JPBootResult check_framebuffer(const JPBootContext *context)
{
	JPPagingRange range;
	size_t i;

	if (context->framebuffer.state != JP_DETECTION_STATE_DETECTED) {
		return JP_BOOT_SUCCESS;
	}
	if (context->framebuffer.base_address == 0 || !framebuffer_range(&context->framebuffer, &range)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	for (i = 0; i < context->kernel.segment_count; i++) {
		JPPagingRange segment;

		if (segment_range(&context->kernel.segments[i], &segment) &&
		    range.base < segment.end && segment.base < range.end) {
			return JP_BOOT_FAILED;     /* the framebuffer cannot be kernel code or data */
		}
	}
	return JP_BOOT_SUCCESS;
}

/* The 2 MiB blocks that a mappable region touches: [base, end) rounded outwards to whole blocks. */
static bool region_blocks(const JPMemoryRegion *region, JPPagingRange *range)
{
	uint64_t end;

	if (region->length == 0 || region->length > UINT64_MAX - region->base_address ||
	    !align_up(region->base_address + region->length, JP_PAGING_LARGE_SIZE, &end)) {
		return false;
	}
	range->base = align_down(region->base_address, JP_PAGING_LARGE_SIZE);
	range->end = end;
	return true;
}

/* Two regions that touch the same 2 MiB block must give it the same permission. */
static JPBootResult check_regions(const JPBootContext *context)
{
	size_t i;
	size_t j;

	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		JPPagePermission permission = region_permission(context->handoff.architecture, region);
		JPPagingRange range;

		if (permission == JP_PAGE_NONE) {
			continue;
		}
		if (!region_blocks(region, &range)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		for (j = 0; j < i; j++) {
			const JPMemoryRegion *other = &context->memory_map.regions[j];
			JPPagePermission other_permission = region_permission(context->handoff.architecture, other);
			JPPagingRange other_range;

			if (other_permission == JP_PAGE_NONE || !region_blocks(other, &other_range)) {
				continue;
			}
			if (range.base < other_range.end && other_range.base < range.end && permission != other_permission) {
				return JP_BOOT_FAILED;     /* one block, two kinds of memory */
			}
		}
	}
	return JP_BOOT_SUCCESS;
}

static JPPagePermission permission_of_page(const JPBootContext *context, uint64_t address)
{
	JPArchitectureType architecture = context->handoff.architecture;
	JPPagingRange range;
	size_t i;

	for (i = 0; i < context->kernel.segment_count; i++) {
		JPPagePermission permission;

		if (segment_range(&context->kernel.segments[i], &range) && address >= range.base && address < range.end &&
		    segment_permission(context->kernel.segments[i].flags, &permission)) {
			return permission;
		}
	}
	if (framebuffer_range(&context->framebuffer, &range) && address >= range.base && address < range.end) {
		return JP_PAGE_UNCACHED;
	}
	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		JPPagePermission permission = region_permission(architecture, region);

		if (permission != JP_PAGE_NONE && region_blocks(region, &range) && address >= range.base && address < range.end) {
			return permission;
		}
	}
	return JP_PAGE_NONE;
}

static bool boundary_inside(uint64_t boundary, uint64_t block)
{
	return boundary > block && boundary < block + JP_PAGING_LARGE_SIZE;
}

/* True when no start or end of a segment or of the framebuffer lies strictly inside the block (regions are whole blocks). */
static bool block_is_uniform(const JPBootContext *context, uint64_t block)
{
	JPPagingRange range;
	size_t i;

	for (i = 0; i < context->kernel.segment_count; i++) {
		if (segment_range(&context->kernel.segments[i], &range) &&
		    (boundary_inside(range.base, block) || boundary_inside(range.end, block))) {
			return false;
		}
	}
	if (framebuffer_range(&context->framebuffer, &range) &&
	    (boundary_inside(range.base, block) || boundary_inside(range.end, block))) {
		return false;
	}
	return true;
}

/* ---- Private helpers: entries ---- */

static uint64_t table_entry(JPArchitectureType architecture, uint64_t table)
{
	if (is_x86(architecture)) {
		return table | JP_PAGING_X86_PRESENT | JP_PAGING_X86_WRITABLE;
	}
	return table | JP_PAGING_ARM_TABLE_OR_PAGE;
}

static uint64_t leaf_entry(JPArchitectureType architecture, uint64_t physical, JPPagePermission permission, bool large)
{
	bool writable = permission == JP_PAGE_READ_WRITE || permission == JP_PAGE_UNCACHED || permission == JP_PAGE_DEVICE;
	bool executable = permission == JP_PAGE_READ_EXECUTE;
	bool special = permission == JP_PAGE_UNCACHED || permission == JP_PAGE_DEVICE;
	uint64_t entry;

	if (is_x86(architecture)) {
		entry = physical | JP_PAGING_X86_PRESENT;
		if (large) {
			entry |= JP_PAGING_X86_LARGE;
		}
		if (writable) {
			entry |= JP_PAGING_X86_WRITABLE;
		}
		if (!executable) {
			entry |= JP_PAGING_X86_NO_EXECUTE;
		}
		if (special) {
			entry |= JP_PAGING_X86_CACHE_DISABLE;
		}
		return entry;
	}
	entry = physical | (large ? JP_PAGING_ARM_BLOCK : JP_PAGING_ARM_TABLE_OR_PAGE) |
		JP_PAGING_ARM_INNER_SHAREABLE | JP_PAGING_ARM_ACCESS_FLAG | JP_PAGING_ARM_UXN;
	entry |= (special ? JP_PAGING_ARM_ATTR_DEVICE : JP_PAGING_ARM_ATTR_NORMAL) << JP_PAGING_ARM_ATTR_INDEX_SHIFT;
	if (!writable) {
		entry |= JP_PAGING_ARM_READ_ONLY;
	}
	if (!executable) {
		entry |= JP_PAGING_ARM_PXN;
	}
	return entry;
}

static unsigned int index_at(uint64_t address, unsigned int level)
{
	return (unsigned int)((address >> (JP_PAGING_PAGE_SHIFT + JP_PAGING_INDEX_BITS * level)) & JP_PAGING_INDEX_MASK);
}

/* ---- Private helpers: building ---- */

/* The next free page of the pool, zero-filled: its physical address and a pointer to it. */
static JPBootResult take_page(JPBootContext *context, uint64_t *physical, uint64_t **pointer)
{
	JPHandoffInfo *handoff = &context->handoff;
	uint64_t pages = handoff->page_table_memory_size / JP_PAGING_PAGE_SIZE;
	uint64_t *table;
	unsigned int i;

	if (handoff->page_table_memory_used >= pages) {
		return JP_BOOT_FAILED;
	}
	*physical = handoff->page_table_memory_base + handoff->page_table_memory_used * JP_PAGING_PAGE_SIZE;
	table = jp_kernel_paging_table_at(*physical);
	if (table == NULL) {
		return JP_BOOT_FAILED;
	}
	for (i = 0; i < JP_PAGING_ENTRIES; i++) {
		table[i] = 0;
	}
	handoff->page_table_memory_used++;
	*pointer = table;
	return JP_BOOT_SUCCESS;
}

/* The table that the entry `index` of `table` points to; it is created when the entry is empty. */
static JPBootResult next_table(JPBootContext *context, uint64_t table, unsigned int index, uint64_t *next)
{
	uint64_t *entries = jp_kernel_paging_table_at(table);
	uint64_t *created;
	JPBootResult result;

	if (entries == NULL) {
		return JP_BOOT_FAILED;
	}
	if (entries[index] != 0) {
		*next = entries[index] & JP_PAGING_ENTRY_ADDRESS;
		return JP_BOOT_SUCCESS;
	}
	result = take_page(context, next, &created);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	entries[index] = table_entry(context->handoff.architecture, *next);
	return JP_BOOT_SUCCESS;
}

/* Builds the entry of one 2 MiB block of the identity map. A block that has an entry already is skipped. */
static JPBootResult build_block(JPBootContext *context, uint64_t root, uint64_t block)
{
	JPArchitectureType architecture = context->handoff.architecture;
	uint64_t directory;
	uint64_t *entries;
	uint64_t table;
	uint64_t *pages;
	JPBootResult result;
	JPPagePermission permission;
	unsigned int level;
	unsigned int page;
	bool any = false;

	directory = root;
	for (level = JP_PAGING_LEVELS - 1; level > 1; level--) {
		result = next_table(context, directory, index_at(block, level), &directory);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
	}
	entries = jp_kernel_paging_table_at(directory);
	if (entries == NULL) {
		return JP_BOOT_FAILED;
	}
	if (entries[index_at(block, 1)] != 0) {
		return JP_BOOT_SUCCESS;     /* done by an earlier visit */
	}

	permission = permission_of_page(context, block);
	if (block_is_uniform(context, block)) {
		if (permission != JP_PAGE_NONE) {
			entries[index_at(block, 1)] = leaf_entry(architecture, block, permission, true);
		}
		return JP_BOOT_SUCCESS;
	}

	/* Mixed block: a table of 4 KiB pages; pages without a permission stay empty. */
	for (page = 0; page < JP_PAGING_ENTRIES; page++) {
		if (permission_of_page(context, block + page * JP_PAGING_PAGE_SIZE) != JP_PAGE_NONE) {
			any = true;
			break;
		}
	}
	if (!any) {
		return JP_BOOT_SUCCESS;
	}
	result = take_page(context, &table, &pages);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	entries[index_at(block, 1)] = table_entry(architecture, table);
	entries = pages;
	for (page = 0; page < JP_PAGING_ENTRIES; page++) {
		uint64_t address = block + page * JP_PAGING_PAGE_SIZE;

		permission = permission_of_page(context, address);
		if (permission != JP_PAGE_NONE) {
			entries[page] = leaf_entry(architecture, address, permission, false);
		}
	}
	return JP_BOOT_SUCCESS;
}

static JPBootResult build_range(JPBootContext *context, uint64_t root, uint64_t base, uint64_t end)
{
	uint64_t block;
	uint64_t last;

	if (end > JP_PAGING_IDENTITY_LIMIT || !align_up(end, JP_PAGING_LARGE_SIZE, &last)) {
		return JP_BOOT_FAILED;     /* not in the lower half: the identity map cannot hold it */
	}
	for (block = align_down(base, JP_PAGING_LARGE_SIZE); block < last; block += JP_PAGING_LARGE_SIZE) {
		JPBootResult result = build_block(context, root, block);

		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
	}
	return JP_BOOT_SUCCESS;
}

static JPBootResult build_identity_map(JPBootContext *context, uint64_t root)
{
	JPPagingRange range;
	size_t i;
	JPBootResult result;

	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];

		if (region_permission(context->handoff.architecture, region) == JP_PAGE_NONE) {
			continue;
		}
		if (!region_blocks(region, &range)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		result = build_range(context, root, range.base, range.end);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
	}
	for (i = 0; i < context->kernel.segment_count; i++) {
		if (!segment_range(&context->kernel.segments[i], &range)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		result = build_range(context, root, range.base, range.end);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
	}
	if (framebuffer_range(&context->framebuffer, &range)) {
		return build_range(context, root, range.base, range.end);
	}
	return JP_BOOT_SUCCESS;
}

/* ---- Private helpers: the inputs of initialize ---- */

static bool pool_is_owned_as_page_tables(const JPBootContext *context)
{
	const JPHandoffInfo *handoff = &context->handoff;
	JPKernelMemoryOwner owner = JP_KERNEL_MEMORY_AVAILABLE;

	if (handoff->page_table_memory_size == 0 || (handoff->page_table_memory_base & JP_PAGING_PAGE_MASK) != 0 ||
	    (handoff->page_table_memory_size & JP_PAGING_PAGE_MASK) != 0) {
		return false;
	}
	return jp_kernel_memory_query(context, handoff->page_table_memory_base, handoff->page_table_memory_size,
		&owner) == JP_BOOT_SUCCESS && owner == JP_KERNEL_MEMORY_PAGE_TABLES;
}

static JPBootResult check_inputs(const JPBootContext *context, uint64_t *old_root)
{
	const JPHandoffInfo *handoff = &context->handoff;
	JPBootResult result;

	if (context->stage2.entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!JP_KERNEL_ARCH_IS_SUPPORTED(handoff->architecture) || !handoff->paging_enabled ||
	    !pool_is_owned_as_page_tables(context) ||
	    handoff->page_table_memory_used > handoff->page_table_memory_size / JP_PAGING_PAGE_SIZE) {
		return JP_BOOT_FAILED;
	}
	*old_root = is_x86(handoff->architecture) ? jp_kernel_paging_read_root() : handoff->page_table_root;
	if (!root_is_usable(handoff, *old_root)) {
		return JP_BOOT_FAILED;
	}
	result = check_segments(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = check_regions(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	return check_framebuffer(context);
}

#if !defined(JP_KERNEL_ONLY_X86_64)
/* AArch64: after the root is active, WXN is set and read back. */
static JPBootResult aarch64_after_switch(void)
{
	JPKernelAArch64Registers registers;

	jp_kernel_aarch64_set_wxn();
	jp_kernel_aarch64_read_registers(&registers);
	return (registers.sctlr_el1 & JP_PAGING_ARM_SCTLR_WXN) != 0 ? JP_BOOT_SUCCESS : JP_BOOT_FAILED;
}

static bool aarch64_wxn_is_set(void)
{
	JPKernelAArch64Registers registers;

	jp_kernel_aarch64_read_registers(&registers);
	return (registers.sctlr_el1 & JP_PAGING_ARM_SCTLR_WXN) != 0;
}
#endif /* !JP_KERNEL_ONLY_X86_64 */

/* ---- Public API: initialize ---- */

JPBootResult jp_kernel_paging_initialize(JPBootContext *context)
{
	uint64_t old_root = 0;
	uint64_t new_root;
	uint64_t *new_entries;
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	result = check_inputs(context, &old_root);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}

	result = take_page(context, &new_root, &new_entries);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	if (is_x86(context->handoff.architecture)) {
		const uint64_t *old_entries = jp_kernel_paging_table_at(old_root);
		unsigned int i;

		if (old_entries == NULL) {
			return JP_BOOT_FAILED;
		}
		for (i = JP_PAGING_KERNEL_HALF_FIRST; i < JP_PAGING_ENTRIES; i++) {
			new_entries[i] = old_entries[i];     /* the kernel half stays */
		}
	}
	result = build_identity_map(context, new_root);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}

	jp_kernel_paging_write_root(new_root);
	if (jp_kernel_paging_read_root() != new_root) {
		return JP_BOOT_FAILED;
	}
	return JP_KERNEL_ARCH_DISPATCH(context->handoff.architecture, JP_BOOT_SUCCESS, aarch64_after_switch());
}

/* ---- Private helpers: validate ---- */

typedef struct JPLeaf {
	uint64_t physical;         /* the physical address that the virtual address maps to */
	uint64_t span;             /* the bytes from the address to the end of the leaf */
	bool writable;
	bool executable;
} JPLeaf;

/* Walks the tables from `root`; false if an entry is missing or not allowed. */
static bool walk(JPArchitectureType architecture, uint64_t root, uint64_t address, JPLeaf *leaf)
{
	uint64_t table = root;
	unsigned int level;

	if (address >> JP_PAGING_ADDRESS_BITS != 0 && (address >> JP_PAGING_ADDRESS_BITS) != 0xFFFF) {
		return false;
	}
	for (level = JP_PAGING_LEVELS - 1; ; level--) {
		const uint64_t *entries = jp_kernel_paging_table_at(table);
		uint64_t entry;
		uint64_t size = 1ULL << (JP_PAGING_PAGE_SHIFT + JP_PAGING_INDEX_BITS * level);
		bool is_leaf;

		if (entries == NULL) {
			return false;
		}
		entry = entries[index_at(address, level)];
		if (is_x86(architecture)) {
			if ((entry & JP_PAGING_X86_PRESENT) == 0) {
				return false;
			}
			if (level >= 2 && (entry & JP_PAGING_X86_LARGE) != 0) {
				return false;     /* 1 GiB pages are not used */
			}
			is_leaf = level == 0 || (level == 1 && (entry & JP_PAGING_X86_LARGE) != 0);
			if (is_leaf) {
				leaf->physical = (entry & JP_PAGING_ENTRY_ADDRESS) | (address & (size - 1));
				leaf->span = size - (address & (size - 1));
				leaf->writable = (entry & JP_PAGING_X86_WRITABLE) != 0;
				leaf->executable = (entry & JP_PAGING_X86_NO_EXECUTE) == 0;
				return true;
			}
		} else {
			if ((entry & JP_PAGING_ARM_VALID) == 0) {
				return false;
			}
			is_leaf = (level == 0 && (entry & JP_PAGING_ARM_TYPE_MASK) == JP_PAGING_ARM_TABLE_OR_PAGE) ||
			          (level == 1 && (entry & JP_PAGING_ARM_TYPE_MASK) == JP_PAGING_ARM_BLOCK);
			if (level >= 2 && (entry & JP_PAGING_ARM_TYPE_MASK) == JP_PAGING_ARM_BLOCK) {
				return false;     /* 1 GiB blocks are not used */
			}
			if (is_leaf) {
				leaf->physical = (entry & JP_PAGING_ENTRY_ADDRESS) | (address & (size - 1));
				leaf->span = size - (address & (size - 1));
				leaf->writable = (entry & JP_PAGING_ARM_READ_ONLY) == 0;
				leaf->executable = (entry & JP_PAGING_ARM_PXN) == 0;
				return true;
			}
			if ((entry & JP_PAGING_ARM_TYPE_MASK) != JP_PAGING_ARM_TABLE_OR_PAGE) {
				return false;
			}
		}
		if (level == 0) {
			return false;
		}
		table = entry & JP_PAGING_ENTRY_ADDRESS;
	}
}

static bool leaf_has_permission(const JPLeaf *leaf, JPPagePermission permission)
{
	switch (permission) {
	case JP_PAGE_READ_ONLY:
		return !leaf->writable && !leaf->executable;
	case JP_PAGE_READ_EXECUTE:
		return !leaf->writable && leaf->executable;
	default:
		return leaf->writable && !leaf->executable;
	}
}

static bool segments_are_mapped(const JPBootContext *context, uint64_t identity_root, uint64_t kernel_root)
{
	JPArchitectureType architecture = context->handoff.architecture;
	size_t i;

	for (i = 0; i < context->kernel.segment_count; i++) {
		const JPKernelSegment *segment = &context->kernel.segments[i];
		JPPagePermission permission;
		JPPagingRange range;
		uint64_t address;

		if (!segment_range(segment, &range) || !segment_permission(segment->flags, &permission)) {
			return false;
		}
		for (address = range.base; address < range.end; address += JP_PAGING_PAGE_SIZE) {
			uint64_t virtual_address = align_down(segment->virtual_address, JP_PAGING_PAGE_SIZE) + (address - range.base);
			JPLeaf leaf;

			if (!walk(architecture, identity_root, address, &leaf) || leaf.physical != address ||
			    !leaf_has_permission(&leaf, permission)) {
				return false;
			}
			if (!walk(architecture, kernel_root, virtual_address, &leaf) || leaf.physical != address ||
			    !leaf_has_permission(&leaf, permission)) {
				return false;
			}
		}
	}
	return true;
}

static bool owner_is_plain_memory(uint32_t owner)
{
	return owner == (uint32_t)JP_KERNEL_MEMORY_KERNEL_STACK || owner == (uint32_t)JP_KERNEL_MEMORY_PAGE_TABLES ||
	       owner == (uint32_t)JP_KERNEL_MEMORY_EARLY_HEAP || owner == (uint32_t)JP_KERNEL_MEMORY_STAGE2_RUNTIME ||
	       owner == (uint32_t)JP_KERNEL_MEMORY_AVAILABLE;
}

static bool owned_memory_is_mapped(const JPBootContext *context, uint64_t identity_root)
{
	size_t i;

	for (i = 0; i < context->stage2.entry_count; i++) {
		const JPBootStage2MemoryEntry *entry = &context->stage2.entries[i];
		uint64_t address = entry->base_address;
		uint64_t end = entry->base_address + entry->length;

		if (!owner_is_plain_memory(entry->owner)) {
			continue;
		}
		while (address < end) {
			JPLeaf leaf;

			if (!walk(context->handoff.architecture, identity_root, address, &leaf) || leaf.physical != address ||
			    !leaf_has_permission(&leaf, JP_PAGE_READ_WRITE)) {
				return false;
			}
			if (leaf.span >= end - address) {
				break;
			}
			address += leaf.span;
		}
	}
	return true;
}

/* ---- Public API: validate ---- */

JPBootResult jp_kernel_paging_validate(JPBootContext *context)
{
	const JPHandoffInfo *handoff;
	uint64_t root;
	uint64_t kernel_root;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage2.entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	handoff = &context->handoff;
	if (!JP_KERNEL_ARCH_IS_SUPPORTED(handoff->architecture)) {
		return JP_BOOT_FAILED;
	}
	root = jp_kernel_paging_read_root();
	kernel_root = is_x86(handoff->architecture) ? root : handoff->page_table_root;
	if (!root_is_usable(handoff, root) || !root_is_usable(handoff, kernel_root)) {
		return JP_BOOT_FAILED;
	}
	if (!JP_KERNEL_ARCH_DISPATCH(handoff->architecture, true, aarch64_wxn_is_set())) {
		return JP_BOOT_FAILED;
	}
	if (!segments_are_mapped(context, root, kernel_root) || !owned_memory_is_mapped(context, root)) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}
