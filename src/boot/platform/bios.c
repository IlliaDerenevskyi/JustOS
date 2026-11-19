/*
 * bios.c - the BIOS Stage 1 provider (x86_64).
 *
 * Purpose:
 *   Implements the provider operations of bios.h. BIOS services can be called
 *   only in real mode, so the 16-bit loader has already called them and left
 *   the results in the boot data block (bios_block.h). This provider only
 *   reads that block; it calls no BIOS service. The one thing it does itself
 *   is reading the PS/2 keyboard controller, through jp_bios_port_read8().
 *
 * Responsibilities:
 *   - check the block (magic text, version, entry count, A20 flag);
 *   - normalize the E820 entries into JPMemoryRegion values and cut out the
 *     ranges that the boot environment owns (the table in "Decided rules (the
 *     BIOS provider and its boot chain)" of milestone0.md);
 *   - hand out the framebuffer and the kernel image exactly as the loader
 *     recorded them, or report NOT_DETECTED when the loader did not get them;
 *   - map PS/2 scan codes (set 1) to normalized input events;
 *   - record reservations and publish the final memory map.
 *
 * Limitations:
 *   Memory from 4 GiB upward is not offered to the kernel in the BIOS path,
 *   because the loader maps only the first 4 GiB. The code is freestanding: no
 *   libc, so no memcpy (large structures are copied by hand).
 *
 * Depends on:
 *   justos/boot/platform/bios.h, bios_block.h, and the private bios_io.h.
 *
 * Must not depend on:
 *   Generic Stage 1 modules (only the shared types of boot_types.h).
 */

#include "justos/boot/platform/bios.h"
#include "justos/boot/platform/bios_block.h"

#include "bios_io.h"

/* ---- Private constants ---- */

enum {
	JP_BIOS_MAX_RESERVATIONS = 64,
	JP_BIOS_MAX_CUTS = JP_BIOS_MAX_RESERVATIONS + 2,

	JP_BIOS_E820_USABLE = 1,
	JP_BIOS_E820_RESERVED = 2,
	JP_BIOS_E820_ACPI_RECLAIMABLE = 3,
	JP_BIOS_E820_ACPI_NVS = 4,
	JP_BIOS_E820_BAD_MEMORY = 5,

	JP_BIOS_PORT_KEYBOARD_DATA = 0x60,
	JP_BIOS_PORT_KEYBOARD_STATUS = 0x64,
	JP_BIOS_STATUS_OUTPUT_FULL = 0x01,       /* bit 0: a byte is waiting */
	JP_BIOS_STATUS_FROM_MOUSE = 0x20,        /* bit 5: the waiting byte comes from the mouse */
	JP_BIOS_STATUS_NO_CONTROLLER = 0xFF,     /* nothing drives the bus: there is no controller */
	JP_BIOS_SCAN_BREAK = 0x80,               /* bit 7: the key was released */

	JP_BIOS_SCAN_ESCAPE = 0x01,
	JP_BIOS_SCAN_BACKSPACE = 0x0E,
	JP_BIOS_SCAN_ENTER = 0x1C,
	JP_BIOS_SCAN_F10 = 0x44,
	JP_BIOS_SCAN_UP = 0x48,
	JP_BIOS_SCAN_LEFT = 0x4B,
	JP_BIOS_SCAN_RIGHT = 0x4D,
	JP_BIOS_SCAN_DOWN = 0x50
};

#define JP_BIOS_LOW_LIMIT 0x00800000ULL        /* the first 8 MiB belong to the boot environment */
#define JP_BIOS_HIGH_LIMIT 0x100000000ULL      /* the loader maps only the first 4 GiB */

typedef enum JPBiosState {
	JP_BIOS_STATE_UNINITIALIZED = 0,
	JP_BIOS_STATE_INITIALIZED,
	JP_BIOS_STATE_ACTIVE,
	JP_BIOS_STATE_HANDOFF_PREPARED
} JPBiosState;

/* A half-open physical range [start, end). */
typedef struct JPBiosRange {
	uint64_t start;
	uint64_t end;
} JPBiosRange;

