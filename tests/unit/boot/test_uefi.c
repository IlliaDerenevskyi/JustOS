/*
 * Unit test (level U) of src/boot/platform/uefi.c.
 * A fake firmware (a system table with boot services, a graphics output, a
 * file system, and a text input) is built here with the same calling
 * convention as the real one. Rules: "Decided rules (the UEFI provider)" in
 * milestone0.md.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "justos/boot/platform/uefi.h"
#include "uefi_defs.h"

/* ================= the fake firmware ================= */

enum {
	MAX_DESCRIPTORS = 400,
	DESCRIPTOR_STRIDE = 48,        /* larger than sizeof(JPUefiMemoryDescriptor) on purpose */
	MAX_LOG = 16,
	FILE_CHUNK = 7,
	KEY_QUEUE = 16
};

typedef struct FakeKey {
	JPUefiStatus status;
	JPUefiInputKey key;
} FakeKey;

static struct Firmware {
	/* memory map */
	JPUefiMemoryDescriptor descriptors[MAX_DESCRIPTORS];
	size_t descriptor_count;
	JPUefiUintn descriptor_size;
	JPUefiStatus get_memory_map_status;       /* a failure other than "too small" */
	bool always_too_small;
	unsigned int extra_after_pool;            /* descriptors that appear after each pool allocation */
	uint64_t key;
	unsigned int get_memory_map_calls;
	JPUefiUintn map_size_extra;               /* bytes added to the reported map size (not a whole number of descriptors) */

	/* pages and pool */
	JPUefiStatus allocate_pages_status;
	unsigned int allocate_pages_calls;
	JPUefiUintn ap_type[MAX_LOG], ap_memory_type[MAX_LOG], ap_pages[MAX_LOG];
	uint64_t ap_address[MAX_LOG];
	JPUefiStatus allocate_pool_status;
	unsigned int allocate_pool_calls;
	JPUefiUintn last_pool_size;
	unsigned int free_pool_calls;

	/* exit */
	unsigned int exit_calls;
	uint64_t exit_key[MAX_LOG];
	unsigned int exit_refusals;               /* how many exits answer "invalid parameter" */
	JPUefiStatus exit_status;                 /* if not zero: every exit answers this */
	bool exited;
	unsigned int calls_after_exit;

	/* graphics output */
	bool gop_present;
	JPUefiGraphicsOutput gop;
	JPUefiGopMode gop_mode;
	JPUefiGopModeInfo gop_info;

	/* file system */
	bool file_present;
	uint8_t file_bytes[64];
	size_t file_size;
	JPUefiStatus read_status;
	size_t read_limit;                        /* the file claims a size but yields less */
	size_t read_position;
	JPUefiChar16 opened_path[64];
	uint64_t opened_mode;
	unsigned int files_closed;

	/* text input */
	JPUefiSimpleTextInput input;
	FakeKey keys[KEY_QUEUE];
	size_t key_count, key_next;

	/* a probe that calls the provider from inside ExitBootServices */
	JPUefiContext *probe_context;
	JPBootResult probe_input_result;
	bool probe_ran;

	/* handles */
	JPUefiLoadedImage loaded_image;
	JPUefiSimpleFileSystem file_system;
	JPUefiFile root;
	JPUefiFile file;
	char image_handle_object, device_handle_object;
} fw;

static void touch(void)
{
	if (fw.exited) {
		fw.calls_after_exit++;
	}
}

static bool same_guid(const JPUefiGuid *a, const JPUefiGuid *b)
{
	return memcmp(a, b, sizeof *a) == 0;
}

static JPUefiStatus JP_UEFI_API fake_allocate_pages(JPUefiUintn type, JPUefiUintn memory_type,
	JPUefiUintn pages, uint64_t *address)
{
	unsigned int i = fw.allocate_pages_calls++;

	touch();
	if (i < MAX_LOG) {
		fw.ap_type[i] = type;
		fw.ap_memory_type[i] = memory_type;
		fw.ap_pages[i] = pages;
		fw.ap_address[i] = *address;
	}
	return fw.allocate_pages_status;
}

static JPUefiStatus JP_UEFI_API fake_free_pages(uint64_t address, JPUefiUintn pages)
{
	(void)address;
	(void)pages;
	touch();
	return JP_UEFI_SUCCESS;
}

static void add_descriptor(JPUefiMemoryType type, uint64_t start, uint64_t pages)
{
	JPUefiMemoryDescriptor *d = &fw.descriptors[fw.descriptor_count++];

	assert(fw.descriptor_count <= MAX_DESCRIPTORS);
	memset(d, 0, sizeof *d);
	d->type = (uint32_t)type;
	d->physical_start = start;
	d->number_of_pages = pages;
}

