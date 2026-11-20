/*
 * memory.c - Stage 2 physical-memory ownership.
 *
 * Purpose:
 *   Keeps the table "address range -> owner" in context->stage2 and offers the
 *   only operations that change it. The rules are in "Decided rules (the
 *   ownership table and the initial owners)" of milestone0.md.
 *
 * Responsibilities:
 *   - jp_kernel_memory_initialize builds the table in two passes;
 *   - reserve, allocate, query, release, validate_ownership.
 *
 * Limitations:
 *   Physical ownership only. Does not touch firmware or page tables. No
 *   allocation of its own: the table lives inside the context, and the
 *   operations are written with plain loops.
 *
 * Depends on:
 *   justos/kernel/memory.h, cpu_regs.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/memory.h"

#include "cpu_regs.h"

/* ---- Private constants ---- */

#define JP_MEMORY_PAGE_MASK ((uint64_t)JP_BOOT_PAGE_SIZE - 1)
#define JP_MEMORY_OWNER_COUNT ((uint32_t)JP_KERNEL_MEMORY_AVAILABLE + 1)
#define JP_MEMORY_NO_INDEX ((size_t)-1)
#define JP_MEMORY_BITS_UNLIMITED 64

#define JP_MEMORY_OWNER_BIT(owner) ((uint32_t)1 << (uint32_t)(owner))
#define JP_MEMORY_ALL_OWNERS ((JP_MEMORY_OWNER_BIT(JP_MEMORY_OWNER_COUNT)) - 1)

/* ---- Private helpers: arithmetic ---- */

static uint64_t align_down_page(uint64_t value)
{
	return value & ~JP_MEMORY_PAGE_MASK;
}

/* Rounds up; returns false when the result does not fit. */
static bool align_up_page(uint64_t value, uint64_t *result)
{
	if (value > UINT64_MAX - JP_MEMORY_PAGE_MASK) {
		return false;
	}
	*result = (value + JP_MEMORY_PAGE_MASK) & ~JP_MEMORY_PAGE_MASK;
	return true;
}

static bool range_is_valid(uint64_t base, uint64_t length)
{
	return length > 0 && length <= UINT64_MAX - base;
}

static bool is_page_multiple(uint64_t value)
{
	return (value & JP_MEMORY_PAGE_MASK) == 0;
}

static bool owner_is_valid(uint32_t owner)
{
	return owner < JP_MEMORY_OWNER_COUNT;
}

/* The owners that a caller may ask for with reserve and allocate. */
static bool owner_is_requestable(JPKernelMemoryOwner owner)
{
	switch (owner) {
	case JP_KERNEL_MEMORY_KERNEL_IMAGE:
	case JP_KERNEL_MEMORY_KERNEL_STACK:
	case JP_KERNEL_MEMORY_PAGE_TABLES:
	case JP_KERNEL_MEMORY_FRAMEBUFFER:
	case JP_KERNEL_MEMORY_STAGE2_RUNTIME:
	case JP_KERNEL_MEMORY_EARLY_HEAP:
		return true;
	default:
		return false;
	}
}

static uint64_t entry_end(const JPBootStage2MemoryEntry *entry)
{
	return entry->base_address + entry->length;
}

/* ---- Private helpers: the table ---- */

static JPBootStage2Area *table_of(JPBootContext *context)
{
	return &context->stage2;
}

static bool is_strictly_inside_an_entry(const JPBootStage2Area *table, uint64_t address)
{
	size_t i;

	for (i = 0; i < table->entry_count; i++) {
		if (table->entries[i].base_address < address && address < entry_end(&table->entries[i])) {
			return true;
		}
	}
	return false;
}

/* True when giving [base, end) to one owner fits: it adds one entry for each end that cuts an entry. */
static bool table_has_room(const JPBootStage2Area *table, uint64_t base, uint64_t end)
{
	size_t needed = 0;

	if (is_strictly_inside_an_entry(table, base)) {
		needed++;
	}
	if (is_strictly_inside_an_entry(table, end)) {
		needed++;
	}
	return table->entry_count + needed <= JP_KERNEL_MEMORY_MAX_ENTRIES;
}

static void clear_table(JPBootStage2Area *table)
{
	size_t i;

	table->entry_count = 0;
	for (i = 0; i < JP_KERNEL_MEMORY_MAX_ENTRIES; i++) {
		table->entries[i].base_address = 0;
		table->entries[i].length = 0;
		table->entries[i].owner = 0;
	}
}

