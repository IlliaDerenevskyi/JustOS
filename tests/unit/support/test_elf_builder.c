/* Unit test (level U) of the test-support ELF builder and the fake providers. */

#include <assert.h>
#include <string.h>

#include "elf_builder.h"
#include "fake_bios.h"
#include "fake_uefi.h"

static uint16_t get16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t get32(const uint8_t *p)
{
	return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16);
}

static uint64_t get64(const uint8_t *p)
{
	return (uint64_t)get32(p) | ((uint64_t)get32(p + 4) << 32);
}

static void test_default_elf_layout(void)
{
	uint8_t buffer[16384];
	JPTestElfSpec spec;
	size_t size;
	const uint8_t *ph;

	jp_test_elf_default_spec(&spec);
	size = jp_test_elf_build(&spec, buffer, sizeof buffer);
	assert(size > JP_TEST_ELF_HEADER_SIZE + JP_TEST_ELF_PHDR_SIZE);
	assert(size == spec.segments[0].file_offset + spec.segments[0].file_size);
	assert(get32(buffer) == 0x464C457FU);
	assert(buffer[4] == 2);
	assert(get16(buffer + 16) == 2);
	assert(get16(buffer + 18) == 62);
	assert(get64(buffer + 24) == spec.entry_point);
	assert(get64(buffer + 32) == JP_TEST_ELF_HEADER_SIZE);
	assert(get16(buffer + 54) == JP_TEST_ELF_PHDR_SIZE);
	assert(get16(buffer + 56) == 1);

	ph = buffer + JP_TEST_ELF_HEADER_SIZE;
	assert(get32(ph) == 1);
	assert(get64(ph + 32) == spec.segments[0].file_size);
	assert(get64(ph + 40) == spec.segments[0].memory_size);
	assert(buffer[get64(ph + 8)] == 1);
}

static void test_wrong_values_can_be_written(void)
{
	uint8_t buffer[16384];
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	spec.magic = 0;
	spec.machine = 183;
	assert(jp_test_elf_build(&spec, buffer, sizeof buffer) != 0);
	assert(get32(buffer) == 0);
	assert(get16(buffer + 18) == 183);
}

static void test_too_small_buffer(void)
{
	uint8_t buffer[JP_TEST_ELF_HEADER_SIZE];
	JPTestElfSpec spec;

	jp_test_elf_default_spec(&spec);
	assert(jp_test_elf_build(&spec, buffer, sizeof buffer) == 0);
}

static void test_fake_uefi_input_rules(void)
{
	JPUefiContext fake;
	JPBootInputEvent event;
	JPBootInputEvent key = { JP_BOOT_INPUT_KEY, JP_BOOT_KEY_ENTER, 0 };

	jp_fake_uefi_reset(&fake);
	assert(jp_uefi_read_input(&fake, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_initialize(&fake) == JP_BOOT_SUCCESS);
	assert(jp_fake_uefi_push_input(&fake, key) == 0);
	assert(jp_uefi_read_input(&fake, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_KEY && event.key == JP_BOOT_KEY_ENTER);
	assert(jp_uefi_read_input(&fake, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);
	assert(jp_uefi_prepare_handoff(&fake) == JP_BOOT_SUCCESS);
	assert(jp_uefi_read_input(&fake, &event) == JP_BOOT_NOT_INITIALIZED);
}

static void test_fake_bios_failure_is_reported_as_configured(void)
{
	JPBiosContext fake;
	JPMemoryMap map;

	jp_fake_bios_reset(&fake);
	fake.memory_map_result = JP_BOOT_FAILED;
	assert(jp_bios_get_memory_map(&fake, &map) == JP_BOOT_FAILED);
	assert(fake.memory_map_calls == 1);
}

int main(void)
{
	test_default_elf_layout();
	test_wrong_values_can_be_written();
	test_too_small_buffer();
	test_fake_uefi_input_rules();
	test_fake_bios_failure_is_reported_as_configured();
	return 0;
}
