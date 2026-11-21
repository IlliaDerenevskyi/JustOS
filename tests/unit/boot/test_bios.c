/*
 * Unit test (level U) of src/boot/platform/bios.c.
 * The boot data block is built by hand here (the loader is 16-bit code and is
 * not run), and the PS/2 keyboard controller is a fake behind
 * jp_bios_port_read8(). Rules: "Decided rules (the BIOS provider and its boot
 * chain)" in milestone0.md.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bios_io.h"
#include "justos/boot/platform/bios.h"
#include "justos/boot/platform/bios_block.h"

enum {
	PORT_DATA = 0x60,
	PORT_STATUS = 0x64,
	SCAN_QUEUE = 16,
	MIB = 0x100000
};

#define GIB4 0x100000000ULL

/* ================= the fake keyboard controller ================= */

static struct {
	uint8_t status[SCAN_QUEUE];
	uint8_t data[SCAN_QUEUE];
	size_t count;
	size_t next;
	uint8_t idle_status;           /* the status when the queue is empty */
	unsigned int status_reads;
	unsigned int data_reads;
	unsigned int other_reads;
} controller;

static void reset_controller(void)
{
	memset(&controller, 0, sizeof controller);
}

static void push_byte(uint8_t status, uint8_t data)
{
	assert(controller.count < SCAN_QUEUE);
	controller.status[controller.count] = status;
	controller.data[controller.count] = data;
	controller.count++;
}

uint8_t jp_bios_port_read8(uint16_t port)
{
	if (port == PORT_STATUS) {
		controller.status_reads++;
		return controller.next < controller.count ? controller.status[controller.next] : controller.idle_status;
	}
	if (port == PORT_DATA) {
		controller.data_reads++;
		assert(controller.next < controller.count);
		return controller.data[controller.next++];
	}
	controller.other_reads++;
	return 0;
}

/* ================= the block ================= */

static JPBiosBlock block;

static void set_entry(size_t index, uint64_t base, uint64_t length, uint32_t type)
{
	block.e820[index].base = base;
	block.e820[index].length = length;
	block.e820[index].type = type;
	block.e820[index].attributes = 1;
}

/* A normal block: low memory, 127 MiB of usable memory from 1 MiB, a video mode, and a kernel. */
static void reset_block(void)
{
	memset(&block, 0, sizeof block);
	memcpy(block.magic, JP_BIOS_BLOCK_MAGIC, JP_BIOS_BLOCK_MAGIC_SIZE);
	block.version = JP_BIOS_BLOCK_VERSION;
	block.flags = JP_BIOS_BLOCK_FLAG_A20 | JP_BIOS_BLOCK_FLAG_VIDEO | JP_BIOS_BLOCK_FLAG_KERNEL;
	block.boot_drive = 0x80;
	set_entry(0, 0x0, 0x9FC00, 1);
	set_entry(1, 0x9FC00, 0x400, 2);
	set_entry(2, 0xF0000, 0x10000, 2);
	set_entry(3, 1 * MIB, 127 * MIB, 1);
	block.e820_count = 4;
	block.framebuffer_base = 0xFD000000;
	block.width = 1024;
	block.height = 768;
	block.pitch = 4096;
	block.video_format = JP_BIOS_VIDEO_FORMAT_BGR;
	block.kernel_address = 0x200000;
	block.kernel_size = 0x12345;
	reset_controller();
}

static JPBiosContext *new_context(void)
{
	void *memory = calloc(1, jp_bios_context_size());

	assert(memory != NULL);
	return jp_bios_context_setup(memory, &block);
}

static JPBiosContext *initialized_context(void)
{
	JPBiosContext *context = new_context();

	assert(jp_bios_initialize(context) == JP_BOOT_SUCCESS);
	return context;
}

static void expect_region(const JPMemoryMap *map, size_t index, uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	assert(index < map->region_count);
	assert(map->regions[index].base_address == base);
	assert(map->regions[index].length == length);
	assert(map->regions[index].type == type);
}

