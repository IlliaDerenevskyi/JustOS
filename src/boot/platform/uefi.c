/*
 * uefi.c - the UEFI Stage 1 provider (x86_64 and AArch64).
 *
 * Purpose:
 *   Implements the provider operations of uefi.h on top of the firmware boot
 *   services: memory map, framebuffer (graphics output), kernel file, input,
 *   memory reservation, and the transition (ExitBootServices).
 *
 * Responsibilities:
 *   - keep the provider lifecycle truthful: a state changes only after the
 *     firmware operation really happened;
 *   - normalize firmware descriptors into JPMemoryRegion values (the table in
 *     "Decided rules (the UEFI provider)" of milestone0.md);
 *   - never call a firmware service after ExitBootServices succeeded, and
 *     refuse input from the moment the final memory-map key is held.
 *
 * Limitations:
 *   Test and boot code both use only these entry points; no UEFI type appears
 *   in a public header. The code is freestanding: no libc, no heap except the
 *   firmware pool.
 *
 * Depends on:
 *   justos/boot/platform/uefi.h and the private uefi_defs.h.
 *
 * Must not depend on:
 *   Generic Stage 1 modules (only the shared types of boot_types.h).
 */

#include "justos/boot/platform/uefi.h"

#include "uefi_defs.h"

/* ---- Private constants ---- */

enum {
	JP_UEFI_MAP_ATTEMPTS = 4,            /* tries to get the memory map into a growing buffer */
	JP_UEFI_MAP_SPARE_DESCRIPTORS = 2,   /* room for descriptors that the allocation itself adds */
	JP_UEFI_EXIT_ATTEMPTS = 2,           /* the first exit and one retry with a fresh key */
	JP_UEFI_INFO_BUFFER_SIZE = 256,
	JP_UEFI_BYTES_PER_PIXEL = 4,

	JP_UEFI_SCAN_UP = 0x01,
	JP_UEFI_SCAN_DOWN = 0x02,
	JP_UEFI_SCAN_RIGHT = 0x03,
	JP_UEFI_SCAN_LEFT = 0x04,
	JP_UEFI_SCAN_F10 = 0x14,
	JP_UEFI_SCAN_ESCAPE = 0x17,
	JP_UEFI_CHAR_BACKSPACE = 0x08,
	JP_UEFI_CHAR_ENTER = 0x0D,
	JP_UEFI_CHAR_FIRST_PRINTABLE = 0x20,
	JP_UEFI_CHAR_LAST_PRINTABLE = 0x7E
};

typedef enum JPUefiState {
	JP_UEFI_STATE_UNINITIALIZED = 0,
	JP_UEFI_STATE_INITIALIZED,
	JP_UEFI_STATE_ACTIVE,
	JP_UEFI_STATE_FINAL_MAP_ACQUIRED,
	JP_UEFI_STATE_EXIT_BOOT_SERVICES_READY,
	JP_UEFI_STATE_EXITED,
	JP_UEFI_STATE_FAILED
} JPUefiState;

/* The provider context. The entry point allocates jp_uefi_context_size() zeroed bytes for it. */
struct JPUefiContext {
	JPUefiHandle image_handle;
	JPUefiSystemTable *system_table;
	JPUefiBootServices *boot_services;
	JPUefiState state;

	JPUefiMemoryDescriptor *map_buffer;     /* firmware pool memory, loader data */
	JPUefiUintn map_buffer_size;            /* its capacity in bytes */
	JPUefiUintn map_size;                   /* the bytes of the latest map */
	JPUefiUintn map_key;
	JPUefiUintn descriptor_size;
	uint32_t descriptor_version;

	JPMemoryMap final_map;                  /* kept for the kernel after the exit */
	bool final_map_valid;
};

/* ---- Public API ---- */

