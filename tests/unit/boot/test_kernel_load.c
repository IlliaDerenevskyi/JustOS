/*
 * Unit test (level U) of the loading half of src/boot/kernel_loader.c.
 * Rules: see "Decided rules (physical memory, copying, mapping)" in
 * milestone0.md. Physical memory is a test buffer reached through the fake
 * architecture's phys_access; mapping is the fake map_segment.
 */

#include <assert.h>
#include <string.h>

#include "elf_builder.h"
#include "fake_architecture.h"
#include "fake_environment.h"
#include "justos/boot/kernel_loader.h"

#define PAGE 4096ULL
#define MIB 0x100000ULL
#define MEMORY_SIZE (4 * MIB)
#define IMAGE_CAPACITY 131072
#define FILL 0xEE
#define PF_X 1U
#define PF_R 4U
#define PF_W 2U
#define ENTRY 0xFFFFFFFF80000000ULL

static uint8_t physical_memory[MEMORY_SIZE];
static uint8_t image[IMAGE_CAPACITY];
static JPFakeEnvironment environment_fake;

/*
 * Builds a validated-looking kernel in the context from spec (the loader
 * trusts a VALID kernel; validation has its own test) and a memory map with
 * one usable region [usable_base, usable_base + usable_length).
 */
static void prepare(JPBootContext *context, const JPTestElfSpec *spec,
	uint64_t usable_base, uint64_t usable_length)
{
	size_t i;
	size_t size = jp_test_elf_build(spec, image, sizeof image);

	assert(size != 0);
	memset(context, 0, sizeof *context);
	memset(&environment_fake, 0, sizeof environment_fake);
	context->environment.platform_context = &environment_fake;
	memset(physical_memory, FILL, sizeof physical_memory);
	jp_fake_arch_reset();
	jp_fake_arch.phys_memory = physical_memory;
	jp_fake_arch.phys_base = MIB;
	jp_fake_arch.phys_size = MEMORY_SIZE;

	context->handoff.architecture = JP_ARCH_X86_64;
	context->kernel.image.state = JP_DETECTION_STATE_DETECTED;
	context->kernel.image.buffer = image;
	context->kernel.image.size = size;
	context->kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	context->kernel.load_state = JP_KERNEL_LOAD_NOT_INITIALIZED;
	context->kernel.entry_point = spec->entry_point;
	context->kernel.segment_count = spec->segment_count;
	for (i = 0; i < spec->segment_count; i++) {
		context->kernel.segments[i].virtual_address = spec->segments[i].virtual_address;
		context->kernel.segments[i].file_offset = spec->segments[i].file_offset;
		context->kernel.segments[i].file_size = spec->segments[i].file_size;
		context->kernel.segments[i].memory_size = spec->segments[i].memory_size;
		context->kernel.segments[i].flags = spec->segments[i].flags;
		context->kernel.segments[i].state = JP_KERNEL_SEGMENT_VALIDATED;
	}

	context->memory_map.state = JP_DETECTION_STATE_DETECTED;
	context->memory_map.region_count = 1;
	context->memory_map.regions[0].base_address = usable_base;
	context->memory_map.regions[0].length = usable_length;
	context->memory_map.regions[0].type = JP_MEMORY_REGION_USABLE;
}

static const uint8_t *phys(uint64_t address)
{
	return physical_memory + (address - MIB);
}

static void test_single_segment_is_placed_copied_zero_filled_and_mapped(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	uint64_t at;
	size_t i;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].file_size = 16;
	spec.segments[0].memory_size = 0x1800;
	prepare(&context, &spec, MIB, 3 * MIB);

	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.current_stage == JP_BOOT_STAGE_KERNEL_LOADING);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_LOADED);
	at = context.kernel.segments[0].physical_address;
	assert(at == MIB);
	for (i = 0; i < 16; i++) {
		assert(phys(at)[i] == 1);          /* copied from the file */
	}
	for (i = 16; i < 0x1800; i++) {
		assert(phys(at)[i] == 0);          /* zero-filled tail */
	}
	assert(phys(at)[0x1800] == FILL);         /* nothing written beyond */
	assert(jp_fake_arch.map_calls == 1 && jp_fake_arch.mapped_indexes[0] == 0);
	assert(jp_fake_arch.states_at_phys_access[0] == JP_KERNEL_SEGMENT_ALLOCATED);   /* placed before copying */
	assert(jp_fake_arch.state_at_map[0] == JP_KERNEL_SEGMENT_LOADED);              /* copied before mapping */
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_PERMISSIONED);
}