static JPUefiStatus JP_UEFI_API fake_get_memory_map(JPUefiUintn *map_size, JPUefiMemoryDescriptor *map,
	JPUefiUintn *map_key, JPUefiUintn *descriptor_size, uint32_t *descriptor_version)
{
	JPUefiUintn needed;
	size_t i;

	touch();
	fw.get_memory_map_calls++;
	if (fw.get_memory_map_status != JP_UEFI_SUCCESS) {
		return fw.get_memory_map_status;
	}
	needed = fw.descriptor_count * fw.descriptor_size;
	*descriptor_size = fw.descriptor_size;
	*descriptor_version = 1;
	if (fw.always_too_small || *map_size < needed) {
		*map_size = needed;
		return JP_UEFI_BUFFER_TOO_SMALL;
	}
	for (i = 0; i < fw.descriptor_count; i++) {
		memcpy((uint8_t *)map + i * fw.descriptor_size, &fw.descriptors[i], sizeof fw.descriptors[i]);
	}
	*map_size = needed + fw.map_size_extra;
	fw.key++;
	*map_key = fw.key;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_allocate_pool(JPUefiUintn memory_type, JPUefiUintn size, void **buffer)
{
	unsigned int i;

	(void)memory_type;
	touch();
	fw.allocate_pool_calls++;
	fw.last_pool_size = size;
	if (fw.allocate_pool_status != JP_UEFI_SUCCESS) {
		return fw.allocate_pool_status;
	}
	*buffer = malloc(size == 0 ? 1 : size);
	assert(*buffer != NULL);
	for (i = 0; i < fw.extra_after_pool; i++) {
		add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x40000000ULL + fw.descriptor_count * 0x10000ULL, 1);
	}
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_free_pool(void *buffer)
{
	touch();
	fw.free_pool_calls++;
	free(buffer);
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_handle_protocol(JPUefiHandle handle, const JPUefiGuid *protocol, void **interface)
{
	static const JPUefiGuid loaded_image = JP_UEFI_LOADED_IMAGE_GUID;
	static const JPUefiGuid file_system = JP_UEFI_SIMPLE_FILE_SYSTEM_GUID;

	touch();
	if (handle == &fw.image_handle_object && same_guid(protocol, &loaded_image)) {
		*interface = &fw.loaded_image;
		return JP_UEFI_SUCCESS;
	}
	if (handle == &fw.device_handle_object && same_guid(protocol, &file_system)) {
		*interface = &fw.file_system;
		return JP_UEFI_SUCCESS;
	}
	return JP_UEFI_UNSUPPORTED;
}

static JPUefiStatus JP_UEFI_API fake_exit_boot_services(JPUefiHandle image_handle, JPUefiUintn map_key)
{
	touch();
	assert(image_handle == &fw.image_handle_object);
	if (fw.probe_context != NULL) {
		JPBootInputEvent probe_event;

		fw.probe_input_result = jp_uefi_read_input(fw.probe_context, &probe_event);
		fw.probe_ran = true;
	}
	if (fw.exit_calls < MAX_LOG) {
		fw.exit_key[fw.exit_calls] = map_key;
	}
	fw.exit_calls++;
	if (fw.exit_status != JP_UEFI_SUCCESS) {
		return fw.exit_status;
	}
	if (fw.exit_refusals > 0) {
		fw.exit_refusals--;
		return JP_UEFI_INVALID_PARAMETER;
	}
	if (map_key != fw.key) {
		return JP_UEFI_INVALID_PARAMETER;            /* a stale key */
	}
	fw.exited = true;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_locate_protocol(const JPUefiGuid *protocol, void *registration, void **interface)
{
	static const JPUefiGuid gop_guid = JP_UEFI_GRAPHICS_OUTPUT_GUID;

	(void)registration;
	touch();
	if (same_guid(protocol, &gop_guid) && fw.gop_present) {
		*interface = &fw.gop;
		return JP_UEFI_SUCCESS;
	}
	return JP_UEFI_NOT_FOUND;
}

static JPUefiStatus JP_UEFI_API fake_open_volume(JPUefiSimpleFileSystem *self, JPUefiFile **root)
{
	(void)self;
	touch();
	*root = &fw.root;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_open(JPUefiFile *self, JPUefiFile **new_handle,
	const JPUefiChar16 *file_name, uint64_t open_mode, uint64_t attributes)
{
	size_t n = 0;

	(void)self;
	(void)attributes;
	touch();
	while (file_name[n] != 0 && n < 63) {
		fw.opened_path[n] = file_name[n];
		n++;
	}
	fw.opened_path[n] = 0;
	fw.opened_mode = open_mode;
	if (!fw.file_present) {
		return JP_UEFI_NOT_FOUND;
	}
	fw.read_position = 0;
	*new_handle = &fw.file;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_close(JPUefiFile *self)
{
	(void)self;
	touch();
	fw.files_closed++;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_read(JPUefiFile *self, JPUefiUintn *size, void *buffer)
{
	size_t available;
	size_t n;

	(void)self;
	touch();
	if (fw.read_status != JP_UEFI_SUCCESS) {
		return fw.read_status;
	}
	available = fw.read_limit - fw.read_position;
	n = *size;
	if (n > FILE_CHUNK) {
		n = FILE_CHUNK;                           /* the firmware returns small pieces */
	}
	if (n > available) {
		n = available;
	}
	memcpy(buffer, fw.file_bytes + fw.read_position, n);
	fw.read_position += n;
	*size = n;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_get_info(JPUefiFile *self, const JPUefiGuid *type, JPUefiUintn *size, void *buffer)
{
	static const JPUefiGuid info_guid = JP_UEFI_FILE_INFO_GUID;
	JPUefiFileInfo *info = (JPUefiFileInfo *)buffer;
	JPUefiUintn needed = sizeof(JPUefiFileInfo);

	(void)self;
	touch();
	assert(same_guid(type, &info_guid));
	if (*size < needed) {
		*size = needed;
		return JP_UEFI_BUFFER_TOO_SMALL;
	}
	memset(info, 0, needed);
	info->size = needed;
	info->file_size = fw.file_size;
	*size = needed;
	return JP_UEFI_SUCCESS;
}

static JPUefiStatus JP_UEFI_API fake_read_key_stroke(JPUefiSimpleTextInput *self, JPUefiInputKey *key)
{
	(void)self;
	touch();
	if (fw.key_next >= fw.key_count) {
		return JP_UEFI_NOT_READY;
	}
	*key = fw.keys[fw.key_next].key;
	return fw.keys[fw.key_next++].status;
}

static void push_key(uint16_t scan_code, JPUefiChar16 character)
{
	fw.keys[fw.key_count].status = JP_UEFI_SUCCESS;
	fw.keys[fw.key_count].key.scan_code = scan_code;
	fw.keys[fw.key_count].key.unicode_char = character;
	fw.key_count++;
}

/* ================= a fresh firmware and a provider context ================= */

static JPUefiSystemTable system_table;
static JPUefiBootServices boot_services;

static void reset_firmware(void)
{
	size_t i;

	memset(&fw, 0, sizeof fw);
	memset(&system_table, 0, sizeof system_table);
	memset(&boot_services, 0, sizeof boot_services);

	fw.descriptor_size = DESCRIPTOR_STRIDE;
	boot_services.allocate_pages = fake_allocate_pages;
	boot_services.free_pages = fake_free_pages;
	boot_services.get_memory_map = fake_get_memory_map;
	boot_services.allocate_pool = fake_allocate_pool;
	boot_services.free_pool = fake_free_pool;
	boot_services.handle_protocol = fake_handle_protocol;
	boot_services.exit_boot_services = fake_exit_boot_services;
	boot_services.locate_protocol = fake_locate_protocol;

	system_table.header.signature = JP_UEFI_SYSTEM_TABLE_SIGNATURE;
	system_table.boot_services = &boot_services;
	fw.input.read_key_stroke = fake_read_key_stroke;
	system_table.con_in = &fw.input;

	fw.loaded_image.device_handle = &fw.device_handle_object;
	fw.file_system.open_volume = fake_open_volume;
	fw.root.open = fake_open;
	fw.root.close = fake_close;
	fw.file.read = fake_read;
	fw.file.close = fake_close;
	fw.file.get_info = fake_get_info;

	fw.gop.mode = &fw.gop_mode;
	fw.gop_mode.info = &fw.gop_info;
	fw.gop_present = true;
	fw.gop_mode.frame_buffer_base = 0x80000000ULL;
	fw.gop_info.horizontal_resolution = 1024;
	fw.gop_info.vertical_resolution = 768;
	fw.gop_info.pixels_per_scan_line = 1056;
	fw.gop_info.pixel_format = JP_UEFI_GOP_PIXEL_BGR_RESERVED_8;

	fw.file_present = true;
	fw.file_size = 40;
	fw.read_limit = 40;
	for (i = 0; i < fw.file_size; i++) {
		fw.file_bytes[i] = (uint8_t)(i + 1);
	}

	/* A small ordinary map: low usable memory, a hole, and high usable memory. */
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000, 256);
	add_descriptor(JP_UEFI_MEMORY_RESERVED, 0x1000000, 16);
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x2000000, 4096);
}

static JPUefiContext *new_context(void)
{
	void *memory = calloc(1, jp_uefi_context_size());

	assert(memory != NULL);
	return jp_uefi_context_setup(memory, &fw.image_handle_object, &system_table);
}

static JPUefiContext *initialized_context(void)
{
	JPUefiContext *context = new_context();

	assert(jp_uefi_initialize(context) == JP_BOOT_SUCCESS);
	return context;
}

/* ================= tests ================= */

static void test_context_setup(void)
{
	void *memory;

	reset_firmware();
	assert(jp_uefi_context_size() > 0);
	assert(jp_uefi_context_setup(NULL, &fw.image_handle_object, &system_table) == NULL);
	memory = calloc(1, jp_uefi_context_size());
	assert(jp_uefi_context_setup(memory, NULL, &system_table) == NULL);
	assert(jp_uefi_context_setup(memory, &fw.image_handle_object, NULL) == NULL);
	assert(jp_uefi_context_setup(memory, &fw.image_handle_object, &system_table) != NULL);
	free(memory);
}

static void test_initialize(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	reset_firmware();
	assert(jp_uefi_initialize(NULL) == JP_BOOT_ERROR_INVALID_PARAM);

	context = new_context();
	assert(jp_uefi_initialize(context) == JP_BOOT_SUCCESS);
	assert(jp_uefi_initialize(context) == JP_BOOT_FAILED);   /* only once */

	/* A system table without the right signature. */
	reset_firmware();
	system_table.header.signature = 0x1234;
	context = new_context();
	assert(jp_uefi_initialize(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_NOT_INITIALIZED);   /* terminal */
	assert(jp_uefi_initialize(context) == JP_BOOT_FAILED);                    /* and cannot be tried again */

	/* No boot services. */
	reset_firmware();
	system_table.boot_services = NULL;
	context = new_context();
	assert(jp_uefi_initialize(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_boot_services_active(void)
{
	JPUefiContext *context;

	reset_firmware();
	assert(!jp_uefi_boot_services_active(NULL));
	context = new_context();
	assert(!jp_uefi_boot_services_active(context));           /* before initialize */
	assert(jp_uefi_initialize(context) == JP_BOOT_SUCCESS);
	assert(jp_uefi_boot_services_active(context));
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_SUCCESS);
	assert(!jp_uefi_boot_services_active(context));           /* after the exit */

	/* A failed transition before the exit leaves the services usable (the entry can still print). */
	reset_firmware();
	context = initialized_context();
	fw.exit_status = JP_UEFI_DEVICE_ERROR;
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_FAILED);
	assert(jp_uefi_boot_services_active(context));
}

static void test_operations_need_initialization(void)
{
	JPUefiContext *context;
	JPMemoryMap map;
	JPFramebufferInfo fb;
	JPKernelImage image;
	JPBootInputEvent event;

	reset_firmware();
	context = new_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_reserve_memory(context, 0x200000, 0x1000) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_NOT_INITIALIZED);
	assert(fw.get_memory_map_calls == 0 && fw.allocate_pages_calls == 0 && fw.exit_calls == 0);
}

/* Each firmware memory type becomes the normalized type of the specification table. */
static void test_memory_type_normalization(void)
{
	static const struct { JPUefiMemoryType type; JPMemoryRegionType expected; } table[] = {
		{ JP_UEFI_MEMORY_CONVENTIONAL, JP_MEMORY_REGION_USABLE },
		{ JP_UEFI_MEMORY_LOADER_CODE, JP_MEMORY_REGION_RESERVED },
		{ JP_UEFI_MEMORY_LOADER_DATA, JP_MEMORY_REGION_RESERVED },
		{ JP_UEFI_MEMORY_BOOT_SERVICES_CODE, JP_MEMORY_REGION_RESERVED },   /* while boot services run */
		{ JP_UEFI_MEMORY_BOOT_SERVICES_DATA, JP_MEMORY_REGION_RESERVED },
		{ JP_UEFI_MEMORY_RUNTIME_SERVICES_CODE, JP_MEMORY_REGION_FIRMWARE },
		{ JP_UEFI_MEMORY_RUNTIME_SERVICES_DATA, JP_MEMORY_REGION_FIRMWARE },
		{ JP_UEFI_MEMORY_ACPI_RECLAIM, JP_MEMORY_REGION_ACPI },
		{ JP_UEFI_MEMORY_ACPI_NVS, JP_MEMORY_REGION_ACPI },
		{ JP_UEFI_MEMORY_MAPPED_IO, JP_MEMORY_REGION_MMIO },
		{ JP_UEFI_MEMORY_MAPPED_IO_PORT_SPACE, JP_MEMORY_REGION_MMIO },
		{ JP_UEFI_MEMORY_UNUSABLE, JP_MEMORY_REGION_UNUSABLE },
		{ JP_UEFI_MEMORY_RESERVED, JP_MEMORY_REGION_RESERVED },
		{ JP_UEFI_MEMORY_PERSISTENT, JP_MEMORY_REGION_RESERVED },
		{ JP_UEFI_MEMORY_PAL_CODE, JP_MEMORY_REGION_RESERVED }
	};
	size_t i;

	for (i = 0; i < sizeof table / sizeof table[0]; i++) {
		JPUefiContext *context;
		JPMemoryMap map;

		reset_firmware();
		fw.descriptor_count = 0;
		add_descriptor(table[i].type, 0x400000, 8);
		context = initialized_context();
		assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
		assert(map.region_count == 1);
		assert(map.regions[0].base_address == 0x400000);
		assert(map.regions[0].length == 8 * 4096);
		assert(map.regions[0].type == table[i].expected);
	}
}

static void test_ordinary_map_and_descriptor_stride(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	reset_firmware();
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 3);
	assert(map.regions[0].base_address == 0x100000 && map.regions[0].length == 256 * 4096);
	assert(map.regions[1].type == JP_MEMORY_REGION_RESERVED);
	assert(map.regions[2].base_address == 0x2000000 && map.regions[2].length == 4096 * 4096ULL);
	assert(map.regions[2].type == JP_MEMORY_REGION_USABLE);

	/* A descriptor size smaller than the structure is corrupted data. */
	reset_firmware();
	fw.descriptor_size = 24;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_touching_regions_of_the_same_type_are_merged(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	reset_firmware();
	fw.descriptor_count = 0;
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000, 16);          /* [0x100000, 0x110000) */
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x110000, 16);          /* touches: merged */
	add_descriptor(JP_UEFI_MEMORY_ACPI_RECLAIM, 0x120000, 16);          /* touches, other type: kept */
	add_descriptor(JP_UEFI_MEMORY_ACPI_RECLAIM, 0x140000, 16);          /* same type but a gap: kept */
	add_descriptor(JP_UEFI_MEMORY_LOADER_DATA, 0x150000, 16);           /* touches, normalized to RESERVED */
	add_descriptor(JP_UEFI_MEMORY_BOOT_SERVICES_DATA, 0x160000, 16);    /* RESERVED too: merged with the previous */
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 4);
	assert(map.regions[0].base_address == 0x100000 && map.regions[0].length == 32 * 4096);
	assert(map.regions[1].type == JP_MEMORY_REGION_ACPI && map.regions[1].length == 16 * 4096);
	assert(map.regions[2].type == JP_MEMORY_REGION_ACPI && map.regions[2].base_address == 0x140000);
	assert(map.regions[3].type == JP_MEMORY_REGION_RESERVED && map.regions[3].length == 32 * 4096);
}

static void test_too_many_regions_after_merging(void)
{
	JPUefiContext *context;
	JPMemoryMap map;
	size_t i;

	reset_firmware();
	fw.descriptor_count = 0;
	for (i = 0; i < JP_MAX_MEMORY_REGIONS; i++) {
		add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000 + i * 0x10000, 1);   /* gaps: nothing merges */
	}
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);     /* exactly the capacity */
	assert(map.region_count == JP_MAX_MEMORY_REGIONS);

	reset_firmware();
	fw.descriptor_count = 0;
	for (i = 0; i < JP_MAX_MEMORY_REGIONS + 1; i++) {
		add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000 + i * 0x10000, 1);
	}
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);
}

