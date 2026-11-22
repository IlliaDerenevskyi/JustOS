/*
 * memory_map.c - memory map acquisition and validation.
 *
 * Purpose:
 *   Gets the normalized memory map through the boot environment and checks
 *   that it can be trusted.
 *
 * Responsibilities:
 *   - jp_boot_discover_memory(): request the map, check the region count,
 *     publish the state (DETECTED only after real acquisition);
 *   - jp_boot_validate_memory_map(): check every region, overflow, overlap,
 *     and that usable memory exists. It never merges, reorders, or changes
 *     regions.
 *
 * Limitations:
 *   The state of the map is NOT_INITIALIZED before acquisition, DETECTED after
 *   it, and FAILED after a failed acquisition or validation; "validated" is the
 *   successful result of the validation call (JPDetectionState has no separate
 *   value for it).
 *
 * Depends on:
 *   justos/boot/memory_map.h, justos/boot/boot_environment.h.
 *
 * Must not depend on:
 *   Provider headers and provider-native memory descriptors.
 */

#include "justos/boot/memory_map.h"

#include "justos/boot/boot_environment.h"

/* ---- Public API ---- */

JPBootResult jp_boot_discover_memory(JPBootContext *context);
JPBootResult jp_boot_validate_memory_map(JPBootContext *context);

/* ---- Private helpers ---- */

static bool validate_memory_region(const JPMemoryRegion *region)
{
	int type = (int)region->type;

	if (region->length == 0) {
		return false;
	}
	if (type <= (int)JP_MEMORY_REGION_UNKNOWN || type > (int)JP_MEMORY_REGION_UNUSABLE) {
		return false;
	}
	return true;
}

static bool validate_memory_overflow(const JPMemoryRegion *region)
{
	return region->length <= UINT64_MAX - region->base_address;
}

/* Two regions overlap when each begins before the other ends. Touching
 * regions (one ends exactly where the next begins) do not overlap. The caller
 * has already checked that base + length does not overflow. */
static bool regions_overlap(const JPMemoryRegion *a, const JPMemoryRegion *b)
{
	return a->base_address < b->base_address + b->length &&
	       b->base_address < a->base_address + a->length;
}

static bool validate_memory_overlap(const JPMemoryMap *map)
{
	size_t i;
	size_t j;

	for (i = 0; i < map->region_count; i++) {
		for (j = i + 1; j < map->region_count; j++) {
			if (regions_overlap(&map->regions[i], &map->regions[j])) {
				return false;
			}
		}
	}
	return true;
}

static bool validate_memory_layout(const JPMemoryMap *map)
{
	bool usable_found = false;
	size_t i;

	if (map->region_count == 0 || map->region_count > JP_MAX_MEMORY_REGIONS) {
		return false;
	}
	for (i = 0; i < map->region_count; i++) {
		if (!validate_memory_region(&map->regions[i]) ||
		    !validate_memory_overflow(&map->regions[i])) {
			return false;
		}
		if (map->regions[i].type == JP_MEMORY_REGION_USABLE) {
			usable_found = true;
		}
	}
	return usable_found && validate_memory_overlap(map);
}

/* ---- Implementation ---- */

JPBootResult jp_boot_discover_memory(JPBootContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	context->current_stage = JP_BOOT_STAGE_MEMORY_DISCOVERY;

	result = jp_boot_environment_get_memory_map(context);
	if (result != JP_BOOT_SUCCESS) {
		context->memory_map.state = JP_DETECTION_STATE_FAILED;
		return result;
	}
	if (context->memory_map.region_count > JP_MAX_MEMORY_REGIONS) {
		context->memory_map.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	context->memory_map.state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_validate_memory_map(JPBootContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->memory_map.state != JP_DETECTION_STATE_DETECTED) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (!validate_memory_layout(&context->memory_map)) {
		context->memory_map.state = JP_DETECTION_STATE_FAILED;
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	return JP_BOOT_SUCCESS;
}
