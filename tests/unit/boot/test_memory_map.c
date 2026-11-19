/*
 * Unit test (level U) of src/boot/memory_map.c.
 * Rules: see "Decided rules (memory map)" in milestone0.md.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "fake_environment.h"
#include "justos/boot/memory_map.h"

#define PAGE 0x1000ULL

static void set_region(JPMemoryMap *map, size_t index, uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	map->regions[index].base_address = base;
	map->regions[index].length = length;
	map->regions[index].type = type;
}

/* A valid two-region map placed in the fake provider. */
static void prepare(JPBootContext *context, JPFakeEnvironment *fake)
{
	jp_fake_environment_attach(context, fake);
	fake->memory_map.region_count = 2;
	set_region(&fake->memory_map, 0, 0x0, 0x100000, JP_MEMORY_REGION_RESERVED);
	set_region(&fake->memory_map, 1, 0x100000, 0x10000000, JP_MEMORY_REGION_USABLE);
}

/* The same map, already acquired (state DETECTED), for validation tests. */
static void prepare_acquired(JPBootContext *context, JPFakeEnvironment *fake)
{
	prepare(context, fake);
	assert(jp_boot_discover_memory(context) == JP_BOOT_SUCCESS);
}

static void test_discover_success(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	assert(jp_boot_discover_memory(&context) == JP_BOOT_SUCCESS);
	assert(fake.memory_map_calls == 1);
	assert(context.current_stage == JP_BOOT_STAGE_MEMORY_DISCOVERY);
	assert(context.memory_map.state == JP_DETECTION_STATE_DETECTED);
	assert(context.memory_map.region_count == 2);
	assert(context.memory_map.regions[0].type == JP_MEMORY_REGION_RESERVED);
	assert(context.memory_map.regions[1].length == 0x10000000);
}

static void test_discover_keeps_provider_order(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare(&context, &fake);
	set_region(&fake.memory_map, 0, 0x200000, PAGE, JP_MEMORY_REGION_USABLE);
	set_region(&fake.memory_map, 1, 0x100000, PAGE, JP_MEMORY_REGION_USABLE);
	assert(jp_boot_discover_memory(&context) == JP_BOOT_SUCCESS);
	assert(context.memory_map.regions[0].base_address == 0x200000);
	assert(context.memory_map.regions[1].base_address == 0x100000);
}