static void test_zero_page_descriptors_are_skipped(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	reset_firmware();
	fw.descriptor_count = 0;
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000, 0);
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x200000, 4);
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 1 && map.regions[0].base_address == 0x200000);
}

static void test_map_buffer_grows_and_failures(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	/* Allocating the buffer itself adds descriptors: the provider keeps room for them. */
	reset_firmware();
	fw.extra_after_pool = 1;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(map.region_count == 4);                         /* the descriptor that the allocation added is there */
	assert(fw.get_memory_map_calls >= 2);

	/* A firmware that always says "too small" ends after a few tries with a failure. */
	reset_firmware();
	fw.always_too_small = true;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_FAILED);
	assert(fw.get_memory_map_calls <= 6);

	/* Any other firmware error. */
	reset_firmware();
	fw.get_memory_map_status = JP_UEFI_DEVICE_ERROR;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_FAILED);

	/* No memory for the buffer. */
	reset_firmware();
	fw.allocate_pool_status = JP_UEFI_OUT_OF_RESOURCES;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_FAILED);
}

static void test_malformed_maps_are_rejected(void)
{
	JPUefiContext *context;
	JPMemoryMap map;

	/* The map size is not a whole number of descriptors. */
	reset_firmware();
	fw.map_size_extra = 5;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* A descriptor with more pages than the address space holds. */
	reset_firmware();
	fw.descriptor_count = 0;
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000, UINT64_MAX / 4096 + 1);
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* A descriptor whose end passes the top of the address space. */
	reset_firmware();
	fw.descriptor_count = 0;
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, UINT64_MAX - 0x10000, 32);
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* "Too small" together with a descriptor size of zero cannot be repaired by growing. */
	reset_firmware();
	fw.always_too_small = true;
	fw.descriptor_size = 0;
	context = initialized_context();
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_FAILED);
	assert(fw.get_memory_map_calls == 1);
}