JPBootResult jp_uefi_initialize(JPUefiContext *context);
JPBootResult jp_uefi_get_memory_map(JPUefiContext *context, JPMemoryMap *memory_map);
JPBootResult jp_uefi_get_framebuffer(JPUefiContext *context, JPFramebufferInfo *framebuffer);
JPBootResult jp_uefi_get_kernel_image(JPUefiContext *context, JPKernelImage *image);
JPBootResult jp_uefi_prepare_handoff(JPUefiContext *context);
JPBootResult jp_uefi_read_input(JPUefiContext *context, JPBootInputEvent *event);
JPBootResult jp_uefi_reserve_memory(JPUefiContext *context, uint64_t physical, uint64_t length);
size_t jp_uefi_context_size(void);
JPUefiContext *jp_uefi_context_setup(void *memory, void *image_handle, void *system_table);
bool jp_uefi_boot_services_active(const JPUefiContext *context);

/* ---- Private helpers: state ---- */

static bool state_permits_use(const JPUefiContext *context)
{
	return context->state == JP_UEFI_STATE_INITIALIZED ||
	       context->state == JP_UEFI_STATE_ACTIVE;
}

static JPBootResult fail_provider(JPUefiContext *context, JPBootResult result)
{
	context->state = JP_UEFI_STATE_FAILED;
	return result;
}

/* ---- Private helpers: memory map ---- */

static uint64_t align_down_page(uint64_t value)
{
	return value & ~(JP_UEFI_PAGE_SIZE - 1);
}

/* The normalized type of a firmware memory type (see the table in the specification). */
static JPMemoryRegionType normalize_memory_type(uint32_t type, bool final_map)
{
	switch (type) {
	case JP_UEFI_MEMORY_CONVENTIONAL:
		return JP_MEMORY_REGION_USABLE;
	case JP_UEFI_MEMORY_BOOT_SERVICES_CODE:
	case JP_UEFI_MEMORY_BOOT_SERVICES_DATA:
		/* The firmware gives this memory back at the exit: usable only in the final map. */
		return final_map ? JP_MEMORY_REGION_USABLE : JP_MEMORY_REGION_RESERVED;
	case JP_UEFI_MEMORY_RUNTIME_SERVICES_CODE:
	case JP_UEFI_MEMORY_RUNTIME_SERVICES_DATA:
		return JP_MEMORY_REGION_FIRMWARE;
	case JP_UEFI_MEMORY_ACPI_RECLAIM:
	case JP_UEFI_MEMORY_ACPI_NVS:
		return JP_MEMORY_REGION_ACPI;
	case JP_UEFI_MEMORY_MAPPED_IO:
	case JP_UEFI_MEMORY_MAPPED_IO_PORT_SPACE:
		return JP_MEMORY_REGION_MMIO;
	case JP_UEFI_MEMORY_UNUSABLE:
		return JP_MEMORY_REGION_UNUSABLE;
	default:
		/* Loader code and data, reserved, persistent, PAL code, and every other type. */
		return JP_MEMORY_REGION_RESERVED;
	}
}

/*
 * Turns the raw firmware map of the context into normalized regions. Touching
 * regions of the same type are merged into one (the only merging done); a
 * region of zero pages is skipped.
 */