static void test_segments_follow_each_other_page_aligned(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segment_count = 3;
	spec.segments[0].memory_size = 0x1800;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = PF_R;
	spec.segments[1].virtual_address = ENTRY + 0x10000;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.segments[1].file_size = 0;          /* a segment that is only zeros */
	spec.segments[1].memory_size = 0x1000;
	spec.segments[2] = spec.segments[0];
	spec.segments[2].flags = PF_R | PF_W;
	spec.segments[2].virtual_address = ENTRY + 0x20000;
	spec.segments[2].file_offset = 3 * PAGE;
	spec.segments[2].memory_size = 0x10;
	prepare(&context, &spec, MIB, 3 * MIB);

	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segments[0].physical_address == MIB);
	assert(context.kernel.segments[1].physical_address == MIB + 0x2000);
	assert(context.kernel.segments[2].physical_address == MIB + 0x3000);
	assert(jp_fake_arch.map_calls == 3);
	assert(jp_fake_arch.mapped_indexes[0] == 0);
	assert(jp_fake_arch.mapped_indexes[1] == 1);
	assert(jp_fake_arch.mapped_indexes[2] == 2);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_PERMISSIONED);
	assert(context.kernel.segments[1].state == JP_KERNEL_SEGMENT_PERMISSIONED);
	assert(context.kernel.segments[2].state == JP_KERNEL_SEGMENT_PERMISSIONED);
	assert(phys(MIB + 0x2000)[0] == 0 && phys(MIB + 0x2000)[0xFFF] == 0);
	assert(phys(MIB + 0x3000)[0] == 3);
}

static void test_placement_skips_low_memory_and_unusable_regions(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].memory_size = 0x2000;
	prepare(&context, &spec, 0x1000, 8 * MIB);

	/* A usable region that starts below 1 MiB: the segment goes above 1 MiB. */
	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segments[0].physical_address == MIB);

	/* A reserved region first, a too small usable one, then a fitting one. */
	prepare(&context, &spec, MIB, 0x1000);
	context.memory_map.region_count = 3;
	context.memory_map.regions[1] = context.memory_map.regions[0];
	context.memory_map.regions[0].base_address = 0x400000;
	context.memory_map.regions[0].length = MIB;
	context.memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	context.memory_map.regions[2].base_address = 0x300000;
	context.memory_map.regions[2].length = MIB;
	context.memory_map.regions[2].type = JP_MEMORY_REGION_USABLE;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segments[0].physical_address == 0x300000);
}

static void test_placement_avoids_the_kernel_image(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	uint64_t image_start = (uint64_t)(uintptr_t)image;
	uint64_t region_base = image_start & ~(PAGE - 1);
	uint64_t image_end;

	jp_test_elf_default_spec(&spec);
	prepare(&context, &spec, region_base, 8 * MIB);
	jp_fake_arch.phys_base = region_base;      /* the test buffer stands for this region */
	image_end = image_start + context.kernel.image.size;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segments[0].physical_address >= image_end);
	assert(context.kernel.segments[0].physical_address % PAGE == 0);
}

static void test_no_room_fails(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].memory_size = 0x2000;
	prepare(&context, &spec, MIB, 0x1000);
	assert(jp_boot_load_kernel(&context) == JP_BOOT_FAILED);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_FAILED);
	assert(jp_fake_arch.map_calls == 0);

	/* No usable region at all. */
	prepare(&context, &spec, MIB, 3 * MIB);
	context.memory_map.regions[0].type = JP_MEMORY_REGION_RESERVED;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_FAILED);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
}

static void test_unreachable_physical_memory_fails(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	prepare(&context, &spec, MIB, 3 * MIB);
	jp_fake_arch.phys_memory = NULL;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_FAILED);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_FAILED);
	assert(jp_fake_arch.map_calls == 0);
}

static void test_mapping_failure_stops_loading_and_is_propagated(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segment_count = 3;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].virtual_address = ENTRY + 0x10000;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.segments[1].flags = PF_R;
	spec.segments[2] = spec.segments[1];
	spec.segments[2].virtual_address = ENTRY + 0x20000;
	spec.segments[2].file_offset = 3 * PAGE;
	prepare(&context, &spec, MIB, 3 * MIB);
	jp_fake_arch.map_result = JP_BOOT_ERROR_CORRUPTED_DATA;
	jp_fake_arch.map_fail_index = 1;

	assert(jp_boot_load_kernel(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
	assert(jp_fake_arch.map_calls == 2);          /* segment 2 was never handled */
	assert(context.kernel.segments[2].physical_address == 0);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_PERMISSIONED);
	assert(context.kernel.segments[1].state == JP_KERNEL_SEGMENT_FAILED);   /* distinct from every success state */
	assert(context.kernel.segments[2].state == JP_KERNEL_SEGMENT_VALIDATED);
}