static void test_framebuffer(void)
{
	JPUefiContext *context;
	JPFramebufferInfo fb;

	reset_firmware();
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(fb.base_address == 0x80000000ULL);
	assert(fb.width == 1024 && fb.height == 768);
	assert(fb.pitch == 1056 * 4);                         /* pixels per scan line * 4 bytes */
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_BGR_888);

	reset_firmware();
	fw.gop_info.pixel_format = JP_UEFI_GOP_PIXEL_RGB_RESERVED_8;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_RGB_888);

	/* Pixel formats the generic code cannot use are reported as unknown. */
	reset_firmware();
	fw.gop_info.pixel_format = JP_UEFI_GOP_PIXEL_BIT_MASK;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_UNKNOWN);
	fw.gop_info.pixel_format = JP_UEFI_GOP_PIXEL_BLT_ONLY;
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_SUCCESS);
	assert(fb.format == JP_FRAMEBUFFER_FORMAT_UNKNOWN);

	/* A pitch that does not fit in 32 bits. */
	reset_firmware();
	fw.gop_info.pixels_per_scan_line = 0x40000001U;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_ERROR_CORRUPTED_DATA);

	/* No graphics output at all. */
	reset_firmware();
	fw.gop_present = false;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_NOT_DETECTED);

	/* A graphics output without a current mode. */
	reset_firmware();
	fw.gop.mode = NULL;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_FAILED);
	reset_firmware();
	fw.gop_mode.info = NULL;
	context = initialized_context();
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_FAILED);
}