static JPBootResult normalize_memory_map(const JPUefiContext *context, bool final_map, JPMemoryMap *out)
{
	const uint8_t *bytes = (const uint8_t *)context->map_buffer;
	JPUefiUintn count;
	JPUefiUintn i;

	if (context->descriptor_size < sizeof(JPUefiMemoryDescriptor) ||
	    context->map_size % context->descriptor_size != 0) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	count = context->map_size / context->descriptor_size;
	out->region_count = 0;

	for (i = 0; i < count; i++) {
		const JPUefiMemoryDescriptor *descriptor =
			(const JPUefiMemoryDescriptor *)(const void *)(bytes + i * context->descriptor_size);
		JPMemoryRegionType type = normalize_memory_type(descriptor->type, final_map);
		uint64_t base = descriptor->physical_start;
		uint64_t length;

		if (descriptor->number_of_pages == 0) {
			continue;
		}
		if (descriptor->number_of_pages > UINT64_MAX / JP_UEFI_PAGE_SIZE) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		length = descriptor->number_of_pages * JP_UEFI_PAGE_SIZE;
		if (length > UINT64_MAX - base) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}

		if (out->region_count > 0) {
			JPMemoryRegion *last = &out->regions[out->region_count - 1];

			if (last->type == type && last->base_address + last->length == base) {
				if (length > UINT64_MAX - last->length) {
					return JP_BOOT_ERROR_CORRUPTED_DATA;
				}
				last->length += length;
				continue;
			}
		}
		if (out->region_count == JP_MAX_MEMORY_REGIONS) {
			return JP_BOOT_ERROR_CORRUPTED_DATA;
		}
		out->regions[out->region_count].base_address = base;
		out->regions[out->region_count].length = length;
		out->regions[out->region_count].type = type;
		out->region_count++;
	}
	return JP_BOOT_SUCCESS;
}

static void copy_memory_map(JPMemoryMap *destination, const JPMemoryMap *source)
{
	size_t i;

	destination->region_count = source->region_count;
	for (i = 0; i < source->region_count; i++) {
		destination->regions[i] = source->regions[i];
	}
}

/*
 * Gets the raw map into the buffer of the context. The buffer is firmware pool
 * memory; it grows while the firmware says "too small", and keeps spare room
 * because allocating the buffer can add descriptors to the map.
 */
static JPBootResult acquire_firmware_memory_map(JPUefiContext *context)
{
	JPUefiBootServices *services = context->boot_services;
	int attempt;

	for (attempt = 0; attempt < JP_UEFI_MAP_ATTEMPTS; attempt++) {
		JPUefiUintn size = context->map_buffer_size;
		JPUefiStatus status = services->get_memory_map(&size, context->map_buffer,
			&context->map_key, &context->descriptor_size, &context->descriptor_version);

		if (status == JP_UEFI_SUCCESS) {
			context->map_size = size;
			return JP_BOOT_SUCCESS;
		}
		if (status != JP_UEFI_BUFFER_TOO_SMALL || context->descriptor_size == 0) {
			return JP_BOOT_FAILED;
		}
		{
			void *buffer = NULL;
			JPUefiUintn new_size = size + JP_UEFI_MAP_SPARE_DESCRIPTORS * context->descriptor_size;

			if (services->allocate_pool(JP_UEFI_MEMORY_LOADER_DATA, new_size, &buffer) != JP_UEFI_SUCCESS ||
			    buffer == NULL) {
				return JP_BOOT_FAILED;
			}
			if (context->map_buffer != NULL) {
				(void)services->free_pool(context->map_buffer);
			}
			context->map_buffer = (JPUefiMemoryDescriptor *)buffer;
			context->map_buffer_size = new_size;
		}
	}
	return JP_BOOT_FAILED;
}

/* ---- Private helpers: framebuffer, file, input ---- */

static JPFramebufferFormat normalize_pixel_format(uint32_t format)
{
	switch (format) {
	case JP_UEFI_GOP_PIXEL_RGB_RESERVED_8:
		return JP_FRAMEBUFFER_FORMAT_RGB_888;
	case JP_UEFI_GOP_PIXEL_BGR_RESERVED_8:
		return JP_FRAMEBUFFER_FORMAT_BGR_888;
	default:
		/* Bit-mask and blt-only formats cannot be used; the generic code rejects "unknown". */
		return JP_FRAMEBUFFER_FORMAT_UNKNOWN;
	}
}

static void close_file(JPUefiFile *file)
{
	if (file != NULL && file->close != NULL) {
		(void)file->close(file);
	}
}