/* ================= the tests ================= */

static void test_context_setup(void)
{
	void *memory;
	JPBiosContext *context;

	assert(jp_bios_context_size() > 0);
	assert(jp_bios_context_size() <= 65536);     /* it must fit the area of the entry point */
	memory = calloc(1, jp_bios_context_size());
	reset_block();
	assert(jp_bios_context_setup(NULL, &block) == NULL);
	assert(jp_bios_context_setup(memory, NULL) == NULL);
	context = jp_bios_context_setup(memory, &block);
	assert((void *)context == memory);
	free(memory);
}

static void test_initialize(void)
{
	JPBiosContext *context;

	reset_block();
	context = new_context();
	assert(jp_bios_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_initialize(context) == JP_BOOT_SUCCESS);
	assert(jp_bios_initialize(context) == JP_BOOT_FAILED);          /* only from UNINITIALIZED */

	/* Every field of the check is needed. */
	reset_block();
	block.magic[7] = '2';
	assert(jp_bios_initialize(new_context()) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.magic[0] = 'X';
	assert(jp_bios_initialize(new_context()) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.version = 2;
	assert(jp_bios_initialize(new_context()) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.version = 0;
	assert(jp_bios_initialize(new_context()) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.e820_count = JP_BIOS_BLOCK_MAX_E820 + 1;
	assert(jp_bios_initialize(new_context()) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.e820_count = JP_BIOS_BLOCK_MAX_E820;                       /* exactly the maximum is fine */
	assert(jp_bios_initialize(new_context()) == JP_BOOT_SUCCESS);
	reset_block();
	block.flags &= ~(uint32_t)JP_BIOS_BLOCK_FLAG_A20;
	assert(jp_bios_initialize(new_context()) == JP_BOOT_FAILED);

	/* A failed initialize leaves the provider unusable. */
	reset_block();
	block.version = 9;
	context = new_context();
	assert(jp_bios_initialize(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	{
		JPMemoryMap map;

		assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_NOT_INITIALIZED);
	}
}

static void test_operations_need_initialization(void)
{
	JPBiosContext *context;
	JPMemoryMap map;
	JPFramebufferInfo fb;
	JPKernelImage image;
	JPBootInputEvent event;

	reset_block();
	context = new_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_get_framebuffer(context, &fb) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_get_kernel_image(context, &image) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_prepare_handoff(context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_reserve_memory(context, 0x1000000, 0x1000) == JP_BOOT_NOT_INITIALIZED);
	assert(controller.status_reads == 0 && controller.data_reads == 0);   /* the controller was not touched */

	/* NULL arguments. */
	context = initialized_context();
	assert(jp_bios_get_memory_map(NULL, &map) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_memory_map(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_framebuffer(NULL, &fb) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_framebuffer(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_kernel_image(NULL, &image) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_kernel_image(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_prepare_handoff(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_read_input(NULL, &event) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_read_input(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_reserve_memory(NULL, 0x1000000, 0x1000) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_memory_map_normalization(void)
{
	static const struct { uint32_t type; JPMemoryRegionType expected; } table[] = {
		{ 1, JP_MEMORY_REGION_USABLE },
		{ 2, JP_MEMORY_REGION_RESERVED },
		{ 3, JP_MEMORY_REGION_ACPI },
		{ 4, JP_MEMORY_REGION_ACPI },
		{ 5, JP_MEMORY_REGION_UNUSABLE },
		{ 0, JP_MEMORY_REGION_RESERVED },
		{ 6, JP_MEMORY_REGION_RESERVED },
		{ 0xFFFFFFFF, JP_MEMORY_REGION_RESERVED }
	};
	size_t i;

	for (i = 0; i < sizeof table / sizeof table[0]; i++) {
		JPMemoryMap map;
		JPBiosContext *context;

		reset_block();
		set_entry(0, 0x20 * MIB, 4 * MIB, table[i].type);       /* above the 8 MiB: no cutting */
		block.e820_count = 1;
		context = initialized_context();
		assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
		assert(map.state == JP_DETECTION_STATE_DETECTED);
		assert(map.region_count == 1);
		expect_region(&map, 0, 0x20 * MIB, 4 * MIB, table[i].expected);
	}
}

static void test_boot_environment_ranges_are_cut_out(void)
{
	JPMemoryMap map;
	JPBiosContext *context;

	/* The default block: the first 8 MiB become RESERVED, the rest stays usable. */
	reset_block();
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 5);
	expect_region(&map, 0, 0x0, 0x9FC00, JP_MEMORY_REGION_RESERVED);      /* usable low memory, now reserved */
	expect_region(&map, 1, 0x9FC00, 0x400, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 2, 0xF0000, 0x10000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 3, 1 * MIB, 7 * MIB, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 4, 8 * MIB, 120 * MIB, JP_MEMORY_REGION_USABLE);

	/* A region that ends exactly at 8 MiB is reserved completely, one that starts there is untouched. */
	reset_block();
	set_entry(0, 2 * MIB, 6 * MIB, 1);
	set_entry(1, 8 * MIB, 8 * MIB, 1);
	block.e820_count = 2;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 2);
	expect_region(&map, 0, 2 * MIB, 6 * MIB, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 1, 8 * MIB, 8 * MIB, JP_MEMORY_REGION_USABLE);

	/* One byte over the limit is split at the limit. */
	reset_block();
	set_entry(0, 7 * MIB, 2 * MIB, 1);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 2);
	expect_region(&map, 0, 7 * MIB, 1 * MIB, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 1, 8 * MIB, 1 * MIB, JP_MEMORY_REGION_USABLE);

	/* Only usable regions are cut: a reserved region in the low area stays as it is (and is not doubled). */
	reset_block();
	set_entry(0, 2 * MIB, 2 * MIB, 3);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 1);
	expect_region(&map, 0, 2 * MIB, 2 * MIB, JP_MEMORY_REGION_ACPI);
}

static void test_memory_above_4_gib_is_cut_out(void)
{
	JPMemoryMap map;
	JPBiosContext *context;

	reset_block();
	set_entry(0, 16 * MIB, 3 * 1024ULL * MIB, 1);                 /* up to 3 GiB + 16 MiB: under the limit */
	set_entry(1, GIB4 - 2 * MIB, 6 * MIB, 1);                      /* crosses the 4 GiB */
	set_entry(2, GIB4 + 4 * MIB, 1024ULL * MIB, 1);                /* completely above */
	set_entry(3, GIB4, 2 * MIB, 3);                                /* an ACPI region above stays as it is */
	block.e820_count = 4;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 5);
	expect_region(&map, 0, 16 * MIB, 3 * 1024ULL * MIB, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 1, GIB4 - 2 * MIB, 2 * MIB, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 2, GIB4, 4 * MIB, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 3, GIB4 + 4 * MIB, 1024ULL * MIB, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 4, GIB4, 2 * MIB, JP_MEMORY_REGION_ACPI);

	/* A region that starts exactly at 4 GiB is cut out completely. */
	reset_block();
	set_entry(0, GIB4, 1 * MIB, 1);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 1);
	expect_region(&map, 0, GIB4, 1 * MIB, JP_MEMORY_REGION_RESERVED);

	/* A region that ends exactly at 4 GiB is not touched. */
	reset_block();
	set_entry(0, GIB4 - 1 * MIB, 1 * MIB, 1);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 1);
	expect_region(&map, 0, GIB4 - 1 * MIB, 1 * MIB, JP_MEMORY_REGION_USABLE);
}

static void test_memory_map_failures(void)
{
	JPMemoryMap map;
	JPBiosContext *context;
	size_t i;

	/* No entries: nothing was detected. */
	reset_block();
	block.e820_count = 0;
	context = initialized_context();
	memset(&map, 0xFF, sizeof map);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_NOT_DETECTED);
	assert(map.state == JP_DETECTION_STATE_NOT_DETECTED);

	/* Zero-length entries are skipped. */
	reset_block();
	set_entry(0, 20 * MIB, 0, 1);
	set_entry(1, 24 * MIB, 1 * MIB, 1);
	block.e820_count = 2;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 1);
	expect_region(&map, 0, 24 * MIB, 1 * MIB, JP_MEMORY_REGION_USABLE);

	/* Only zero-length entries: nothing usable to publish. */
	reset_block();
	set_entry(0, 20 * MIB, 0, 1);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_NOT_DETECTED);

	/* An entry that overflows. */
	reset_block();
	set_entry(0, UINT64_MAX - 0x1000, 0x2000, 2);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* An entry that ends exactly at the end of the address space does not overflow. */
	reset_block();
	set_entry(0, UINT64_MAX - 0x1000, 0x1000, 2);
	block.e820_count = 1;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	expect_region(&map, 0, UINT64_MAX - 0x1000, 0x1000, JP_MEMORY_REGION_RESERVED);

	/* More regions than the map holds: 128 entries fit exactly, a split makes 129. */
	reset_block();
	for (i = 0; i < JP_BIOS_BLOCK_MAX_E820; i++) {
		set_entry(i, 16 * MIB + i * 2 * MIB, 1 * MIB, 1);
	}
	block.e820_count = JP_BIOS_BLOCK_MAX_E820;
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == JP_MAX_MEMORY_REGIONS);
	assert(jp_bios_reserve_memory(context, 16 * MIB + 0x1000, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_bios_prepare_handoff(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	{
		/* The failed handoff changed nothing: the provider still serves, and is not prepared. */
		JPBootInputEvent event;

		assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	}
}

static void test_final_map_copy_stays_inside_the_map(void)
{
	struct { JPMemoryMap map; uint64_t canary[8]; } holder;
	JPBiosContext *context;
	size_t i;

	reset_block();
	for (i = 0; i < JP_BIOS_BLOCK_MAX_E820; i++) {
		set_entry(i, 16 * MIB + i * 2 * MIB, 1 * MIB, 1);
	}
	block.e820_count = JP_BIOS_BLOCK_MAX_E820;
	context = initialized_context();
	assert(jp_bios_prepare_handoff(context) == JP_BOOT_SUCCESS);
	for (i = 0; i < 8; i++) {
		holder.canary[i] = 0x5A5A5A5A5A5A5A5AULL;
	}
	assert(jp_bios_get_memory_map(context, &holder.map) == JP_BOOT_SUCCESS);
	assert(holder.map.region_count == JP_MAX_MEMORY_REGIONS);
	for (i = 0; i < 8; i++) {
		assert(holder.canary[i] == 0x5A5A5A5A5A5A5A5AULL);
	}
}

static void test_framebuffer(void)
{
	JPBiosContext *context;
	JPFramebufferInfo fb;

	reset_block();
	context = initialized_context();
	memset(&fb, 0xFF, sizeof fb);
	assert(jp_bios_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(fb.state == JP_DETECTION_STATE_DETECTED);
	assert(fb.base_address == 0xFD000000);
	assert(fb.width == 1024 && fb.height == 768 && fb.pitch == 4096);
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_BGR_888);

	reset_block();
	block.video_format = JP_BIOS_VIDEO_FORMAT_RGB;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_SUCCESS);
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_RGB_888);

	/* No mode: not detected, and the fields of the block are not looked at. */
	reset_block();
	block.flags &= ~(uint32_t)JP_BIOS_BLOCK_FLAG_VIDEO;
	context = initialized_context();
	memset(&fb, 0xFF, sizeof fb);
	assert(jp_bios_get_framebuffer(context, &fb) == JP_BOOT_NOT_DETECTED);
	assert(fb.state == JP_DETECTION_STATE_NOT_DETECTED);

	/* A mode the provider cannot name is refused, never guessed. */
	reset_block();
	block.video_format = 0;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.video_format = 3;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.framebuffer_base = 0;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.width = 0;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.height = 0;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.pitch = 0;
	assert(jp_bios_get_framebuffer(initialized_context(), &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_kernel_image(void)
{
	JPBiosContext *context;
	JPKernelImage image;

	reset_block();
	context = initialized_context();
	memset(&image, 0xFF, sizeof image);
	assert(jp_bios_get_kernel_image(context, &image) == JP_BOOT_SUCCESS);
	assert(image.state == JP_DETECTION_STATE_DETECTED);
	assert((uintptr_t)image.buffer == 0x200000);
	assert(image.size == 0x12345);

	reset_block();
	block.flags &= ~(uint32_t)JP_BIOS_BLOCK_FLAG_KERNEL;
	context = initialized_context();
	memset(&image, 0xFF, sizeof image);
	assert(jp_bios_get_kernel_image(context, &image) == JP_BOOT_NOT_DETECTED);
	assert(image.state == JP_DETECTION_STATE_NOT_DETECTED);

	/* A kernel the loader says it read but with no place or no size is not accepted. */
	reset_block();
	block.kernel_size = 0;
	assert(jp_bios_get_kernel_image(initialized_context(), &image) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.kernel_address = 0;
	assert(jp_bios_get_kernel_image(initialized_context(), &image) == JP_BOOT_ERROR_CORRUPTED_DATA);
	reset_block();
	block.kernel_address = UINT64_MAX - 0x100;
	assert(jp_bios_get_kernel_image(initialized_context(), &image) == JP_BOOT_ERROR_CORRUPTED_DATA);
	block.kernel_size = 0x100;                                      /* ends exactly at the end of the address space */
	assert(jp_bios_get_kernel_image(initialized_context(), &image) == JP_BOOT_SUCCESS);
}

static void test_input(void)
{
	static const struct { uint8_t scan; JPBootInputKind kind; JPBootInputKey key; uint32_t character; } table[] = {
		{ 0x48, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_UP, 0 },
		{ 0x50, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_DOWN, 0 },
		{ 0x4D, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_RIGHT, 0 },
		{ 0x4B, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_LEFT, 0 },
		{ 0x1C, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_ENTER, 0 },
		{ 0x01, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_ESCAPE, 0 },
		{ 0x44, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_F10, 0 },
		{ 0x0E, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_BACKSPACE, 0 },
		{ 0x02, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '1' },
		{ 0x0A, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '9' },
		{ 0x0B, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '0' },
		{ 0x10, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'q' },
		{ 0x19, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'p' },
		{ 0x1E, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'a' },
		{ 0x26, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'l' },
		{ 0x2C, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'z' },
		{ 0x32, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'm' },
		{ 0x11, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'w' },
		{ 0x1F, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 's' },
		{ 0x2D, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'x' },
		/* punctuation and space (without Shift) */
		{ 0x0C, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '-' },
		{ 0x0D, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '=' },
		{ 0x1A, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '[' },
		{ 0x1B, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, ']' },
		{ 0x27, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, ';' },
		{ 0x28, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '\'' },
		{ 0x29, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '`' },
		{ 0x2B, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '\\' },
		{ 0x33, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, ',' },
		{ 0x34, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '.' },
		{ 0x35, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '/' },
		{ 0x39, JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, ' ' },
		/* keys that cannot be mapped */
		{ 0x1D, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* Ctrl */
		{ 0x3B, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* F1 */
		{ 0x0F, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* Tab, just before 'q' */
		{ 0x2A, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* left Shift, just before z and after the backslash */
		{ 0x36, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* right Shift, just after '/' */
		{ 0x38, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* Alt, just before space */
		{ 0x3A, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* Caps Lock, just after space */
		{ 0x37, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* keypad multiply */
		{ 0xE0, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* the prefix byte of an extended key */
		{ 0x00, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },
		/* break codes (key released): bit 7 set, never mapped */
		{ 0xC8, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* 0x48 released */
		{ 0x81, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* Escape released */
		{ 0x9E, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 }       /* 'a' released */
	};
	JPBiosContext *context;
	JPBootInputEvent event;
	size_t i;

	for (i = 0; i < sizeof table / sizeof table[0]; i++) {
		reset_block();
		push_byte(0x01, table[i].scan);
		context = initialized_context();
		memset(&event, 0xFF, sizeof event);
		assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
		assert(event.kind == table[i].kind);
		if (table[i].kind == JP_BOOT_INPUT_KEY) {
			assert(event.key == table[i].key);
		}
		if (table[i].kind == JP_BOOT_INPUT_CHARACTER) {
			assert(event.character == table[i].character);
		}
		assert(controller.data_reads == 1);               /* the byte was consumed */
		assert(controller.other_reads == 0);              /* only the two known ports are used */
	}

	/* Nothing waiting: no event, and the data port is not read. */
	reset_block();
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);
	assert(controller.status_reads == 1 && controller.data_reads == 0);

	/* The other bits of the status do not mean "a byte is waiting" (bit 1: input buffer full). */
	reset_block();
	controller.idle_status = 0x02;
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);
	assert(controller.data_reads == 0);

	/* Bit 0 together with other bits: a byte is waiting. */
	reset_block();
	push_byte(0x1D, 0x01);
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_KEY && event.key == JP_BOOT_KEY_ESCAPE);

	/* Bit 5 marks a byte of the mouse: it is consumed and is no key. */
	reset_block();
	push_byte(0x21, 0x01);
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);
	assert(controller.data_reads == 1);

	/* Several keys come one per call, in order. */
	reset_block();
	push_byte(0x01, 0x1E);
	push_byte(0x01, 0x1C);
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_CHARACTER && event.character == 'a');
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_KEY && event.key == JP_BOOT_KEY_ENTER);
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);

	/* No controller: the status port reads 0xFF (nothing drives the bus). */
	reset_block();
	controller.idle_status = 0xFF;
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_NOT_DETECTED);
	assert(controller.data_reads == 0);
}

static void test_reserve_memory(void)
{
	JPBiosContext *context;
	JPMemoryMap map;
	size_t i;

	reset_block();
	context = initialized_context();
	/* Inside the usable region [8 MiB, 128 MiB); the range is rounded outward to whole pages. */
	assert(jp_bios_reserve_memory(context, 20 * MIB + 0x10, 0x1000) == JP_BOOT_SUCCESS);   /* [20 MiB, 20 MiB + 8 KiB) */
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 7);
	expect_region(&map, 4, 8 * MIB, 12 * MIB, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 5, 20 * MIB, 0x2000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 6, 20 * MIB + 0x2000, 128 * MIB - 20 * MIB - 0x2000, JP_MEMORY_REGION_USABLE);

	/* Touching the start and the end of a usable region leaves no empty piece. */
	assert(jp_bios_reserve_memory(context, 8 * MIB, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_bios_reserve_memory(context, 128 * MIB - 0x1000, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	expect_region(&map, 4, 8 * MIB, 0x1000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 5, 8 * MIB + 0x1000, 12 * MIB - 0x1000, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 8, 128 * MIB - 0x1000, 0x1000, JP_MEMORY_REGION_RESERVED);
	assert(map.region_count == 9);
	for (i = 0; i < map.region_count; i++) {
		assert(map.regions[i].length > 0);
	}

	/* Invalid ranges. */
	assert(jp_bios_reserve_memory(context, 30 * MIB, 0) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_reserve_memory(context, UINT64_MAX - 10, 0x1000) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_reserve_memory(context, UINT64_MAX - 0x100, 0x80) == JP_BOOT_ERROR_INVALID_PARAM);   /* rounding overflows */

	/* The rounding is checked exactly at its limit: these ranges round without overflow and are simply not usable memory. */
	assert(jp_bios_reserve_memory(context, UINT64_MAX - 4095 - 0x1000, 0x1000) == JP_BOOT_FAILED);
	assert(jp_bios_reserve_memory(context, UINT64_MAX - 4096 - 0x1000, 0x1000) == JP_BOOT_FAILED);

	/* Not inside usable memory: the same range twice, low memory, a region boundary, memory above 4 GiB, nothing there. */
	assert(jp_bios_reserve_memory(context, 20 * MIB, 0x1000) == JP_BOOT_FAILED);               /* already reserved */
	assert(jp_bios_reserve_memory(context, 4 * MIB, 0x1000) == JP_BOOT_FAILED);                /* the first 8 MiB */
	assert(jp_bios_reserve_memory(context, 128 * MIB - 0x2000, 0x2000) == JP_BOOT_FAILED);     /* touches the reserved end */
	assert(jp_bios_reserve_memory(context, 127 * MIB, 2 * MIB) == JP_BOOT_FAILED);             /* leaves the region */
	assert(jp_bios_reserve_memory(context, 200 * MIB, 0x1000) == JP_BOOT_FAILED);              /* no region there */
	assert(jp_bios_reserve_memory(context, GIB4, 0x1000) == JP_BOOT_FAILED);

	/* A failed reservation changed nothing. */
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 9);

	/* No memory map can be built (the loader recorded nothing): nothing can be reserved. */
	reset_block();
	block.e820_count = 0;
	context = initialized_context();
	assert(jp_bios_reserve_memory(context, 16 * MIB, 0x1000) == JP_BOOT_FAILED);

	/* The usable range must be one region: two touching usable regions do not form one for the check. */
	reset_block();
	set_entry(0, 16 * MIB, 4 * MIB, 1);
	set_entry(1, 20 * MIB, 4 * MIB, 1);
	block.e820_count = 2;
	context = initialized_context();
	assert(jp_bios_reserve_memory(context, 19 * MIB, 2 * MIB) == JP_BOOT_FAILED);
	assert(jp_bios_reserve_memory(context, 18 * MIB, 2 * MIB) == JP_BOOT_SUCCESS);

	/* The table holds 64 ranges; the 65th is refused (the range itself is fine, so the table is the reason). */
	reset_block();
	context = initialized_context();
	for (i = 0; i < 64; i++) {
		assert(jp_bios_reserve_memory(context, 16 * MIB + i * 0x1000, 0x1000) == JP_BOOT_SUCCESS);
	}
	assert(jp_bios_reserve_memory(context, 64 * MIB, 0x1000) == JP_BOOT_FAILED);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 4 + 1 + 64 + 1);
	expect_region(&map, 4, 8 * MIB, 8 * MIB, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 5, 16 * MIB, 0x1000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 68, 16 * MIB + 63 * 0x1000, 0x1000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 69, 16 * MIB + 64 * 0x1000, 128 * MIB - 16 * MIB - 64 * 0x1000, JP_MEMORY_REGION_USABLE);
}

static void test_reservations_from_touching_ranges_do_not_overlap(void)
{
	JPBiosContext *context;
	JPMemoryMap map;
	size_t i;

	/* Two touching reservations stay two regions: no usable piece between them and no overlap. */
	reset_block();
	context = initialized_context();
	assert(jp_bios_reserve_memory(context, 16 * MIB, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_bios_reserve_memory(context, 16 * MIB + 0x1000, 0x1000) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	for (i = 0; i < map.region_count; i++) {
		size_t j;

		for (j = i + 1; j < map.region_count; j++) {
			assert(map.regions[i].base_address + map.regions[i].length <= map.regions[j].base_address ||
			       map.regions[j].base_address + map.regions[j].length <= map.regions[i].base_address);
		}
	}
	assert(map.region_count == 8);
	expect_region(&map, 4, 8 * MIB, 8 * MIB, JP_MEMORY_REGION_USABLE);
	expect_region(&map, 5, 16 * MIB, 0x1000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 6, 16 * MIB + 0x1000, 0x1000, JP_MEMORY_REGION_RESERVED);
	expect_region(&map, 7, 16 * MIB + 0x2000, 128 * MIB - 16 * MIB - 0x2000, JP_MEMORY_REGION_USABLE);
}

static void test_transition(void)
{
	JPBiosContext *context;
	JPMemoryMap early;
	JPMemoryMap final_map;
	JPBootInputEvent event;
	JPFramebufferInfo fb;
	JPKernelImage image;

	reset_block();
	context = initialized_context();
	assert(jp_bios_get_memory_map(context, &early) == JP_BOOT_SUCCESS);
	assert(jp_bios_reserve_memory(context, 32 * MIB, 3 * 0x1000) == JP_BOOT_SUCCESS);
	push_byte(0x01, 0x01);

	assert(jp_bios_prepare_handoff(context) == JP_BOOT_SUCCESS);

	/* The final map shows the reservation and equals what get_memory_map computes. */
	assert(jp_bios_get_memory_map(context, &final_map) == JP_BOOT_SUCCESS);
	assert(final_map.state == JP_DETECTION_STATE_DETECTED);
	assert(final_map.region_count == early.region_count + 2);
	expect_region(&final_map, 5, 32 * MIB, 3 * 0x1000, JP_MEMORY_REGION_RESERVED);

	/* The final map does not depend on the block any more (the block may be overwritten by the kernel). */
	memset(&block, 0xEE, sizeof block);
	memset(&final_map, 0, sizeof final_map);
	assert(jp_bios_get_memory_map(context, &final_map) == JP_BOOT_SUCCESS);
	assert(final_map.region_count == early.region_count + 2);

	/* From now on nothing else works: no input (and the controller is untouched), no reservation, no second handoff. */
	assert(jp_bios_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(controller.status_reads == 0 && controller.data_reads == 0);
	assert(jp_bios_reserve_memory(context, 48 * MIB, 0x1000) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_prepare_handoff(context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_get_framebuffer(context, &fb) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_get_kernel_image(context, &image) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_bios_initialize(context) == JP_BOOT_FAILED);
	assert(jp_bios_get_memory_map(NULL, &final_map) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_bios_get_memory_map(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_input_is_allowed_while_active(void)
{
	JPBiosContext *context;
	JPMemoryMap map;
	JPFramebufferInfo fb;
	JPKernelImage image;
	JPBootInputEvent event;

	/* INITIALIZED and ACTIVE (after any successful operation) both allow input. */
	reset_block();
	push_byte(0x01, 0x01);
	push_byte(0x01, 0x01);
	push_byte(0x01, 0x01);
	push_byte(0x01, 0x01);
	context = initialized_context();
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(jp_bios_get_kernel_image(context, &image) == JP_BOOT_SUCCESS);
	assert(jp_bios_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_KEY);
}

int main(void)
{
	test_context_setup();
	test_initialize();
	test_operations_need_initialization();
	test_memory_map_normalization();
	test_boot_environment_ranges_are_cut_out();
	test_memory_above_4_gib_is_cut_out();
	test_memory_map_failures();
	test_final_map_copy_stays_inside_the_map();
	test_framebuffer();
	test_kernel_image();
	test_input();
	test_reserve_memory();
	test_reservations_from_touching_ranges_do_not_overlap();
	test_transition();
	test_input_is_allowed_while_active();
	return 0;
}
