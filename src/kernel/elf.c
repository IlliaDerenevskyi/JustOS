/*
 * elf.c - Stage 2 check of the loaded kernel image.
 *
 * Purpose:
 *   Checks that the kernel image state that Stage 1 recorded is consistent.
 *   The exact checks are in "Decided rules (the exact checks of
 *   jp_kernel_elf_initialize())" of milestone0.md.
 *
 * Responsibilities:
 *   - the ELF header matches the architecture and what Stage 1 supports;
 *   - every segment is permissioned, has a valid range, and its pages are
 *     owned by the kernel image;
 *   - the entry point is the same everywhere and lies in an executable
 *     segment;
 *   - the mappings are checked by asking paging.
 *
 * Limitations:
 *   Only reads. Does not load, copy, relocate, map, or change the
 *   permissions of anything.
 *
 * Depends on:
 *   justos/kernel/elf.h, memory.h, paging.h, arch_dispatch.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/elf.h"

#include "arch_dispatch.h"
#include "justos/kernel/memory.h"
#include "justos/kernel/paging.h"

#define JP_ELF_PROGRAM_HEADER_SIZE 56U
#define JP_ELF_PAGE_MASK ((uint64_t)JP_BOOT_PAGE_SIZE - 1)
#define JP_ELF_X86_64_CANONICAL_SHIFT 47U       /* bits 63..47 must be equal */
#define JP_ELF_AARCH64_CANONICAL_SHIFT 48U      /* bits 63..48 must be equal */

/* ---- Private helpers ---- */

/* True when the bits of `address` from `shift` up are all zero or all one. */
static bool upper_bits_are_equal(uint64_t address, unsigned int shift)
{
	uint64_t upper = address >> shift;
	uint64_t all_ones = UINT64_MAX >> shift;

	return upper == 0 || upper == all_ones;
}

static bool address_is_canonical(JPArchitectureType architecture, uint64_t address)
{
	return upper_bits_are_equal(address, architecture == JP_ARCH_X86_64 ?
		JP_ELF_X86_64_CANONICAL_SHIFT : JP_ELF_AARCH64_CANONICAL_SHIFT);
}

static uint16_t machine_of(JPArchitectureType architecture)
{
	return architecture == JP_ARCH_X86_64 ? JP_ELF_MACHINE_X86_64 : JP_ELF_MACHINE_AARCH64;
}

static bool header_is_valid(const JPKernelInfo *kernel, JPArchitectureType architecture)
{
	const JPKernelHeader *header = &kernel->header;

	return header->magic == JP_ELF_MAGIC &&
	       header->elf_class == JP_ELF_CLASS_64 &&
	       header->type == JP_ELF_TYPE_EXEC &&
	       header->machine == machine_of(architecture) &&
	       header->program_header_entry_size == JP_ELF_PROGRAM_HEADER_SIZE &&
	       header->program_header_count >= kernel->segment_count;
}

/* The physical pages of the segment are owned, as one range, by the kernel image. */
static bool segment_pages_are_image(const JPBootContext *context, const JPKernelSegment *segment)
{
	uint64_t base = segment->physical_address & ~JP_ELF_PAGE_MASK;
	uint64_t end = segment->physical_address + segment->memory_size;
	JPKernelMemoryOwner owner;

	if (end > UINT64_MAX - JP_ELF_PAGE_MASK) {
		return false;
	}
	end = (end + JP_ELF_PAGE_MASK) & ~JP_ELF_PAGE_MASK;
	return jp_kernel_memory_query(context, base, end - base, &owner) == JP_BOOT_SUCCESS &&
	       owner == JP_KERNEL_MEMORY_KERNEL_IMAGE;
}

static bool segment_is_valid(const JPBootContext *context, const JPKernelSegment *segment,
	JPArchitectureType architecture)
{
	if (segment->state != JP_KERNEL_SEGMENT_PERMISSIONED || segment->memory_size == 0 ||
	    segment->file_size > segment->memory_size) {
		return false;
	}
	if (segment->memory_size > UINT64_MAX - segment->virtual_address ||
	    segment->memory_size > UINT64_MAX - segment->physical_address) {
		return false;
	}
	if (!address_is_canonical(architecture, segment->virtual_address) ||
	    !address_is_canonical(architecture, segment->virtual_address + segment->memory_size - 1)) {
		return false;
	}
	return segment_pages_are_image(context, segment);
}

/* The virtual ranges of two segments overlap. */
static bool segments_overlap(const JPKernelSegment *a, const JPKernelSegment *b)
{
	return a->virtual_address < b->virtual_address + b->memory_size &&
	       b->virtual_address < a->virtual_address + a->memory_size;
}

static bool segments_are_valid(const JPBootContext *context, JPArchitectureType architecture)
{
	size_t i;
	size_t j;

	for (i = 0; i < context->kernel.segment_count; i++) {
		if (!segment_is_valid(context, &context->kernel.segments[i], architecture)) {
			return false;
		}
		for (j = 0; j < i; j++) {
			if (segments_overlap(&context->kernel.segments[i], &context->kernel.segments[j])) {
				return false;
			}
		}
	}
	return true;
}

static bool entry_is_valid(const JPBootContext *context, JPArchitectureType architecture)
{
	uint64_t entry = context->kernel.entry_point;
	size_t i;

	if (entry != context->kernel.header.entry_point || entry != context->handoff.kernel_entry ||
	    !address_is_canonical(architecture, entry)) {
		return false;
	}
	for (i = 0; i < context->kernel.segment_count; i++) {
		const JPKernelSegment *segment = &context->kernel.segments[i];

		if ((segment->flags & JP_ELF_PF_X) != 0 && entry >= segment->virtual_address &&
		    entry - segment->virtual_address < segment->memory_size) {
			return true;
		}
	}
	return false;
}

/* ---- Public API ---- */

JPBootResult jp_kernel_elf_initialize(JPBootContext *context)
{
	JPArchitectureType architecture;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	if (context->stage2.entry_count == 0) {
		return JP_BOOT_NOT_INITIALIZED;
	}
	architecture = context->handoff.architecture;
	if (!JP_KERNEL_ARCH_IS_SUPPORTED(architecture) ||
	    context->kernel.validation_state != JP_KERNEL_VALIDATION_VALID ||
	    context->kernel.load_state != JP_KERNEL_LOAD_LOADED) {
		return JP_BOOT_FAILED;
	}
	if (!header_is_valid(&context->kernel, architecture) || context->kernel.segment_count == 0 ||
	    context->kernel.segment_count > JP_MAX_KERNEL_SEGMENTS) {
		return JP_BOOT_FAILED;
	}
	if (!segments_are_valid(context, architecture) || !entry_is_valid(context, architecture)) {
		return JP_BOOT_FAILED;
	}
	if (jp_kernel_paging_validate(context) != JP_BOOT_SUCCESS) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}
