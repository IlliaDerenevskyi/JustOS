/*
 * Unit test (level U) of the validation half of src/boot/kernel_loader.c.
 * Rules: see "Decided rules (ELF validation)" in milestone0.md.
 * Images are produced by the test-support ELF builder; the architecture
 * operations are the fake ones (the machine check is delegated to them).
 */

#include <assert.h>
#include <string.h>

#include "elf_builder.h"
#include "fake_architecture.h"
#include "justos/boot/kernel_loader.h"

#define IMAGE_CAPACITY 131072
#define PAGE 4096ULL
#define PF_X 1U
#define PF_W 2U
#define PF_R 4U
#define PT_NOTE 4U
#define ENTRY 0xFFFFFFFF80000000ULL

static uint8_t image[IMAGE_CAPACITY];
static size_t image_size;

static void poke16(size_t offset, uint16_t v)
{
	image[offset] = (uint8_t)(v & 0xFFU);
	image[offset + 1] = (uint8_t)(v >> 8);
}

static void poke64(size_t offset, uint64_t v)
{
	size_t i;

	for (i = 0; i < 8; i++) {
		image[offset + i] = (uint8_t)((v >> (8U * i)) & 0xFFU);
	}
}

/* Builds the image from spec and connects it to a fresh context. */
static void build(JPBootContext *context, const JPTestElfSpec *spec, JPArchitectureType arch)
{
	memset(context, 0, sizeof *context);
	jp_fake_arch_reset();
	image_size = jp_test_elf_build(spec, image, sizeof image);
	assert(image_size != 0);
	context->handoff.architecture = arch;
	context->kernel.image.state = JP_DETECTION_STATE_DETECTED;
	context->kernel.image.buffer = image;
	context->kernel.image.size = image_size;
	context->kernel.validation_state = JP_KERNEL_VALIDATION_NOT_INITIALIZED;
}

static void expect_invalid(JPBootContext *context)
{
	assert(jp_boot_validate_kernel(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context->kernel.validation_state == JP_KERNEL_VALIDATION_INVALID);
	assert(context->kernel.segment_count == 0);
}

static void test_valid_kernel_is_accepted_and_described(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.current_stage == JP_BOOT_STAGE_KERNEL_VALIDATION);
	assert(context.kernel.validation_state == JP_KERNEL_VALIDATION_VALID);
	assert(context.kernel.header.magic == JP_ELF_MAGIC);
	assert(context.kernel.header.elf_class == JP_ELF_CLASS_64);
	assert(context.kernel.header.machine == JP_ELF_MACHINE_X86_64);
	assert(context.kernel.header.type == JP_ELF_TYPE_EXEC);
	assert(context.kernel.header.program_header_count == 1);
	assert(context.kernel.entry_point == ENTRY);
	assert(context.kernel.segment_count == 1);
	assert(context.kernel.segments[0].virtual_address == ENTRY);
	assert(context.kernel.segments[0].file_offset == PAGE);
	assert(context.kernel.segments[0].file_size == spec.segments[0].file_size);
	assert(context.kernel.segments[0].memory_size == spec.segments[0].memory_size);
	assert(context.kernel.segments[0].flags == (PF_R | PF_X));
	assert(context.kernel.segments[0].physical_address == 0);
	assert(context.kernel.segments[0].state == JP_KERNEL_SEGMENT_VALIDATED);
	assert(jp_fake_arch.validate_machine_calls == 1);
}