/* Reads exactly `size` bytes of the open file into the buffer; false if it ends early or fails. */
static bool read_whole_file(JPUefiFile *file, uint8_t *buffer, JPUefiUintn size)
{
	JPUefiUintn total = 0;

	while (total < size) {
		JPUefiUintn piece = size - total;

		if (file->read(file, &piece, buffer + total) != JP_UEFI_SUCCESS || piece == 0) {
			return false;
		}
		total += piece;
	}
	return true;
}

static JPBootResult map_key_event(const JPUefiInputKey *key, JPBootInputEvent *event)
{
	event->kind = JP_BOOT_INPUT_NONE;
	event->key = JP_BOOT_KEY_UP;
	event->character = 0;

	switch (key->scan_code) {
	case JP_UEFI_SCAN_UP: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_UP; return JP_BOOT_SUCCESS;
	case JP_UEFI_SCAN_DOWN: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_DOWN; return JP_BOOT_SUCCESS;
	case JP_UEFI_SCAN_RIGHT: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_RIGHT; return JP_BOOT_SUCCESS;
	case JP_UEFI_SCAN_LEFT: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_LEFT; return JP_BOOT_SUCCESS;
	case JP_UEFI_SCAN_ESCAPE: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_ESCAPE; return JP_BOOT_SUCCESS;
	case JP_UEFI_SCAN_F10: event->kind = JP_BOOT_INPUT_KEY; event->key = JP_BOOT_KEY_F10; return JP_BOOT_SUCCESS;
	case 0: break;
	default: return JP_BOOT_SUCCESS;        /* a special key that cannot be mapped */
	}
	if (key->unicode_char == JP_UEFI_CHAR_ENTER) {
		event->kind = JP_BOOT_INPUT_KEY;
		event->key = JP_BOOT_KEY_ENTER;
	} else if (key->unicode_char == JP_UEFI_CHAR_BACKSPACE) {
		event->kind = JP_BOOT_INPUT_KEY;
		event->key = JP_BOOT_KEY_BACKSPACE;
	} else if (key->unicode_char >= JP_UEFI_CHAR_FIRST_PRINTABLE &&
		   key->unicode_char <= JP_UEFI_CHAR_LAST_PRINTABLE) {
		event->kind = JP_BOOT_INPUT_CHARACTER;
		event->character = key->unicode_char;
	}
	return JP_BOOT_SUCCESS;
}

/* ---- Implementation ---- */

size_t jp_uefi_context_size(void)
{
	return sizeof(JPUefiContext);
}

/* The memory is zeroed by the caller; only the fields that matter are set. */
JPUefiContext *jp_uefi_context_setup(void *memory, void *image_handle, void *system_table)
{
	JPUefiContext *context = (JPUefiContext *)memory;

	if (memory == NULL || image_handle == NULL || system_table == NULL) {
		return NULL;
	}
	context->image_handle = image_handle;
	context->system_table = (JPUefiSystemTable *)system_table;
	context->state = JP_UEFI_STATE_UNINITIALIZED;
	return context;
}

bool jp_uefi_boot_services_active(const JPUefiContext *context)
{
	return context != NULL && context->boot_services != NULL;
}