/*
 * Walks [base, end) through the entries. Returns true when the whole range is
 * covered without a hole and every entry that touches it has an owner whose
 * bit is in `allowed`. `*single` tells whether all those entries have the same
 * owner, and `*first_owner` is the owner of the first one.
 */
static bool scan_range(const JPBootStage2Area *table, uint64_t base, uint64_t end, uint32_t allowed,
	bool *single, uint32_t *first_owner)
{
	uint64_t position = base;
	bool have_owner = false;
	size_t i;

	*single = true;
	*first_owner = 0;
	for (i = 0; i < table->entry_count && position < end; i++) {
		const JPBootStage2MemoryEntry *entry = &table->entries[i];

		if (entry_end(entry) <= position) {
			continue;
		}
		if (entry->base_address > position) {
			return false;     /* a hole */
		}
		if ((allowed & JP_MEMORY_OWNER_BIT(entry->owner)) == 0) {
			return false;
		}
		if (!have_owner) {
			*first_owner = entry->owner;
			have_owner = true;
		} else if (entry->owner != *first_owner) {
			*single = false;
		}
		position = entry_end(entry);
	}
	return position >= end;
}

static bool range_touches_table(const JPBootStage2Area *table, uint64_t base, uint64_t end)
{
	size_t i;

	for (i = 0; i < table->entry_count; i++) {
		if (table->entries[i].base_address < end && entry_end(&table->entries[i]) > base) {
			return true;
		}
	}
	return false;
}

/* Cuts the entry that contains `address` strictly inside it into two. The caller checked the room. */
static void split_at(JPBootStage2Area *table, uint64_t address)
{
	size_t i;
	size_t j;

	for (i = 0; i < table->entry_count; i++) {
		JPBootStage2MemoryEntry *entry = &table->entries[i];

		if (entry->base_address < address && address < entry_end(entry)) {
			for (j = table->entry_count; j > i + 1; j--) {
				table->entries[j] = table->entries[j - 1];
			}
			table->entries[i + 1].base_address = address;
			table->entries[i + 1].length = entry_end(entry) - address;
			table->entries[i + 1].owner = entry->owner;
			entry->length = address - entry->base_address;
			table->entry_count++;
			return;
		}
	}
}

/* Joins neighbours that touch and have the same owner. */
static void merge_neighbours(JPBootStage2Area *table)
{
	size_t read;
	size_t write = 0;

	for (read = 0; read < table->entry_count; read++) {
		if (write > 0 &&
		    entry_end(&table->entries[write - 1]) == table->entries[read].base_address &&
		    table->entries[write - 1].owner == table->entries[read].owner) {
			table->entries[write - 1].length += table->entries[read].length;
		} else {
			table->entries[write] = table->entries[read];
			write++;
		}
	}
	for (read = write; read < table->entry_count; read++) {
		table->entries[read].base_address = 0;
		table->entries[read].length = 0;
		table->entries[read].owner = 0;
	}
	table->entry_count = write;
}

/* Gives [base, end) to `owner`. The caller checked that the range is covered and that there is room. */
static void set_owner(JPBootStage2Area *table, uint64_t base, uint64_t end, JPKernelMemoryOwner owner)
{
	size_t i;

	split_at(table, base);
	split_at(table, end);
	for (i = 0; i < table->entry_count; i++) {
		if (table->entries[i].base_address >= base && entry_end(&table->entries[i]) <= end) {
			table->entries[i].owner = (uint32_t)owner;
		}
	}
	merge_neighbours(table);
}

/* ---- Private helpers: initialize ---- */

/* Adds one region of the Stage 1 map in its sorted place; false when it overlaps another one. */
static bool insert_region(JPBootStage2Area *table, const JPMemoryRegion *region, JPKernelMemoryOwner owner)
{
	uint64_t end = region->base_address + region->length;
	size_t position = 0;
	size_t i;

	while (position < table->entry_count && table->entries[position].base_address < region->base_address) {
		position++;
	}
	if (position > 0 && entry_end(&table->entries[position - 1]) > region->base_address) {
		return false;
	}
	if (position < table->entry_count && end > table->entries[position].base_address) {
		return false;
	}
	for (i = table->entry_count; i > position; i--) {
		table->entries[i] = table->entries[i - 1];
	}
	table->entries[position].base_address = region->base_address;
	table->entries[position].length = region->length;
	table->entries[position].owner = (uint32_t)owner;
	table->entry_count++;
	return true;
}