/* A row of keys on the keyboard: consecutive scan codes that give consecutive characters of a string. */
typedef struct JPBiosKeyRow {
	uint8_t first_scan;
	const char *characters;
} JPBiosKeyRow;

/* The provider context. The entry point gives jp_bios_context_size() zeroed bytes for it. */
struct JPBiosContext {
	const JPBiosBlock *block;
	JPBiosState state;
	JPBiosRange reservations[JP_BIOS_MAX_RESERVATIONS];
	size_t reservation_count;
	JPMemoryMap final_map;                  /* kept for the kernel: the block may be overwritten later */
};

/* The letter and digit rows (scan code set 1). */
static const JPBiosKeyRow jp_bios_key_rows[] = {
	{ 0x02, "1234567890" },
	{ 0x10, "qwertyuiop" },
	{ 0x1E, "asdfghjkl" },
	{ 0x2C, "zxcvbnm" }
};

/* ---- Public API ---- */

JPBootResult jp_bios_initialize(JPBiosContext *context);
JPBootResult jp_bios_get_memory_map(JPBiosContext *context, JPMemoryMap *memory_map);
JPBootResult jp_bios_get_framebuffer(JPBiosContext *context, JPFramebufferInfo *framebuffer);
JPBootResult jp_bios_get_kernel_image(JPBiosContext *context, JPKernelImage *image);
JPBootResult jp_bios_prepare_handoff(JPBiosContext *context);
JPBootResult jp_bios_read_input(JPBiosContext *context, JPBootInputEvent *event);
JPBootResult jp_bios_reserve_memory(JPBiosContext *context, uint64_t physical, uint64_t length);
size_t jp_bios_context_size(void);
JPBiosContext *jp_bios_context_setup(void *memory, const void *block);

/* ---- Private helpers: state ---- */

static bool state_permits_use(const JPBiosContext *context)
{
	return context->state == JP_BIOS_STATE_INITIALIZED ||
	       context->state == JP_BIOS_STATE_ACTIVE;
}

/* A successful operation moves INITIALIZED to ACTIVE. */
static void mark_active(JPBiosContext *context)
{
	context->state = JP_BIOS_STATE_ACTIVE;
}

/* ---- Private helpers: memory map ---- */

static uint64_t align_down_page(uint64_t value)
{
	return value & ~((uint64_t)JP_BOOT_PAGE_SIZE - 1);
}

static JPMemoryRegionType normalize_memory_type(uint32_t type)
{
	switch (type) {
	case JP_BIOS_E820_USABLE:
		return JP_MEMORY_REGION_USABLE;
	case JP_BIOS_E820_ACPI_RECLAIMABLE:
	case JP_BIOS_E820_ACPI_NVS:
		return JP_MEMORY_REGION_ACPI;
	case JP_BIOS_E820_BAD_MEMORY:
		return JP_MEMORY_REGION_UNUSABLE;
	case JP_BIOS_E820_RESERVED:
	default:
		return JP_MEMORY_REGION_RESERVED;
	}
}

static JPBootResult append_region(JPMemoryMap *map, uint64_t base, uint64_t length, JPMemoryRegionType type)
{
	if (map->region_count >= JP_MAX_MEMORY_REGIONS) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	map->regions[map->region_count].base_address = base;
	map->regions[map->region_count].length = length;
	map->regions[map->region_count].type = type;
	map->region_count++;
	return JP_BOOT_SUCCESS;
}

/* The ranges that are cut out of usable memory: the fixed ones and the reservations. */
static size_t collect_cuts(const JPBiosContext *context, JPBiosRange *cuts)
{
	size_t count = 0;
	size_t i;

	cuts[count].start = 0;
	cuts[count].end = JP_BIOS_LOW_LIMIT;
	count++;
	cuts[count].start = JP_BIOS_HIGH_LIMIT;
	cuts[count].end = UINT64_MAX;
	count++;
	for (i = 0; i < context->reservation_count; i++) {
		cuts[count++] = context->reservations[i];
	}
	return count;
}