static void test_aarch64_kernel_is_accepted_only_on_aarch64(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.machine = JP_ELF_MACHINE_AARCH64;
	build(&context, &spec, JP_ARCH_AARCH64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);

	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

static void test_wrong_machine_is_rejected(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.machine = JP_ELF_MACHINE_AARCH64;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

static void test_parameters_and_state(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	assert(jp_boot_validate_kernel(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	build(&context, &spec, JP_ARCH_UNKNOWN);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_ERROR_INVALID_PARAM);

	build(&context, &spec, JP_ARCH_X86_64);
	context.kernel.image.state = JP_DETECTION_STATE_NOT_INITIALIZED;
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(context.kernel.validation_state != JP_KERNEL_VALIDATION_VALID);
}

static void test_header_rejections(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.magic = 0x12345678;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	jp_test_elf_default_spec(&spec);
	spec.elf_class = 1;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	jp_test_elf_default_spec(&spec);
	spec.type = JP_ELF_TYPE_DYN;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	jp_test_elf_default_spec(&spec);
	spec.type = 1;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* The image is shorter than the ELF header. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	context.kernel.image.size = JP_TEST_ELF_HEADER_SIZE - 1;
	expect_invalid(&context);
}

static void test_program_header_table_rejections(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.phdr_entry_size = JP_TEST_ELF_PHDR_SIZE - 8;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* The table starts beyond the image. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke64(32, image_size);
	expect_invalid(&context);

	/* The table runs past the end of the image. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke16(56, 0xFFFF);
	expect_invalid(&context);

	/* offset + count * size overflows. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke64(32, UINT64_MAX - 8);
	expect_invalid(&context);
}

static void test_load_segment_selection(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	size_t i;

	/* No PT_LOAD at all. */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].type = PT_NOTE;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* Non-load entries are ignored. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = 2;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].type = PT_NOTE;
	spec.segments[1].file_size = 1u << 30; /* would be invalid if it were checked */
	build(&context, &spec, JP_ARCH_X86_64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segment_count == 1);

	/* Exactly the maximum number of loadable segments is accepted. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = JP_MAX_KERNEL_SEGMENTS;
	for (i = 0; i < JP_MAX_KERNEL_SEGMENTS; i++) {
		spec.segments[i] = spec.segments[0];
		spec.segments[i].virtual_address = ENTRY + i * PAGE;
		spec.segments[i].file_offset = (i + 1) * PAGE;
		spec.segments[i].file_size = 16;
		spec.segments[i].memory_size = 16;
	}
	build(&context, &spec, JP_ARCH_X86_64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segment_count == JP_MAX_KERNEL_SEGMENTS);
	for (i = 0; i < JP_MAX_KERNEL_SEGMENTS; i++) {
		assert(context.kernel.segments[i].state == JP_KERNEL_SEGMENT_VALIDATED);
	}

	/* One more is rejected. */
	spec.segment_count = JP_MAX_KERNEL_SEGMENTS + 1;
	spec.segments[JP_MAX_KERNEL_SEGMENTS] = spec.segments[0];
	spec.segments[JP_MAX_KERNEL_SEGMENTS].virtual_address = ENTRY + JP_MAX_KERNEL_SEGMENTS * PAGE;
	spec.segments[JP_MAX_KERNEL_SEGMENTS].file_offset = (JP_MAX_KERNEL_SEGMENTS + 1) * PAGE;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

static void test_segment_range_rejections(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* file_size > memory_size */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].file_size = 64;
	spec.segments[0].memory_size = 32;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* file range beyond the image (program header 0 starts at byte 64) */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke64(64 + 32, image_size);
	poke64(64 + 40, image_size);
	expect_invalid(&context);
}

static void test_segment_file_offset_overflow(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].file_offset = UINT64_MAX - 4095;
	spec.segments[0].file_size = 0x2000;
	spec.segments[0].memory_size = 0x2000;
	build(&context, &spec, JP_ARCH_X86_64);
	/* file_offset + file_size wraps around; alignment and sizes are fine. */
	expect_invalid(&context);
}

static void test_virtual_range_overflow(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].virtual_address = 0xFFFFFFFFFFFFF000ULL;
	spec.segments[0].memory_size = 0x2000;
	spec.entry_point = 0xFFFFFFFFFFFFF000ULL;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

static void test_alignment_rejections(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* virtual address not a multiple of 4096 */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].virtual_address = ENTRY + 8;
	spec.entry_point = ENTRY + 8;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* file offset and virtual address are not congruent modulo 4096 */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].file_offset = 4096 + 16;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

static void test_flag_rejections(void)
{
	JPBootContext context;
	JPTestElfSpec spec;
	static const uint32_t bad_flags[] = { 0, PF_W | PF_X, PF_R | PF_W | PF_X, PF_R | 8U, 0x80000000U };
	size_t i;

	for (i = 0; i < sizeof bad_flags / sizeof bad_flags[0]; i++) {
		jp_test_elf_default_spec(&spec);
		spec.segments[0].flags = bad_flags[i];
		build(&context, &spec, JP_ARCH_X86_64);
		expect_invalid(&context);
	}
}

static void test_permission_combinations_that_are_valid(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* A read-only data segment and a writable data segment next to the code. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = 3;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = PF_R;
	spec.segments[1].virtual_address = ENTRY + 1 * PAGE;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.segments[2] = spec.segments[0];
	spec.segments[2].flags = PF_R | PF_W;
	spec.segments[2].virtual_address = ENTRY + 2 * PAGE;
	spec.segments[2].file_offset = 3 * PAGE;
	build(&context, &spec, JP_ARCH_X86_64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);
	assert(context.kernel.segment_count == 3);
	assert(context.kernel.segments[1].flags == PF_R);
	assert(context.kernel.segments[2].flags == (PF_R | PF_W));
}