static JPKernelMemoryOwner owner_of_region_type(JPMemoryRegionType type)
{
	switch (type) {
	case JP_MEMORY_REGION_USABLE:
		return JP_KERNEL_MEMORY_AVAILABLE;
	case JP_MEMORY_REGION_RESERVED:
		return JP_KERNEL_MEMORY_STAGE1_RESERVED;
	case JP_MEMORY_REGION_ACPI:
	case JP_MEMORY_REGION_MMIO:
	case JP_MEMORY_REGION_FIRMWARE:
		return JP_KERNEL_MEMORY_FIRMWARE_RESERVED;
	case JP_MEMORY_REGION_UNUSABLE:
	case JP_MEMORY_REGION_UNKNOWN:
	default:
		return JP_KERNEL_MEMORY_UNAVAILABLE;
	}
}

/* Pass 1: one entry per region of the Stage 1 map. */
static JPBootResult build_from_map(JPBootContext *context, JPBootStage2Area *table)
{
	uint32_t bits = jp_kernel_cpu_physical_address_bits();
	size_t i;

	if (bits == 0) {
		return JP_BOOT_FAILED;     /* the width of a physical address is unknown */
	}
	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];

		if (!range_is_valid(region->base_address, region->length)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		if (bits < JP_MEMORY_BITS_UNLIMITED &&
		    region->base_address + region->length > ((uint64_t)1 << bits)) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		if (!insert_region(table, region, owner_of_region_type(region->type))) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
	}
	merge_neighbours(table);
	return JP_BOOT_SUCCESS;
}