static void test_kernel_image(void)
{
	static const JPUefiChar16 expected[] = u"\\EFI\\JustOS\\kernel.elf";
	JPUefiContext *context;
	JPKernelImage image;
	size_t i;

	reset_firmware();
	context = initialized_context();
	memset(&image, 0, sizeof image);
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_SUCCESS);
	assert(image.size == 40 && image.buffer != NULL);
	for (i = 0; i < 40; i++) {
		assert(((const uint8_t *)image.buffer)[i] == (uint8_t)(i + 1));
	}
	assert(memcmp(fw.opened_path, expected, sizeof expected) == 0);   /* the fixed path, nothing else */
	assert(fw.opened_mode == JP_UEFI_FILE_MODE_READ);
	assert(fw.files_closed == 2);                                      /* the file and the volume */

	/* No file. */
	reset_firmware();
	fw.file_present = false;
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_NOT_DETECTED);

	/* The file cannot be read completely: it ends early, or the read fails. */
	reset_firmware();
	fw.read_limit = 30;
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_FAILED);
	assert(fw.free_pool_calls >= 1);                          /* the buffer is given back */
	reset_firmware();
	fw.read_status = JP_UEFI_DEVICE_ERROR;
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_FAILED);

	/* The device of the boot application has no file system. */
	reset_firmware();
	fw.loaded_image.device_handle = &fw.image_handle_object;      /* the wrong handle: no file system */
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_FAILED);

	/* No memory for the image. */
	reset_firmware();
	fw.allocate_pool_status = JP_UEFI_OUT_OF_RESOURCES;
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_FAILED);

	/* An empty file is read successfully with size 0 (the generic code rejects it). */
	reset_firmware();
	fw.file_size = 0;
	fw.read_limit = 0;
	context = initialized_context();
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_SUCCESS);
	assert(image.size == 0);
	assert(fw.last_pool_size >= 1);                           /* the pointer stays valid */
}

