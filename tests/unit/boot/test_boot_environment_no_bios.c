/*
 * Unit test (level U) of src/boot/boot_environment.c built with
 * JP_BOOT_WITHOUT_BIOS (the UEFI boot application): the BIOS provider is not
 * linked at all, and the BIOS environment type is refused.
 */

#include <assert.h>
#include <string.h>

#include "fake_uefi.h"
#include "justos/boot/boot_environment.h"

static void test_uefi_still_works(void)
{
	JPBootContext context;
	JPUefiContext fake;
	JPMemoryMap map;

	memset(&context, 0, sizeof context);
	jp_fake_uefi_reset(&fake);
	context.environment.type = JP_BOOT_ENVIRONMENT_UEFI;
	context.environment.platform_context = &fake;
	memset(&map, 0, sizeof map);
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_SUCCESS);
	assert(jp_boot_environment_reserve_memory(&context, 0x200000, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_boot_environment_prepare_handoff(&context) == JP_BOOT_SUCCESS);
	assert(fake.called_by == 'U');
}

static void test_bios_is_refused(void)
{
	JPBootContext context;
	char not_a_provider = 0;

	memset(&context, 0, sizeof context);
	context.environment.type = JP_BOOT_ENVIRONMENT_BIOS;
	context.environment.platform_context = &not_a_provider;   /* never touched */
	assert(jp_boot_environment_initialize(&context) == JP_BOOT_FAILED);
	assert(context.environment.state == JP_DETECTION_STATE_FAILED);
	assert(context.environment.lifecycle == JP_BOOT_ENVIRONMENT_LIFECYCLE_FAILED);
	assert(jp_boot_environment_get_memory_map(&context) == JP_BOOT_NOT_INITIALIZED);
	assert(not_a_provider == 0);
}

int main(void)
{
	test_uefi_still_works();
	test_bios_is_refused();
	return 0;
}