/*
 * Appends one usable E820 range [begin, end) with the cuts applied: the parts
 * outside every cut stay usable, the parts inside become RESERVED. Cuts that
 * overlap each other make one reserved region; cuts that only touch stay
 * separate regions.
 */
static JPBootResult append_usable_with_cuts(JPMemoryMap *map, uint64_t begin, uint64_t end,
	const JPBiosRange *cuts, size_t cut_count)
{
	uint64_t position = begin;

	while (position < end) {
		bool found = false;
		uint64_t cut_start = 0;
		uint64_t cut_end = 0;
		bool grown;
		size_t i;
		JPBootResult result;

		/* The cut that touches the rest of the range and starts first. */
		for (i = 0; i < cut_count; i++) {
			uint64_t start;

			if (cuts[i].end <= position || cuts[i].start >= end) {
				continue;
			}
			start = cuts[i].start > position ? cuts[i].start : position;
			if (!found || start < cut_start) {
				found = true;
				cut_start = start;
				cut_end = cuts[i].end < end ? cuts[i].end : end;
			}
		}
		if (!found) {
			return append_region(map, position, end - position, JP_MEMORY_REGION_USABLE);
		}
		/* Cuts that overlap this one extend it. */
		do {
			grown = false;
			for (i = 0; i < cut_count; i++) {
				uint64_t candidate = cuts[i].end < end ? cuts[i].end : end;

				if (cuts[i].start < cut_end && candidate > cut_end) {
					cut_end = candidate;
					grown = true;
				}
			}
		} while (grown);

		if (cut_start > position) {
			result = append_region(map, position, cut_start - position, JP_MEMORY_REGION_USABLE);
			if (result != JP_BOOT_SUCCESS) {
				return result;
			}
		}
		result = append_region(map, cut_start, cut_end - cut_start, JP_MEMORY_REGION_RESERVED);
		if (result != JP_BOOT_SUCCESS) {
			return result;
		}
		position = cut_end;
	}
	return JP_BOOT_SUCCESS;
}

/*
 * Builds the normalized map from the E820 entries of the block and the
 * context's reservations. NOT_DETECTED when the loader recorded no usable
 * entry; CORRUPTED_DATA for an entry that overflows or too many regions.
 */