static void test_entry_point_rules(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* Entry outside every segment. */
	jp_test_elf_default_spec(&spec);
	spec.entry_point = ENTRY + 0x10000;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* Entry exactly at the end of the segment is outside it. */
	jp_test_elf_default_spec(&spec);
	spec.entry_point = ENTRY + spec.segments[0].memory_size;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* Entry in the last byte of the segment is inside. */
	jp_test_elf_default_spec(&spec);
	spec.entry_point = ENTRY + spec.segments[0].memory_size - 1;
	build(&context, &spec, JP_ARCH_X86_64);
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_SUCCESS);

	/* Entry inside a segment that is not executable. */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].flags = PF_R;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* Entry inside a non-executable segment although another segment is executable. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = 2;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = PF_R | PF_W;
	spec.segments[1].virtual_address = ENTRY + PAGE;
	spec.segments[1].file_offset = 2 * PAGE;
	spec.entry_point = ENTRY + PAGE;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

/* Each rule must be able to reject on its own, so the segments below are
 * arranged so that no other rule would reject them first. */
static void test_each_segment_rule_rejects_on_its_own(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* Unaligned virtual address, but congruent with the file offset. */
	jp_test_elf_default_spec(&spec);
	spec.segments[0].virtual_address = ENTRY + 8;
	spec.segments[0].file_offset = PAGE + 8;
	spec.entry_point = ENTRY + 8;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* A second segment without any permission, next to a good code segment. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = 2;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = 0;
	spec.segments[1].virtual_address = ENTRY + PAGE;
	spec.segments[1].file_offset = 2 * PAGE;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	/* A second segment with a flag bit outside R, W, X. */
	jp_test_elf_default_spec(&spec);
	spec.segment_count = 2;
	spec.segments[1] = spec.segments[0];
	spec.segments[1].flags = PF_R | 8U;
	spec.segments[1].virtual_address = ENTRY + PAGE;
	spec.segments[1].file_offset = 2 * PAGE;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);

	spec.segments[1].flags = PF_R | 0x80000000U;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
}

/* An offset beyond the end of the image is rejected, not wrapped around. */
static void test_offsets_beyond_the_image_are_rejected(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	/* The program header table starts far beyond the image. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke64(32, image_size + 1000);
	poke16(56, 0);
	expect_invalid(&context);

	/* A segment with no file bytes whose file offset is beyond the image. */
	jp_test_elf_default_spec(&spec);
	build(&context, &spec, JP_ARCH_X86_64);
	poke64(64 + 8, (image_size / PAGE + 2) * PAGE);   /* program header 0: file offset */
	poke64(64 + 32, 0);                               /* file size */
	expect_invalid(&context);
}

/* A failed validation clears whatever segment count was there before. */
static void test_failed_validation_resets_the_segment_count(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.magic = 0;
	build(&context, &spec, JP_ARCH_X86_64);
	context.kernel.segment_count = 5;
	assert(jp_boot_validate_kernel(&context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(context.kernel.segment_count == 0);
	assert(context.kernel.validation_state == JP_KERNEL_VALIDATION_INVALID);
}

static void test_invalid_image_is_never_partially_stored(void)
{
	JPBootContext context;
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.segments[0].flags = PF_W | PF_X;
	build(&context, &spec, JP_ARCH_X86_64);
	expect_invalid(&context);
	assert(context.kernel.entry_point == 0);
}

int main(void)
{
	test_valid_kernel_is_accepted_and_described();
	test_aarch64_kernel_is_accepted_only_on_aarch64();
	test_wrong_machine_is_rejected();
	test_parameters_and_state();
	test_header_rejections();
	test_program_header_table_rejections();
	test_load_segment_selection();
	test_segment_range_rejections();
	test_segment_file_offset_overflow();
	test_virtual_range_overflow();
	test_alignment_rejections();
	test_flag_rejections();
	test_permission_combinations_that_are_valid();
	test_entry_point_rules();
	test_each_segment_rule_rejects_on_its_own();
	test_offsets_beyond_the_image_are_rejected();
	test_failed_validation_resets_the_segment_count();
	test_invalid_image_is_never_partially_stored();
	return 0;
}