/* A usable region that ends below the protected low memory cannot hold the kernel. */
static void test_region_below_the_protected_limit_is_unusable(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].memory_size = 0x1000;
	prepare(&context, &spec, 0x1000, 0x2000);       /* [0x1000, 0x3000) */
	assert(jp_boot_load_kernel(&context) == JP_BOOT_FAILED);
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
	assert(jp_fake_arch.map_calls == 0);
}

/* A segment that is not VALIDATED is not handled, and nothing changes. */
/* Every segment is reserved right after it is placed and before any byte is copied. */
static void test_segments_are_reserved(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	size_t i;

	jp_test_elf_default_spec(&spec);
	spec.segment_count = 3;
	spec.segments[0].memory_size = 0x1800;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = PF_R;
	spec.segments[1].virtual_address = ENTRY + 0x10000;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.segments[1].memory_size = 0x1000;
	spec.segments[2] = spec.segments[1];
	spec.segments[2].virtual_address = ENTRY + 0x20000;
	spec.segments[2].file_offset = 3 * PAGE;
	prepare(&context, &spec, MIB, 3 * MIB);

	assert(jp_boot_load_kernel(&context) == JP_BOOT_SUCCESS);
	assert(environment_fake.reserve_calls == 3);
	for (i = 0; i < 3; i++) {
		assert(environment_fake.reserved_base[i] == context.kernel.segments[i].physical_address);
		assert(environment_fake.reserved_length[i] == context.kernel.segments[i].memory_size);
		/* The reservation is part of step 1: the segment is not yet ALLOCATED. */
		assert(environment_fake.segment_state_at_reserve[i] == JP_KERNEL_SEGMENT_VALIDATED);
	}
}

/* A failing reservation stops the load before anything is copied or mapped. */
static void test_reservation_failure_stops_the_load(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segment_count = 2;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].virtual_address = ENTRY + 0x10000;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.segments[1].flags = PF_R;
	prepare(&context, &spec, MIB, 3 * MIB);
	environment_fake.reserve_result = JP_BOOT_ERROR_CORRUPTED_DATA;

	assert(jp_boot_load_kernel(&context) == JP_BOOT_FAILED);   /* not the provider's code */
	assert(context.kernel.load_state == JP_KERNEL_LOAD_FAILED);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_FAILED);
	assert(context.kernel.segments[1].state == JP_KERNEL_SEGMENT_VALIDATED);
	assert(environment_fake.reserve_calls == 1);
	assert(jp_fake_arch.map_calls == 0);
	assert(phys(MIB)[0] == FILL);                               /* nothing was copied */
}

static void test_segment_must_be_validated(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	JPKernelSegmentState states[] = {
		JP_KERNEL_SEGMENT_UNINITIALIZED, JP_KERNEL_SEGMENT_ALLOCATED,
		JP_KERNEL_SEGMENT_LOADED, JP_KERNEL_SEGMENT_PERMISSIONED,
		JP_KERNEL_SEGMENT_FAILED
	};
	size_t i;

	jp_test_elf_default_spec(&spec);
	for (i = 0; i < sizeof states / sizeof states[0]; i++) {
		prepare(&context, &spec, MIB, 3 * MIB);
		context.kernel.segments[0].state = states[i];
		assert(jp_boot_load_kernel(&context) == JP_BOOT_NOT_INITIALIZED);
		assert(context.kernel.load_state != JP_KERNEL_LOAD_LOADED);
		assert(context.kernel.segments[0].state == states[i]);
		assert(context.kernel.segments[0].physical_address == 0);
		assert(jp_fake_arch.map_calls == 0);
	}
}

static void test_parameters_and_preconditions(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	assert(jp_boot_load_kernel(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context, &spec, MIB, 3 * MIB);
	context.handoff.architecture = JP_ARCH_UNKNOWN;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_ERROR_INVALID_PARAM);

	prepare(&context, &spec, MIB, 3 * MIB);
	context.kernel.validation_state = JP_KERNEL_VALIDATION_INVALID;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(context.kernel.load_state != JP_KERNEL_LOAD_LOADED);

	prepare(&context, &spec, MIB, 3 * MIB);
	context.memory_map.state = JP_DETECTION_STATE_UNKNOWN;
	assert(jp_boot_load_kernel(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_fake_arch.map_calls == 0);
}

int main(void)
{
	test_single_segment_is_placed_copied_zero_filled_and_mapped();
	test_segments_follow_each_other_page_aligned();
	test_placement_skips_low_memory_and_unusable_regions();
	test_placement_avoids_the_kernel_image();
	test_no_room_fails();
	test_unreachable_physical_memory_fails();
	test_mapping_failure_stops_loading_and_is_propagated();
	test_region_below_the_protected_limit_is_unusable();
	test_segments_are_reserved();
	test_reservation_failure_stops_the_load();
	test_segment_must_be_validated();
	test_parameters_and_preconditions();
	return 0;
}
