/*
 * kernel_loader.c - generic ELF64 kernel validation and loading.
 *
 * Purpose:
 *   Validates the kernel image (jp_boot_validate_kernel) and loads it into
 *   physical memory and maps it (jp_boot_load_kernel).
 *
 * Responsibilities:
 *   - ELF header, program-header table, PT_LOAD segments, entry point;
 *   - placing every segment in usable physical memory and reserving it through
 *     the boot environment, copying the file bytes, zero-filling the rest, and
 *     mapping it through the architecture module.
 *
 * Limitations:
 *   The caller sets handoff.architecture. Only ET_EXEC kernels are accepted.
 *   Nothing from an invalid image is stored in the context. The rules are in
 *   milestone0.md ("Decided rules (ELF validation)" and "Decided rules
 *   (physical memory, copying, mapping)").
 *
 * Depends on:
 *   justos/boot/kernel_loader.h, justos/boot/boot_environment.h, and the
 *   architecture headers.
 *
 * Must not depend on:
 *   Provider headers, JPBootInfo.
 */

#include "justos/boot/kernel_loader.h"

#include "justos/boot/architecture/aarch64.h"
#include "justos/boot/architecture/x86_64.h"
#include "justos/boot/boot_environment.h"
#include "arch_dispatch.h"

/* ---- Public API ---- */

JPBootResult jp_boot_validate_kernel(JPBootContext *context);
JPBootResult jp_boot_load_kernel(JPBootContext *context);

/* ---- Private constants ---- */

enum {
	JP_ELF_HEADER_SIZE = 64,
	JP_ELF_PHDR_SIZE = 56,

	/* ELF file header field offsets. */
	JP_EH_CLASS = 4,
	JP_EH_TYPE = 16,
	JP_EH_MACHINE = 18,
	JP_EH_ENTRY = 24,
	JP_EH_PHOFF = 32,
	JP_EH_PHENTSIZE = 54,
	JP_EH_PHNUM = 56,

	/* ELF program header field offsets. */
	JP_PH_TYPE = 0,
	JP_PH_FLAGS = 4,
	JP_PH_OFFSET = 8,
	JP_PH_VADDR = 16,
	JP_PH_FILESZ = 32,
	JP_PH_MEMSZ = 40,

	JP_PF_X = 1,
	JP_PF_W = 2,
	JP_PF_R = 4,
	JP_PF_ALL = JP_PF_X | JP_PF_W | JP_PF_R,

	JP_BYTE_BITS = 8
};

/* ---- Private helpers: little-endian reads ---- */

static uint16_t read_u16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << JP_BYTE_BITS));
}

static uint32_t read_u32(const uint8_t *p)
{
	return (uint32_t)read_u16(p) | ((uint32_t)read_u16(p + 2) << (2 * JP_BYTE_BITS));
}

static uint64_t read_u64(const uint8_t *p)
{
	return (uint64_t)read_u32(p) | ((uint64_t)read_u32(p + 4) << (4 * JP_BYTE_BITS));
}

/* ---- Private helpers: architecture dispatch ---- */

static JPBootResult arch_validate_elf_machine(JPArchitectureType arch, const JPKernelHeader *header)
{
	(void)arch;    /* unused in a build for one architecture */
	return JP_ARCH_DISPATCH(arch, jp_x86_64_validate_elf_machine(header),
		jp_aarch64_validate_elf_machine(header));
}

static void *arch_phys_access(JPBootContext *context, uint64_t physical, uint64_t length)
{
	return JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_phys_access(context, physical, length),
		jp_aarch64_phys_access(context, physical, length));
}

static JPBootResult arch_map_segment(JPBootContext *context, size_t index)
{
	return JP_ARCH_DISPATCH(context->handoff.architecture,
		jp_x86_64_map_segment(context, index), jp_aarch64_map_segment(context, index));
}

static bool architecture_is_known(JPArchitectureType arch)
{
	return JP_ARCH_IS_SUPPORTED(arch);
}

/* ---- Private helpers: validation ---- */

static void fail_validation(JPBootContext *context)
{
	context->kernel.validation_state = JP_KERNEL_VALIDATION_INVALID;
	context->kernel.segment_count = 0;
}

/* Does [offset, offset + length) lie inside an image of size bytes? */
static bool range_inside(uint64_t offset, uint64_t length, uint64_t size)
{
	return offset <= size && length <= size - offset;
}

static bool validate_segment(const JPKernelSegment *segment, uint64_t image_size)
{
	if (segment->file_size > segment->memory_size) {
		return false;
	}
	if (!range_inside(segment->file_offset, segment->file_size, image_size)) {
		return false;
	}
	if (segment->memory_size > UINT64_MAX - segment->virtual_address) {
		return false;
	}
	if (segment->virtual_address % JP_BOOT_PAGE_SIZE != 0) {
		return false;
	}
	if (segment->file_offset % JP_BOOT_PAGE_SIZE != segment->virtual_address % JP_BOOT_PAGE_SIZE) {
		return false;
	}
	if ((segment->flags & JP_PF_ALL) == 0 || (segment->flags & ~(uint32_t)JP_PF_ALL) != 0) {
		return false;
	}
	/* Write and execute together are refused (W xor X). */
	return (segment->flags & (JP_PF_W | JP_PF_X)) != (JP_PF_W | JP_PF_X);
}