static void test_input(void)
{
	static const struct { uint16_t scan; JPUefiChar16 ch; JPBootInputKind kind; JPBootInputKey key; uint32_t character; } table[] = {
		{ 0x01, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_UP, 0 },
		{ 0x02, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_DOWN, 0 },
		{ 0x03, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_RIGHT, 0 },
		{ 0x04, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_LEFT, 0 },
		{ 0x17, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_ESCAPE, 0 },
		{ 0x14, 0, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_F10, 0 },
		{ 0x00, 0x0D, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_ENTER, 0 },
		{ 0x00, 0x08, JP_BOOT_INPUT_KEY, JP_BOOT_KEY_BACKSPACE, 0 },
		{ 0x00, 'a', JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, 'a' },
		{ 0x00, '~', JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, '~' },
		{ 0x00, ' ', JP_BOOT_INPUT_CHARACTER, JP_BOOT_KEY_UP, ' ' },
		{ 0x00, 0x7F, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },      /* not printable */
		{ 0x00, 0x1F, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },
		{ 0x0B, 0, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },         /* a key we cannot map (F1) */
		{ 0x0B, 'a', JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 },       /* a special key never becomes a character */
		{ 0x00, 0x00E9, JP_BOOT_INPUT_NONE, JP_BOOT_KEY_UP, 0 }     /* not ASCII */
	};
	JPUefiContext *context;
	JPBootInputEvent event;
	size_t i;

	for (i = 0; i < sizeof table / sizeof table[0]; i++) {
		reset_firmware();
		push_key(table[i].scan, table[i].ch);
		context = initialized_context();
		memset(&event, 0xFF, sizeof event);
		assert(jp_uefi_read_input(context, &event) == JP_BOOT_SUCCESS);
		assert(event.kind == table[i].kind);
		if (table[i].kind == JP_BOOT_INPUT_KEY) {
			assert(event.key == table[i].key);
		}
		if (table[i].kind == JP_BOOT_INPUT_CHARACTER) {
			assert(event.character == table[i].character);
		}
	}

	/* Nothing pending is no event, not an error. */
	reset_firmware();
	context = initialized_context();
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_SUCCESS);
	assert(event.kind == JP_BOOT_INPUT_NONE);

	/* No input device. */
	reset_firmware();
	system_table.con_in = NULL;
	context = initialized_context();
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_DETECTED);

	/* A firmware error. */
	reset_firmware();
	fw.keys[0].status = JP_UEFI_DEVICE_ERROR;
	fw.key_count = 1;
	context = initialized_context();
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_FAILED);

	/* NULL arguments. */
	assert(jp_uefi_read_input(NULL, &event) == JP_BOOT_ERROR_INVALID_PARAM);
	context = initialized_context();
	assert(jp_uefi_read_input(context, NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

static void test_reserve_memory(void)
{
	JPUefiContext *context;

	reset_firmware();
	context = initialized_context();
	assert(jp_uefi_reserve_memory(context, 0x200000, 0x3000) == JP_BOOT_SUCCESS);
	assert(fw.allocate_pages_calls == 1);
	assert(fw.ap_type[0] == JP_UEFI_ALLOCATE_ADDRESS);
	assert(fw.ap_memory_type[0] == JP_UEFI_MEMORY_LOADER_DATA);
	assert(fw.ap_pages[0] == 3 && fw.ap_address[0] == 0x200000);

	/* The range is rounded outward to whole pages. */
	assert(jp_uefi_reserve_memory(context, 0x300010, 0x1000) == JP_BOOT_SUCCESS);   /* [0x300010, 0x301010) */
	assert(fw.ap_address[1] == 0x300000 && fw.ap_pages[1] == 2);
	assert(jp_uefi_reserve_memory(context, 0x400000, 1) == JP_BOOT_SUCCESS);
	assert(fw.ap_pages[2] == 1);

	/* Invalid arguments. */
	assert(jp_uefi_reserve_memory(NULL, 0x200000, 0x1000) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_uefi_reserve_memory(context, 0x200000, 0) == JP_BOOT_ERROR_INVALID_PARAM);
	assert(jp_uefi_reserve_memory(context, UINT64_MAX - 10, 0x1000) == JP_BOOT_ERROR_INVALID_PARAM);   /* overflows */

	assert(jp_uefi_reserve_memory(context, UINT64_MAX - 0x100, 0x80) == JP_BOOT_ERROR_INVALID_PARAM);   /* rounding would overflow */
	assert(fw.allocate_pages_calls == 3);

	/* The firmware refuses. */
	fw.allocate_pages_status = JP_UEFI_NOT_FOUND;
	assert(jp_uefi_reserve_memory(context, 0x500000, 0x1000) == JP_BOOT_FAILED);
	fw.allocate_pages_status = JP_UEFI_OUT_OF_RESOURCES;
	assert(jp_uefi_reserve_memory(context, 0x500000, 0x1000) == JP_BOOT_FAILED);
}

static void test_transition(void)
{
	JPUefiContext *context;
	JPMemoryMap map;
	JPBootInputEvent event;
	JPFramebufferInfo fb;
	JPKernelImage image;
	unsigned int calls_before;

	reset_firmware();
	fw.descriptor_count = 0;
	add_descriptor(JP_UEFI_MEMORY_CONVENTIONAL, 0x100000, 256);
	add_descriptor(JP_UEFI_MEMORY_BOOT_SERVICES_DATA, 0x300000, 16);       /* usable after the exit (a gap before it) */
	add_descriptor(JP_UEFI_MEMORY_LOADER_DATA, 0x500000, 16);               /* stays reserved */
	push_key(0, 'q');
	context = initialized_context();
	fw.probe_context = context;
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);       /* an early map first */
	assert(map.regions[1].type == JP_MEMORY_REGION_RESERVED);

	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_SUCCESS);
	assert(fw.exit_calls == 1);
	assert(fw.exit_key[0] == fw.key);            /* the key of the latest map: not stale */
	assert(fw.exited);
	/* While the exit was running (the key is held) input was already refused, and no key was consumed. */
	assert(fw.probe_ran && fw.probe_input_result == JP_BOOT_NOT_INITIALIZED);
	assert(fw.key_next == 0);

	/* The final map is served from the provider's own copy, without any firmware call. */
	calls_before = fw.get_memory_map_calls;
	memset(&map, 0, sizeof map);
	assert(jp_uefi_get_memory_map(context, &map) == JP_BOOT_SUCCESS);
	assert(fw.get_memory_map_calls == calls_before);
	assert(map.region_count == 3);
	assert(map.regions[1].type == JP_MEMORY_REGION_USABLE);                  /* boot services memory is given back */
	assert(map.regions[2].type == JP_MEMORY_REGION_RESERVED);

	/* After the exit nothing may call the firmware: every other operation refuses. */
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_get_framebuffer(context, &fb) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_get_kernel_image(context, &image) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_reserve_memory(context, 0x600000, 0x1000) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_NOT_INITIALIZED);
	assert(jp_uefi_initialize(context) == JP_BOOT_FAILED);
	assert(fw.calls_after_exit == 0);
}