/* Pass 2: [base, end) must lie inside STAGE1_RESERVED (or already be `owner`); it is cut out for `owner`. */
static JPBootResult cut_out(JPBootStage2Area *table, uint64_t base, uint64_t end, JPKernelMemoryOwner owner)
{
	bool single;
	uint32_t first;
	uint32_t allowed = JP_MEMORY_OWNER_BIT(JP_KERNEL_MEMORY_STAGE1_RESERVED) | JP_MEMORY_OWNER_BIT(owner);

	if (!scan_range(table, base, end, allowed, &single, &first)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	if (!table_has_room(table, base, end)) {
		return JP_BOOT_FAILED;
	}
	set_owner(table, base, end, owner);
	return JP_BOOT_SUCCESS;
}

/* A range given as base and size, rounded outwards to whole pages. */
static JPBootResult cut_out_rounded(JPBootStage2Area *table, uint64_t base, uint64_t size, JPKernelMemoryOwner owner)
{
	uint64_t end;

	if (!range_is_valid(base, size) || !align_up_page(base + size, &end)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	return cut_out(table, align_down_page(base), end, owner);
}

static JPBootResult cut_out_framebuffer(JPBootContext *context, JPBootStage2Area *table)
{
	uint64_t size;
	uint64_t end;
	uint64_t base;

	if (context->framebuffer.state != JP_DETECTION_STATE_DETECTED) {
		return JP_BOOT_SUCCESS;
	}
	size = (uint64_t)context->framebuffer.pitch * context->framebuffer.height;
	base = context->framebuffer.base_address;
	if (base == 0 || !range_is_valid(base, size) || !align_up_page(base + size, &end)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	if (!range_touches_table(table, align_down_page(base), end)) {
		return JP_BOOT_SUCCESS;     /* device memory outside the map: the table does not own it */
	}
	{
		bool single;
		uint32_t first;

		if (scan_range(table, align_down_page(base), end, JP_MEMORY_OWNER_BIT(JP_KERNEL_MEMORY_FIRMWARE_RESERVED),
			&single, &first)) {
			return JP_BOOT_SUCCESS;     /* inside a region that the firmware lists as its own: it stays the firmware's */
		}
	}
	return cut_out(table, align_down_page(base), end, JP_KERNEL_MEMORY_FRAMEBUFFER);
}

static JPBootResult cut_out_owners(JPBootContext *context, JPBootStage2Area *table)
{
	const JPHandoffInfo *handoff = &context->handoff;
	uint64_t context_address = (uint64_t)(uintptr_t)context;
	JPBootResult result;
	size_t i;

	for (i = 0; i < context->kernel.segment_count && i < JP_MAX_KERNEL_SEGMENTS; i++) {
		result = cut_out_rounded(table, context->kernel.segments[i].physical_address,
			context->kernel.segments[i].memory_size, JP_KERNEL_MEMORY_KERNEL_IMAGE);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
	}
	if (handoff->stack_pointer < JP_BOOT_KERNEL_STACK_SIZE) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	result = cut_out_rounded(table, handoff->stack_pointer - JP_BOOT_KERNEL_STACK_SIZE,
		JP_BOOT_KERNEL_STACK_SIZE, JP_KERNEL_MEMORY_KERNEL_STACK);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	if (handoff->page_table_memory_size == 0 || !is_page_multiple(handoff->page_table_memory_base) ||
	    !is_page_multiple(handoff->page_table_memory_size)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	result = cut_out_rounded(table, handoff->page_table_memory_base, handoff->page_table_memory_size,
		JP_KERNEL_MEMORY_PAGE_TABLES);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = cut_out_framebuffer(context, table);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	return cut_out_rounded(table, context_address, sizeof(JPBootContext), JP_KERNEL_MEMORY_STAGE2_RUNTIME);
}

static bool inputs_are_usable(const JPBootContext *context)
{
	return context->memory_map.state == JP_DETECTION_STATE_DETECTED &&
	       context->memory_map.region_count >= 1 &&
	       context->memory_map.region_count <= JP_MAX_MEMORY_REGIONS &&
	       context->kernel.load_state == JP_KERNEL_LOAD_LOADED &&
	       context->kernel.segment_count >= 1 &&
	       context->kernel.segment_count <= JP_MAX_KERNEL_SEGMENTS;
}

/* ---- Public API ---- */

JPBootResult jp_kernel_memory_initialize(JPBootContext *context)
{
	JPBootStage2Area *table;
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!inputs_are_usable(context)) {
		return JP_BOOT_FAILED;
	}
	table = table_of(context);
	clear_table(table);
	result = build_from_map(context, table);
	if (result == JP_BOOT_SUCCESS) {
		result = cut_out_owners(context, table);
	}
	if (result != JP_BOOT_SUCCESS) {
		clear_table(table);     /* a failed initialization publishes nothing */
	}
	return result;
}

JPBootResult jp_kernel_memory_reserve(JPBootContext *context, uint64_t base_address, uint64_t length,
	JPKernelMemoryOwner owner)
{
	JPBootStage2Area *table;
	bool single;
	uint32_t first;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	table = table_of(context);
	if (table->entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!owner_is_requestable(owner) || !range_is_valid(base_address, length) ||
	    !is_page_multiple(base_address) || !is_page_multiple(length)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!scan_range(table, base_address, base_address + length,
		JP_MEMORY_OWNER_BIT(JP_KERNEL_MEMORY_AVAILABLE), &single, &first) || !table_has_room(table, base_address, base_address + length)) {
		return JP_BOOT_FAILED;
	}
	set_owner(table, base_address, base_address + length, owner);
	return JP_BOOT_SUCCESS;
}

/* The lowest start >= `minimum`, a multiple of `alignment`, with `length` bytes available; false if none. */
static bool find_available(const JPBootStage2Area *table, uint64_t length, uint64_t alignment, uint64_t *found)
{
	size_t i;

	for (i = 0; i < table->entry_count; i++) {
		const JPBootStage2MemoryEntry *entry = &table->entries[i];
		uint64_t start = entry->base_address;
		uint64_t mask = alignment - 1;

		if (entry->owner != (uint32_t)JP_KERNEL_MEMORY_AVAILABLE) {
			continue;
		}
		if (start < JP_BOOT_PROTECTED_LOW_MEMORY) {
			start = JP_BOOT_PROTECTED_LOW_MEMORY;
		}
		if (start > UINT64_MAX - mask) {
			continue;
		}
		start = (start + mask) & ~mask;
		if (start >= entry_end(entry) || length > entry_end(entry) - start) {
			continue;
		}
		*found = start;
		return true;
	}
	return false;
}

JPBootResult jp_kernel_memory_allocate(JPBootContext *context, uint64_t length, uint64_t alignment,
	JPKernelMemoryOwner owner, uint64_t *base_address)
{
	JPBootStage2Area *table;
	uint64_t start;

	if (context == NULL || base_address == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	table = table_of(context);
	if (table->entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!owner_is_requestable(owner) || length == 0 || !is_page_multiple(length) ||
	    alignment < JP_BOOT_PAGE_SIZE || (alignment & (alignment - 1)) != 0) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!find_available(table, length, alignment, &start) || !table_has_room(table, start, start + length)) {
		return JP_BOOT_FAILED;
	}
	set_owner(table, start, start + length, owner);
	*base_address = start;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_kernel_memory_query(const JPBootContext *context, uint64_t base_address, uint64_t length,
	JPKernelMemoryOwner *owner)
{
	const JPBootStage2Area *table;
	bool single;
	uint32_t first;

	if (context == NULL || owner == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	table = &context->stage2;
	if (table->entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!range_is_valid(base_address, length)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!scan_range(table, base_address, base_address + length, JP_MEMORY_ALL_OWNERS, &single, &first) ||
	    !single) {
		return JP_BOOT_NOT_DETECTED;
	}
	*owner = (JPKernelMemoryOwner)first;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_kernel_memory_release(JPBootContext *context, uint64_t base_address, uint64_t length,
	JPKernelMemoryOwner owner)
{
	JPBootStage2Area *table;
	bool single;
	uint32_t first;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	table = table_of(context);
	if (table->entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!owner_is_valid((uint32_t)owner) || !range_is_valid(base_address, length) ||
	    !is_page_multiple(base_address) || !is_page_multiple(length)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (owner != JP_KERNEL_MEMORY_EARLY_HEAP ||
	    !scan_range(table, base_address, base_address + length,
		JP_MEMORY_OWNER_BIT(JP_KERNEL_MEMORY_EARLY_HEAP), &single, &first) ||
	    !table_has_room(table, base_address, base_address + length)) {
		return JP_BOOT_FAILED;
	}
	set_owner(table, base_address, base_address + length, JP_KERNEL_MEMORY_AVAILABLE);
	return JP_BOOT_SUCCESS;
}

/* ---- validate_ownership ---- */

/* True when [base, end) is covered without a hole by regions of the Stage 1 map of the given type (any type if `any`). */
static bool map_covers(const JPMemoryMap *map, uint64_t base, uint64_t end, bool any, JPMemoryRegionType type)
{
	uint64_t position = base;

	while (position < end) {
		bool moved = false;
		size_t i;

		for (i = 0; i < map->region_count; i++) {
			const JPMemoryRegion *region = &map->regions[i];

			if ((any || region->type == type) && region->base_address <= position &&
			    region->length <= UINT64_MAX - region->base_address &&
			    position < region->base_address + region->length) {
				position = region->base_address + region->length;
				moved = true;
				break;
			}
		}
		if (!moved) {
			return false;
		}
	}
	return true;
}

static bool entries_are_well_formed(const JPBootStage2Area *table)
{
	size_t i;

	for (i = 0; i < table->entry_count; i++) {
		const JPBootStage2MemoryEntry *entry = &table->entries[i];

		if (!range_is_valid(entry->base_address, entry->length) || !owner_is_valid(entry->owner)) {
			return false;
		}
		if (i > 0) {
			const JPBootStage2MemoryEntry *previous = &table->entries[i - 1];

			if (entry_end(previous) > entry->base_address) {
				return false;     /* out of order or overlapping */
			}
			if (entry_end(previous) == entry->base_address && previous->owner == entry->owner) {
				return false;     /* touching neighbours with one owner must be one entry */
			}
		}
	}
	return true;
}

static bool table_agrees_with_map(const JPBootStage2Area *table, const JPMemoryMap *map)
{
	uint64_t entries_total = 0;
	uint64_t regions_total = 0;
	size_t i;

	for (i = 0; i < table->entry_count; i++) {
		const JPBootStage2MemoryEntry *entry = &table->entries[i];

		if (entry->owner == (uint32_t)JP_KERNEL_MEMORY_AVAILABLE ||
		    entry->owner == (uint32_t)JP_KERNEL_MEMORY_EARLY_HEAP) {
			if (!map_covers(map, entry->base_address, entry_end(entry), false, JP_MEMORY_REGION_USABLE)) {
				return false;
			}
		}
		entries_total += entry->length;
	}
	for (i = 0; i < map->region_count; i++) {
		const JPMemoryRegion *region = &map->regions[i];
		bool single;
		uint32_t first;

		if (!range_is_valid(region->base_address, region->length) ||
		    !scan_range(table, region->base_address, region->base_address + region->length,
			JP_MEMORY_ALL_OWNERS, &single, &first)) {
			return false;     /* a region that the table does not cover */
		}
		regions_total += region->length;
	}
	return entries_total == regions_total;
}

JPBootResult jp_kernel_memory_validate_ownership(const JPBootContext *context)
{
	const JPBootStage2Area *table;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	table = &context->stage2;
	if (table->entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (table->entry_count > JP_KERNEL_MEMORY_MAX_ENTRIES || context->memory_map.region_count > JP_MAX_MEMORY_REGIONS ||
	    !entries_are_well_formed(table) || !table_agrees_with_map(table, &context->memory_map)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	return JP_BOOT_SUCCESS;
}