static void test_discover_failures(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	assert(jp_boot_discover_memory(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context, &fake);
	fake.memory_map_result = JP_BOOT_FAILED;
	assert(jp_boot_discover_memory(&context) == JP_BOOT_FAILED);
	assert(context.memory_map.state == JP_DETECTION_STATE_FAILED);

	prepare(&context, &fake);
	fake.memory_map_result = JP_BOOT_NOT_DETECTED;
	assert(jp_boot_discover_memory(&context) == JP_BOOT_NOT_DETECTED);
	assert(context.memory_map.state == JP_DETECTION_STATE_FAILED);

	prepare(&context, &fake);
	fake.memory_map.region_count = JP_MAX_MEMORY_REGIONS + 1;
	assert(jp_boot_discover_memory(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.memory_map.state == JP_DETECTION_STATE_FAILED);
}

static void test_validate_requires_acquisition(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	assert(jp_boot_validate_memory_map(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	prepare(&context, &fake);
	assert(jp_boot_validate_memory_map(&context) == JP_BOOT_NOT_INITIALIZED);
}

static void test_validate_success_keeps_map_unchanged(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;
	JPMemoryMap before;

	prepare_acquired(&context, &fake);
	before = context.memory_map;
	assert(jp_boot_validate_memory_map(&context) == JP_BOOT_SUCCESS);
	assert(context.memory_map.state == JP_DETECTION_STATE_DETECTED);
	assert(memcmp(&before, &context.memory_map, sizeof before) == 0);
}

static void test_touching_regions_are_allowed(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare_acquired(&context, &fake);
	context.memory_map.region_count = 3;
	set_region(&context.memory_map, 0, 0x1000, 0x1000, JP_MEMORY_REGION_USABLE);
	set_region(&context.memory_map, 1, 0x2000, 0x1000, JP_MEMORY_REGION_ACPI);
	set_region(&context.memory_map, 2, 0x3000, 0x1000, JP_MEMORY_REGION_MMIO);
	assert(jp_boot_validate_memory_map(&context) == JP_BOOT_SUCCESS);
}

/* Touching regions are allowed in any order of the table. */
static void test_touching_regions_in_descending_order_are_allowed(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	prepare_acquired(&context, &fake);
	context.memory_map.region_count = 3;
	set_region(&context.memory_map, 0, 0x3000, 0x1000, JP_MEMORY_REGION_MMIO);
	set_region(&context.memory_map, 1, 0x2000, 0x1000, JP_MEMORY_REGION_ACPI);
	set_region(&context.memory_map, 2, 0x1000, 0x1000, JP_MEMORY_REGION_USABLE);
	assert(jp_boot_validate_memory_map(&context) == JP_BOOT_SUCCESS);
}

#define EXPECT_REJECTED(context) \
	do { \
		assert(jp_boot_validate_memory_map(&(context)) == JP_BOOT_ERROR_CORRUPTED_DATA); \
		assert((context).memory_map.state == JP_DETECTION_STATE_FAILED); \
	} while (0)

static void test_validate_rejections(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;

	/* No regions. */
	prepare_acquired(&context, &fake);
	context.memory_map.region_count = 0;
	EXPECT_REJECTED(context);

	/* More regions than the capacity. */
	prepare_acquired(&context, &fake);
	context.memory_map.region_count = JP_MAX_MEMORY_REGIONS + 1;
	EXPECT_REJECTED(context);

	/* Zero length. */
	prepare_acquired(&context, &fake);
	context.memory_map.regions[0].length = 0;
	EXPECT_REJECTED(context);

	/* Unknown type and a value outside the enumeration. */
	prepare_acquired(&context, &fake);
	context.memory_map.regions[0].type = JP_MEMORY_REGION_UNKNOWN;
	EXPECT_REJECTED(context);
	prepare_acquired(&context, &fake);
	context.memory_map.regions[0].type = (JPMemoryRegionType)99;
	EXPECT_REJECTED(context);

	/* base + length overflows. */
	prepare_acquired(&context, &fake);
	set_region(&context.memory_map, 0, UINT64_MAX - 10, 100, JP_MEMORY_REGION_RESERVED);
	EXPECT_REJECTED(context);

	/* Overlap, neighbours and out of order. */
	prepare_acquired(&context, &fake);
	set_region(&context.memory_map, 0, 0x100000, 0x2000, JP_MEMORY_REGION_USABLE);
	set_region(&context.memory_map, 1, 0x101000, 0x2000, JP_MEMORY_REGION_RESERVED);
	EXPECT_REJECTED(context);
	prepare_acquired(&context, &fake);
	set_region(&context.memory_map, 0, 0x102000, 0x1000, JP_MEMORY_REGION_USABLE);
	set_region(&context.memory_map, 1, 0x101000, 0x2000, JP_MEMORY_REGION_RESERVED);
	EXPECT_REJECTED(context);
	/* One region fully inside another. */
	prepare_acquired(&context, &fake);
	set_region(&context.memory_map, 0, 0x100000, 0x10000, JP_MEMORY_REGION_USABLE);
	set_region(&context.memory_map, 1, 0x104000, 0x1000, JP_MEMORY_REGION_RESERVED);
	EXPECT_REJECTED(context);

	/* No usable memory. */
	prepare_acquired(&context, &fake);
	context.memory_map.regions[1].type = JP_MEMORY_REGION_RESERVED;
	EXPECT_REJECTED(context);
}

static void test_maximum_region_count_is_valid(void)
{
	JPBootContext context;
	JPFakeEnvironment fake;
	size_t i;

	prepare(&context, &fake);
	fake.memory_map.region_count = JP_MAX_MEMORY_REGIONS;
	for (i = 0; i < JP_MAX_MEMORY_REGIONS; i++) {
		set_region(&fake.memory_map, i, (uint64_t)(i + 1) * PAGE, PAGE, JP_MEMORY_REGION_USABLE);
	}
	assert(jp_boot_discover_memory(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_validate_memory_map(&context) == JP_BOOT_SUCCESS);
	assert(context.memory_map.region_count == JP_MAX_MEMORY_REGIONS);
}

int main(void)
{
	test_discover_success();
	test_discover_keeps_provider_order();
	test_discover_failures();
	test_validate_requires_acquisition();
	test_validate_success_keeps_map_unchanged();
	test_touching_regions_are_allowed();
	test_touching_regions_in_descending_order_are_allowed();
	test_validate_rejections();
	test_maximum_region_count_is_valid();
	return 0;
}