static void test_transition_stale_key(void)
{
	JPUefiContext *context;

	/* The first exit is refused (the map changed): a fresh map and key, then it works. */
	reset_firmware();
	fw.exit_refusals = 1;
	context = initialized_context();
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_SUCCESS);
	assert(fw.exit_calls == 2);
	assert(fw.exit_key[1] != fw.exit_key[0]);     /* the stale key is never used a second time */
	assert(fw.exit_key[1] == fw.key);
	assert(fw.get_memory_map_calls >= 2);

	/* Two refusals: the handoff fails and the provider is terminal. */
	reset_firmware();
	fw.exit_refusals = 2;
	context = initialized_context();
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_FAILED);
	assert(fw.exit_calls == 2);
	assert(!fw.exited);
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_NOT_INITIALIZED);
	{
		JPBootInputEvent event;

		assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	}
}

static void test_transition_failures(void)
{
	{
		JPUefiContext *other;

		/* An error other than "stale key" ends the transition at once. */
		reset_firmware();
		fw.exit_status = JP_UEFI_DEVICE_ERROR;
		other = initialized_context();
		assert(jp_uefi_prepare_handoff(other) == JP_BOOT_FAILED);
		assert(fw.exit_calls == 1);
		assert(!fw.exited);
	}

	JPUefiContext *context;
	JPBootInputEvent event;

	/* No final map. */
	reset_firmware();
	context = initialized_context();
	fw.get_memory_map_status = JP_UEFI_DEVICE_ERROR;
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_FAILED);
	assert(fw.exit_calls == 0);
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);

	/* A final map that cannot be normalized. */
	reset_firmware();
	context = initialized_context();
	fw.descriptor_size = 24;
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_ERROR_CORRUPTED_DATA);
	assert(fw.exit_calls == 0);

	/* NULL. */
	assert(jp_uefi_prepare_handoff(NULL) == JP_BOOT_ERROR_INVALID_PARAM);
}