static bool entry_is_in_executable_segment(const JPKernelSegment *segments, size_t count, uint64_t entry)
{
	size_t i;

	for (i = 0; i < count; i++) {
		if ((segments[i].flags & JP_PF_X) != 0 &&
		    entry >= segments[i].virtual_address &&
		    entry - segments[i].virtual_address < segments[i].memory_size) {
			return true;
		}
	}
	return false;
}

/* Parses and checks the header and the program headers into local variables. */
static bool parse_and_validate(JPBootContext *context, JPKernelHeader *header,
	JPKernelSegment *segments, size_t *segment_count, JPBootResult *result)
{
	const uint8_t *image = (const uint8_t *)context->kernel.image.buffer;
	uint64_t size = context->kernel.image.size;
	uint64_t table_bytes;
	size_t found = 0;
	size_t i;

	*result = JP_BOOT_ERROR_CORRUPTED_DATA;
	if (size < JP_ELF_HEADER_SIZE) {
		return false;
	}
	header->magic = read_u32(image);
	header->elf_class = image[JP_EH_CLASS];
	header->type = read_u16(image + JP_EH_TYPE);
	header->machine = read_u16(image + JP_EH_MACHINE);
	header->entry_point = read_u64(image + JP_EH_ENTRY);
	header->program_header_offset = read_u64(image + JP_EH_PHOFF);
	header->program_header_entry_size = read_u16(image + JP_EH_PHENTSIZE);
	header->program_header_count = read_u16(image + JP_EH_PHNUM);

	if (header->magic != JP_ELF_MAGIC || header->elf_class != JP_ELF_CLASS_64) {
		return false;
	}
	*result = arch_validate_elf_machine(context->handoff.architecture, header);
	if (*result != JP_BOOT_SUCCESS) {
		return false;
	}
	*result = JP_BOOT_ERROR_CORRUPTED_DATA;
	if (header->type != JP_ELF_TYPE_EXEC) {
		return false;
	}
	if (header->program_header_entry_size != JP_ELF_PHDR_SIZE) {
		return false;
	}
	table_bytes = (uint64_t)header->program_header_count * JP_ELF_PHDR_SIZE;
	if (!range_inside(header->program_header_offset, table_bytes, size)) {
		return false;
	}

	for (i = 0; i < header->program_header_count; i++) {
		const uint8_t *ph = image + header->program_header_offset + i * JP_ELF_PHDR_SIZE;
		JPKernelSegment *segment;

		if (read_u32(ph + JP_PH_TYPE) != JP_ELF_PT_LOAD) {
			continue;
		}
		if (found == JP_MAX_KERNEL_SEGMENTS) {
			return false;
		}
		segment = &segments[found];
		segment->flags = read_u32(ph + JP_PH_FLAGS);
		segment->file_offset = read_u64(ph + JP_PH_OFFSET);
		segment->virtual_address = read_u64(ph + JP_PH_VADDR);
		segment->physical_address = 0;
		segment->state = JP_KERNEL_SEGMENT_VALIDATED;
		segment->file_size = read_u64(ph + JP_PH_FILESZ);
		segment->memory_size = read_u64(ph + JP_PH_MEMSZ);
		if (!validate_segment(segment, size)) {
			return false;
		}
		found++;
	}
	if (found == 0) {
		return false;
	}
	if (!entry_is_in_executable_segment(segments, found, header->entry_point)) {
		return false;
	}
	*segment_count = found;
	*result = JP_BOOT_SUCCESS;
	return true;
}

/* ---- Private helpers: loading ---- */

static uint64_t align_up_page(uint64_t value)
{
	return (value + JP_BOOT_PAGE_SIZE - 1) / JP_BOOT_PAGE_SIZE * JP_BOOT_PAGE_SIZE;
}

/*
 * Finds the physical address for a segment of length bytes. Regions are taken
 * in map order. In a region the start is the largest of: the region start, the
 * protected low memory limit, and the end of the segment placed just before in
 * the same region; the start is rounded up to a page, and moved behind the
 * kernel image if the range would touch it. Returns false if nothing fits.
 */
static bool place_segment(const JPBootContext *context, uint64_t length,
	size_t *last_region, uint64_t *last_end, uint64_t *address)
{
	uint64_t image_start = (uint64_t)(uintptr_t)context->kernel.image.buffer;
	uint64_t image_end = image_start + context->kernel.image.size;
	size_t i;

	for (i = 0; i < context->memory_map.region_count; i++) {
		const JPMemoryRegion *region = &context->memory_map.regions[i];
		uint64_t region_end;
		uint64_t start;

		if (region->type != JP_MEMORY_REGION_USABLE ||
		    region->length > UINT64_MAX - region->base_address) {
			continue;
		}
		region_end = region->base_address + region->length;
		start = region->base_address;
		if (start < JP_BOOT_PROTECTED_LOW_MEMORY) {
			start = JP_BOOT_PROTECTED_LOW_MEMORY;
		}
		if (*last_region == i && start < *last_end) {
			start = *last_end;
		}
		start = align_up_page(start);
		if (start < image_end && image_start < start + length) {
			start = align_up_page(image_end);
		}
		if (start > region_end || length > region_end - start) {
			continue;
		}
		*address = start;
		*last_region = i;
		*last_end = start + length;
		return true;
	}
	return false;
}