JPBootResult jp_uefi_initialize(JPUefiContext *context)
{
	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->state != JP_UEFI_STATE_UNINITIALIZED) {
		return JP_BOOT_FAILED;
	}
	if (context->system_table == NULL ||
	    context->system_table->header.signature != JP_UEFI_SYSTEM_TABLE_SIGNATURE ||
	    context->system_table->boot_services == NULL) {
		return fail_provider(context, JP_BOOT_ERROR_CORRUPTED_DATA);
	}
	context->boot_services = context->system_table->boot_services;
	context->state = JP_UEFI_STATE_INITIALIZED;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_uefi_get_memory_map(JPUefiContext *context, JPMemoryMap *memory_map)
{
	JPBootResult result;

	if (context == NULL || memory_map == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->state == JP_UEFI_STATE_EXITED) {
		/* No firmware service may be called now: the final map was kept at the transition. */
		if (!context->final_map_valid) {
			return JP_BOOT_NOT_INITIALIZED;
		}
		copy_memory_map(memory_map, &context->final_map);
		return JP_BOOT_SUCCESS;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	result = acquire_firmware_memory_map(context);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	result = normalize_memory_map(context, false, memory_map);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	context->state = JP_UEFI_STATE_ACTIVE;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_uefi_get_framebuffer(JPUefiContext *context, JPFramebufferInfo *framebuffer)
{
	JPUefiGraphicsOutput *gop = NULL;
	JPUefiStatus status;
	uint64_t pitch;
	static const JPUefiGuid gop_guid = JP_UEFI_GRAPHICS_OUTPUT_GUID;

	if (context == NULL || framebuffer == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	status = context->boot_services->locate_protocol(&gop_guid, NULL, (void **)&gop);
	if (status == JP_UEFI_NOT_FOUND) {
		return JP_BOOT_NOT_DETECTED;
	}
	if (status != JP_UEFI_SUCCESS || gop == NULL || gop->mode == NULL || gop->mode->info == NULL) {
		return JP_BOOT_FAILED;
	}
	pitch = (uint64_t)gop->mode->info->pixels_per_scan_line * JP_UEFI_BYTES_PER_PIXEL;
	if (pitch > UINT32_MAX) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	framebuffer->base_address = gop->mode->frame_buffer_base;
	framebuffer->width = gop->mode->info->horizontal_resolution;
	framebuffer->height = gop->mode->info->vertical_resolution;
	framebuffer->pitch = (uint32_t)pitch;
	framebuffer->format = normalize_pixel_format(gop->mode->info->pixel_format);
	context->state = JP_UEFI_STATE_ACTIVE;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_uefi_get_kernel_image(JPUefiContext *context, JPKernelImage *image)
{
	static const JPUefiGuid loaded_image_guid = JP_UEFI_LOADED_IMAGE_GUID;
	static const JPUefiGuid file_system_guid = JP_UEFI_SIMPLE_FILE_SYSTEM_GUID;
	static const JPUefiGuid file_info_guid = JP_UEFI_FILE_INFO_GUID;
	static const JPUefiChar16 kernel_path[] = u"\\EFI\\JustOS\\kernel.elf";
	JPUefiBootServices *services;
	JPUefiLoadedImage *loaded_image = NULL;
	JPUefiSimpleFileSystem *file_system = NULL;
	JPUefiFile *root = NULL;
	JPUefiFile *file = NULL;
	union {
		JPUefiFileInfo info;
		uint8_t bytes[JP_UEFI_INFO_BUFFER_SIZE];
	} information;
	JPUefiUintn information_size = sizeof information;
	JPUefiUintn file_size;
	void *buffer = NULL;
	JPUefiStatus status;
	JPBootResult result = JP_BOOT_FAILED;

	if (context == NULL || image == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	services = context->boot_services;

	/* The file system of the device from which this boot application was loaded. */
	if (services->handle_protocol(context->image_handle, &loaded_image_guid, (void **)&loaded_image) != JP_UEFI_SUCCESS ||
	    loaded_image == NULL ||
	    services->handle_protocol(loaded_image->device_handle, &file_system_guid, (void **)&file_system) != JP_UEFI_SUCCESS ||
	    file_system == NULL ||
	    file_system->open_volume(file_system, &root) != JP_UEFI_SUCCESS || root == NULL) {
		return JP_BOOT_FAILED;
	}

	status = root->open(root, &file, kernel_path, JP_UEFI_FILE_MODE_READ, 0);
	if (status == JP_UEFI_NOT_FOUND) {
		result = JP_BOOT_NOT_DETECTED;
	} else if (status == JP_UEFI_SUCCESS && file != NULL) {
		if (file->get_info(file, &file_info_guid, &information_size, &information) == JP_UEFI_SUCCESS) {
			file_size = information.info.file_size;
			/* A size of zero still gets a one-byte buffer, so that the pointer is valid. */
			if (services->allocate_pool(JP_UEFI_MEMORY_LOADER_DATA, file_size == 0 ? 1 : file_size, &buffer) == JP_UEFI_SUCCESS &&
			    buffer != NULL) {
				if (read_whole_file(file, (uint8_t *)buffer, file_size)) {
					image->buffer = buffer;
					image->size = file_size;
					context->state = JP_UEFI_STATE_ACTIVE;
					result = JP_BOOT_SUCCESS;
				} else {
					(void)services->free_pool(buffer);
				}
			}
		}
		close_file(file);
	}
	close_file(root);
	return result;
}

JPBootResult jp_uefi_read_input(JPUefiContext *context, JPBootInputEvent *event)
{
	JPUefiInputKey key;
	JPUefiStatus status;

	if (context == NULL || event == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	/* From the moment the final key is held, boot services must not be called. */
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	if (context->system_table->con_in == NULL) {
		return JP_BOOT_NOT_DETECTED;
	}
	key.scan_code = 0;
	key.unicode_char = 0;
	status = context->system_table->con_in->read_key_stroke(context->system_table->con_in, &key);
	if (status == JP_UEFI_NOT_READY) {
		event->kind = JP_BOOT_INPUT_NONE;
		event->key = JP_BOOT_KEY_UP;
		event->character = 0;
		return JP_BOOT_SUCCESS;
	}
	if (status != JP_UEFI_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	return map_key_event(&key, event);
}

JPBootResult jp_uefi_reserve_memory(JPUefiContext *context, uint64_t physical, uint64_t length)
{
	uint64_t start;
	uint64_t end;
	uint64_t address;

	if (context == NULL || length == 0 || length > UINT64_MAX - physical) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	/* Round the range outward to whole pages. */
	end = physical + length;
	if (end > UINT64_MAX - (JP_UEFI_PAGE_SIZE - 1)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	start = align_down_page(physical);
	end = align_down_page(end + JP_UEFI_PAGE_SIZE - 1);
	address = start;
	if (context->boot_services->allocate_pages(JP_UEFI_ALLOCATE_ADDRESS, JP_UEFI_MEMORY_LOADER_DATA,
		(end - start) / JP_UEFI_PAGE_SIZE, &address) != JP_UEFI_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	context->state = JP_UEFI_STATE_ACTIVE;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_uefi_prepare_handoff(JPUefiContext *context)
{
	int attempt;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!state_permits_use(context)) {
		return JP_BOOT_NOT_INITIALIZED;
	}

	for (attempt = 0; attempt < JP_UEFI_EXIT_ATTEMPTS; attempt++) {
		JPBootResult result;
		JPUefiStatus status;

		/* The final map and its key. From here on input is refused. */
		result = acquire_firmware_memory_map(context);
		if (result != JP_BOOT_SUCCESS) {
			return fail_provider(context, result);
		}
		context->state = JP_UEFI_STATE_FINAL_MAP_ACQUIRED;

		/* A final map: boot services memory is given back at the exit, so it is usable. */
		result = normalize_memory_map(context, true, &context->final_map);
		if (result != JP_BOOT_SUCCESS) {
			return fail_provider(context, result);
		}
		context->state = JP_UEFI_STATE_EXIT_BOOT_SERVICES_READY;

		status = context->boot_services->exit_boot_services(context->image_handle, context->map_key);
		if (status == JP_UEFI_SUCCESS) {
			context->final_map_valid = true;
			context->boot_services = NULL;
			context->system_table = NULL;
			context->state = JP_UEFI_STATE_EXITED;
			return JP_BOOT_SUCCESS;
		}
		if (status != JP_UEFI_INVALID_PARAMETER) {
			return fail_provider(context, JP_BOOT_FAILED);
		}
		/* The key was stale (the map changed): take a fresh map and key and try once more. */
	}
	return fail_provider(context, JP_BOOT_FAILED);
}