/* Input is refused as soon as the final map has been taken, and never reaches the firmware. */
static void test_input_is_refused_from_the_final_map_on(void)
{
	JPUefiContext *context;
	JPBootInputEvent event;

	reset_firmware();
	push_key(0, 'x');
	context = initialized_context();
	fw.get_memory_map_status = JP_UEFI_DEVICE_ERROR;              /* the transition stops after the state change */
	assert(jp_uefi_prepare_handoff(context) == JP_BOOT_FAILED);
	assert(jp_uefi_read_input(context, &event) == JP_BOOT_NOT_INITIALIZED);
	assert(fw.key_next == 0);                                      /* the key was not even asked for */
}

int main(void)
{
	test_context_setup();
	test_initialize();
	test_boot_services_active();
	test_operations_need_initialization();
	test_memory_type_normalization();
	test_ordinary_map_and_descriptor_stride();
	test_touching_regions_of_the_same_type_are_merged();
	test_too_many_regions_after_merging();
	test_zero_page_descriptors_are_skipped();
	test_map_buffer_grows_and_failures();
	test_malformed_maps_are_rejected();
	test_framebuffer();
	test_kernel_image();
	test_input();
	test_reserve_memory();
	test_transition();
	test_transition_stale_key();
	test_transition_failures();
	test_input_is_refused_from_the_final_map_on();
	return 0;
}