static void copy_and_zero_fill(uint8_t *destination, const uint8_t *source,
	uint64_t file_size, uint64_t memory_size)
{
	uint64_t i;

	for (i = 0; i < file_size; i++) {
		destination[i] = source[i];
	}
	for (; i < memory_size; i++) {
		destination[i] = 0;
	}
}

/* Handles one segment through ALLOCATED, LOADED, and PERMISSIONED. */
static JPBootResult load_one_segment(JPBootContext *context, size_t index,
	size_t *last_region, uint64_t *last_end)
{
	JPKernelSegment *segment = &context->kernel.segments[index];
	JPBootResult result;
	uint64_t address;
	uint8_t *destination;

	if (!place_segment(context, segment->memory_size, last_region, last_end, &address)) {
		return JP_BOOT_FAILED;
	}
	segment->physical_address = address;
	/* Tell the firmware the range is taken before anything is written into it. */
	if (segment->memory_size != 0 &&
	    jp_boot_environment_reserve_memory(context, address, segment->memory_size) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	segment->state = JP_KERNEL_SEGMENT_ALLOCATED;

	if (!range_inside(segment->file_offset, segment->file_size, context->kernel.image.size)) {
		return JP_BOOT_ERROR_CORRUPTED_DATA;
	}
	destination = (uint8_t *)arch_phys_access(context, address, segment->memory_size);
	if (destination == NULL) {
		return JP_BOOT_FAILED;
	}
	copy_and_zero_fill(destination,
		(const uint8_t *)context->kernel.image.buffer + segment->file_offset,
		segment->file_size, segment->memory_size);
	segment->state = JP_KERNEL_SEGMENT_LOADED;

	/* The architecture module creates the mapping together with its access rights. */
	result = arch_map_segment(context, index);
	if (result != JP_BOOT_SUCCESS) {
		return result;
	}
	segment->state = JP_KERNEL_SEGMENT_PERMISSIONED;
	return JP_BOOT_SUCCESS;
}

static bool all_segments_are_validated(const JPKernelInfo *kernel)
{
	size_t i;

	for (i = 0; i < kernel->segment_count; i++) {
		if (kernel->segments[i].state != JP_KERNEL_SEGMENT_VALIDATED) {
			return false;
		}
	}
	return true;
}

/* ---- Implementation ---- */

JPBootResult jp_boot_validate_kernel(JPBootContext *context)
{
	JPKernelHeader header;
	JPKernelSegment segments[JP_MAX_KERNEL_SEGMENTS];
	size_t segment_count = 0;
	JPBootResult result;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	context->current_stage = JP_BOOT_STAGE_KERNEL_VALIDATION;
	if (!architecture_is_known(context->handoff.architecture)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->kernel.image.state != JP_DETECTION_STATE_DETECTED) {
		return JP_BOOT_NOT_INITIALIZED;
	}

	if (!parse_and_validate(context, &header, segments, &segment_count, &result)) {
		fail_validation(context);
		return result;
	}

	/* Everything is valid: only now is anything stored in the context. */
	context->kernel.header = header;
	{
		size_t i;

		for (i = 0; i < segment_count; i++) {
			context->kernel.segments[i] = segments[i];
		}
	}
	context->kernel.segment_count = segment_count;
	context->kernel.entry_point = header.entry_point;
	context->kernel.validation_state = JP_KERNEL_VALIDATION_VALID;
	return JP_BOOT_SUCCESS;
}

JPBootResult jp_boot_load_kernel(JPBootContext *context)
{
	size_t last_region = (size_t)-1;
	uint64_t last_end = 0;
	size_t i;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (!architecture_is_known(context->handoff.architecture)) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->kernel.validation_state != JP_KERNEL_VALIDATION_VALID ||
	    context->memory_map.state != JP_DETECTION_STATE_DETECTED ||
	    !all_segments_are_validated(&context->kernel)) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	context->current_stage = JP_BOOT_STAGE_KERNEL_LOADING;

	for (i = 0; i < context->kernel.segment_count; i++) {
		JPBootResult result = load_one_segment(context, i, &last_region, &last_end);

		if (result != JP_BOOT_SUCCESS) {
			context->kernel.segments[i].state = JP_KERNEL_SEGMENT_FAILED;
			context->kernel.load_state = JP_KERNEL_LOAD_FAILED;
			return result;
		}
	}
	context->kernel.load_state = JP_KERNEL_LOAD_LOADED;
	return JP_BOOT_SUCCESS;
}