static JPBootResult build_memory_map(const JPBiosContext *context, JPMemoryMap *map)
{
	JPBiosRange cuts[JP_BIOS_MAX_CUTS];
	size_t cut_count = collect_cuts(context, cuts);
	size_t i;

	map->state = JP_DETECTION_STATE_NOT_DETECTED;
	map->region_count = 0;
	for (i = 0; i < context->block->e820_count; i++) {
		const JPBiosE820Entry *entry = &context->block->e820[i];
		JPMemoryRegionType type;
		JPBootResult result;

		if (entry->length == 0) {
			continue;
		}
		if (entry->length > UINT64_MAX - entry->base) {
			map->state = JP_DETECTION_STATE_FAILED;
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		type = normalize_memory_type(entry->type);
		if (type == JP_MEMORY_REGION_USABLE) {
			result = append_usable_with_cuts(map, entry->base, entry->base + entry->length, cuts, cut_count);
		} else {
			result = append_region(map, entry->base, entry->length, type);
		}
		if (result != JP_BOOT_SUCCESS) {
			map->state = JP_DETECTION_STATE_FAILED;
			return result;
		}
	}
	if (map->region_count == 0) {
		return JP_BOOT_NOT_DETECTED;
	}
	map->state = JP_DETECTION_STATE_DETECTED;
	return JP_BOOT_SUCCESS;
}

static void copy_memory_map(JPMemoryMap *to, const JPMemoryMap *from)
{
	size_t i;

	to->state = from->state;
	to->region_count = from->region_count;
	for (i = 0; i < from->region_count; i++) {
		to->regions[i] = from->regions[i];
	}
}

/* ---- Private helpers: input ---- */

/* The character of a letter or digit key, or 0 if the scan code is in no row. */
static uint32_t scan_to_character(uint8_t scan)
{
	size_t row;

	for (row = 0; row < sizeof jp_bios_key_rows / sizeof jp_bios_key_rows[0]; row++) {
		const char *characters = jp_bios_key_rows[row].characters;
		size_t length = 0;

		while (characters[length] != '\0') {
			length++;
		}
		if (scan >= jp_bios_key_rows[row].first_scan &&
		    (size_t)(scan - jp_bios_key_rows[row].first_scan) < length) {
			return (uint32_t)(unsigned char)characters[scan - jp_bios_key_rows[row].first_scan];
		}
	}
	return 0;
}

/* Maps a make code to an event; anything that cannot be mapped is NONE. */
static void map_scan_code(uint8_t scan, JPBootInputEvent *event)
{
	uint32_t character;

	event->kind = JP_BOOT_INPUT_KEY;
	switch (scan) {
	case JP_BIOS_SCAN_UP: event->key = JP_BOOT_KEY_UP; return;
	case JP_BIOS_SCAN_DOWN: event->key = JP_BOOT_KEY_DOWN; return;
	case JP_BIOS_SCAN_RIGHT: event->key = JP_BOOT_KEY_RIGHT; return;
	case JP_BIOS_SCAN_LEFT: event->key = JP_BOOT_KEY_LEFT; return;
	case JP_BIOS_SCAN_ENTER: event->key = JP_BOOT_KEY_ENTER; return;
	case JP_BIOS_SCAN_ESCAPE: event->key = JP_BOOT_KEY_ESCAPE; return;
	case JP_BIOS_SCAN_F10: event->key = JP_BOOT_KEY_F10; return;
	case JP_BIOS_SCAN_BACKSPACE: event->key = JP_BOOT_KEY_BACKSPACE; return;
	default: break;
	}
	character = scan_to_character(scan);
	if (character != 0) {
		event->kind = JP_BOOT_INPUT_CHARACTER;
		event->character = character;
		return;
	}
	event->kind = JP_BOOT_INPUT_NONE;
}

/* ---- Context ---- */

size_t jp_bios_context_size(void)
{
	return sizeof(JPBiosContext);
}

/* The memory is zeroed by the caller; only the fields that matter are set. */
JPBiosContext *jp_bios_context_setup(void *memory, const void *block)
{
	JPBiosContext *context = (JPBiosContext *)memory;

	if (memory == NULL || block == NULL) {
		return NULL;
	}
	context->block = (const JPBiosBlock *)block;
	context->state = JP_BIOS_STATE_UNINITIALIZED;
	return context;
}

/* ---- Provider operations ---- */

JPBootResult jp_bios_initialize(JPBiosContext *context)
{
	const JPBiosBlock *block;
	size_t i;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->state != JP_BIOS_STATE_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
	block = context->block;
	for (i = 0; i < JP_BIOS_BLOCK_MAGIC_SIZE; i++) {
		if (block->magic[i] != JP_BIOS_BLOCK_MAGIC[i]) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
	}
	if (block->version != JP_BIOS_BLOCK_VERSION || block->e820_count > JP_BIOS_BLOCK_MAX_E820) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	if ((block->flags & JP_BIOS_BLOCK_FLAG_A20) == 0) {
		return JP_BOOT_FAILED;
	}
	context->state = JP_BIOS_STATE_INITIALIZED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_get_memory_map(JPBiosContext *context, JPMemoryMap *memory_map)
{
	JPBootResult result;

	if (context == NULL || memory_map == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->state == JP_BIOS_STATE_HANDOFF_PREPARED) {
		/* The final map: served from the provider's own copy, the block is not read any more. */
		copy_memory_map(memory_map, &context->final_map);
		return JP_BOOT_SUCCESS;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	result = build_memory_map(context, memory_map);
	if (result == JP_BOOT_SUCCESS) {
		mark_active(context);
	}
	return result;
}

JPBootResult jp_bios_get_framebuffer(JPBiosContext *context, JPFramebufferInfo *framebuffer)
{
	const JPBiosBlock *block;

	if (context == NULL || framebuffer == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	block = context->block;
	framebuffer->state = JP_DETECTION_STATE_NOT_DETECTED;
	if ((block->flags & JP_BIOS_BLOCK_FLAG_VIDEO) == 0) {
		return JP_BOOT_NOT_DETECTED;
	}
	framebuffer->state = JP_DETECTION_STATE_FAILED;
	if (block->framebuffer_base == 0 || block->width == 0 || block->height == 0 || block->pitch == 0) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	if (block->video_format == JP_BIOS_VIDEO_FORMAT_RGB) {
		framebuffer->format = JP_FRAMEBUFFER_FORMAT_RGB_888;
	} else if (block->video_format == JP_BIOS_VIDEO_FORMAT_BGR) {
		framebuffer->format = JP_FRAMEBUFFER_FORMAT_BGR_888;
	} else {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	framebuffer->base_address = block->framebuffer_base;
	framebuffer->width = block->width;
	framebuffer->height = block->height;
	framebuffer->pitch = block->pitch;
	framebuffer->state = JP_DETECTION_STATE_DETECTED;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_get_kernel_image(JPBiosContext *context, JPKernelImage *image)
{
	const JPBiosBlock *block;

	if (context == NULL || image == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	block = context->block;
	image->state = JP_DETECTION_STATE_NOT_DETECTED;
	if ((block->flags & JP_BIOS_BLOCK_FLAG_KERNEL) == 0) {
		return JP_BOOT_NOT_DETECTED;
	}
	image->state = JP_DETECTION_STATE_FAILED;
	if (block->kernel_address == 0 || block->kernel_size == 0 ||
	    block->kernel_size > UINT64_MAX - block->kernel_address) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	image->buffer = (const void *)(uintptr_t)block->kernel_address;
	image->size = (size_t)block->kernel_size;
	image->state = JP_DETECTION_STATE_DETECTED;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_read_input(JPBiosContext *context, JPBootInputEvent *event)
{
	uint8_t status;
	uint8_t scan;

	if (context == NULL || event == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	event->kind = JP_BOOT_INPUT_NONE;
	status = jp_bios_port_read8(JP_BIOS_PORT_KEYBOARD_STATUS);
	if (status == JP_BIOS_STATUS_NO_CONTROLLER) {
		return JP_BOOT_NOT_DETECTED;
	}
	if ((status & JP_BIOS_STATUS_OUTPUT_FULL) == 0) {
		return JP_BOOT_SUCCESS;
	}
	scan = jp_bios_port_read8(JP_BIOS_PORT_KEYBOARD_DATA);
	if ((status & JP_BIOS_STATUS_FROM_MOUSE) != 0 || (scan & JP_BIOS_SCAN_BREAK) != 0) {
		return JP_BOOT_SUCCESS;
	}
	map_scan_code(scan, event);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_reserve_memory(JPBiosContext *context, uint64_t physical, uint64_t length)
{
	JPMemoryMap map;
	uint64_t start;
	uint64_t end;
	bool inside = false;
	size_t i;

	if (context == NULL || length == 0 || length > UINT64_MAX - physical) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	/* Round the range outward to whole pages. */
	end = physical + length;
	if (end > UINT64_MAX - (JP_BOOT_PAGE_SIZE - 1)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	start = align_down_page(physical);
	end = align_down_page(end + JP_BOOT_PAGE_SIZE - 1);

	/* The whole range must lie inside one usable region of the current map. */
	if (build_memory_map(context, &map) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	for (i = 0; i < map.region_count; i++) {
		const JPMemoryRegion *region = &map.regions[i];

		if (region->type == JP_MEMORY_REGION_USABLE && region->base_address <= start &&
		    end <= region->base_address + region->length) {
			inside = true;
			break;
		}
	}
	if (!inside || context->reservation_count >= JP_BIOS_MAX_RESERVATIONS) {
		return JP_BOOT_FAILED;
	}
	context->reservations[context->reservation_count].start = start;
	context->reservations[context->reservation_count].end = end;
	context->reservation_count++;
	mark_active(context);
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_bios_prepare_handoff(JPBiosContext *context)
{
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	/* There is no firmware to leave: the final map is the current map with every reservation applied. */
	result = build_memory_map(context, &context->final_map);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	context->state = JP_BIOS_STATE_HANDOFF_PREPARED;
	return JP_BOOT_SUCCESS;
}
