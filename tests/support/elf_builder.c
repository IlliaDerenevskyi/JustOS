#include "elf_builder.h"

#include <string.h>

enum {
	JP_TEST_ELF_DATA_ALIGN = 16,
	JP_TEST_ELF_VERSION_CURRENT = 1,
	JP_TEST_ELF_DATA_LSB = 1,
	JP_TEST_ELF_DEFAULT_CLASS = 2,
	JP_TEST_ELF_DEFAULT_TYPE = 2,
	JP_TEST_ELF_DEFAULT_MACHINE = 62,
	JP_TEST_ELF_PT_LOAD = 1,
	JP_TEST_ELF_FLAG_RX = 5,
	JP_TEST_ELF_DEFAULT_FILE_SIZE = 16,
	JP_TEST_ELF_DEFAULT_MEMORY_SIZE = 4096,
	JP_TEST_ELF_DEFAULT_FILE_OFFSET = 4096
};

#define JP_TEST_ELF_DEFAULT_MAGIC 0x464C457FU
#define JP_TEST_ELF_DEFAULT_ENTRY 0xFFFFFFFF80000000ULL

static void put16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v & 0xFFU);
	p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
	put16(p, (uint16_t)(v & 0xFFFFU));
	put16(p + 2, (uint16_t)(v >> 16));
}

static void put64(uint8_t *p, uint64_t v)
{
	put32(p, (uint32_t)(v & 0xFFFFFFFFU));
	put32(p + 4, (uint32_t)(v >> 32));
}

static uint64_t align_up(uint64_t value, uint64_t alignment)
{
	return (value + alignment - 1U) / alignment * alignment;
}

void jp_test_elf_default_spec(JPTestElfSpec *spec)
{
	memset(spec, 0, sizeof *spec);
	spec->magic = JP_TEST_ELF_DEFAULT_MAGIC;
	spec->elf_class = JP_TEST_ELF_DEFAULT_CLASS;
	spec->type = JP_TEST_ELF_DEFAULT_TYPE;
	spec->machine = JP_TEST_ELF_DEFAULT_MACHINE;
	spec->entry_point = JP_TEST_ELF_DEFAULT_ENTRY;
	spec->phdr_entry_size = JP_TEST_ELF_PHDR_SIZE;
	spec->segment_count = 1;
	spec->segments[0].type = JP_TEST_ELF_PT_LOAD;
	spec->segments[0].flags = JP_TEST_ELF_FLAG_RX;
	spec->segments[0].virtual_address = JP_TEST_ELF_DEFAULT_ENTRY;
	spec->segments[0].physical_address = 0x100000ULL;
	spec->segments[0].file_size = JP_TEST_ELF_DEFAULT_FILE_SIZE;
	spec->segments[0].memory_size = JP_TEST_ELF_DEFAULT_MEMORY_SIZE;
	spec->segments[0].file_offset = JP_TEST_ELF_DEFAULT_FILE_OFFSET;
}

size_t jp_test_elf_build(
	const JPTestElfSpec *spec,
	uint8_t *buffer,
	size_t buffer_size
)
{
	uint64_t phdr_end;
	uint64_t cursor;
	uint64_t end;
	uint8_t *header;
	size_t i;

	if (spec->segment_count > JP_TEST_ELF_MAX_SEGMENTS) {
		return 0;
	}
	phdr_end = JP_TEST_ELF_HEADER_SIZE +
		(uint64_t)spec->segment_count * JP_TEST_ELF_PHDR_SIZE;
	cursor = align_up(phdr_end, JP_TEST_ELF_DATA_ALIGN);
	if (buffer_size < cursor) {
		return 0;
	}
	memset(buffer, 0, buffer_size);
	end = cursor;

	header = buffer;
	put32(header, spec->magic);
	header[4] = spec->elf_class;
	header[5] = JP_TEST_ELF_DATA_LSB;
	header[6] = JP_TEST_ELF_VERSION_CURRENT;
	put16(header + 16, spec->type);
	put16(header + 18, spec->machine);
	put32(header + 20, JP_TEST_ELF_VERSION_CURRENT);
	put64(header + 24, spec->entry_point);
	put64(header + 32, JP_TEST_ELF_HEADER_SIZE);
	put16(header + 52, JP_TEST_ELF_HEADER_SIZE);
	put16(header + 54, spec->phdr_entry_size);
	put16(header + 56, (uint16_t)spec->segment_count);

	for (i = 0; i < spec->segment_count; i++) {
		const JPTestElfSegment *seg = &spec->segments[i];
		uint8_t *ph = buffer + JP_TEST_ELF_HEADER_SIZE +
			i * JP_TEST_ELF_PHDR_SIZE;
		uint64_t offset = seg->file_offset;

		if (offset == 0) {
			offset = cursor;
			cursor = align_up(cursor + seg->file_size,
				JP_TEST_ELF_DATA_ALIGN);
			if (cursor > buffer_size) {
				return 0;
			}
			memset(buffer + offset, (int)((i + 1U) & 0xFFU),
				(size_t)seg->file_size);
		}
		if (seg->file_offset != 0 &&
		    seg->file_offset <= buffer_size &&
		    seg->file_size <= buffer_size - seg->file_offset) {
			memset(buffer + seg->file_offset, (int)((i + 1U) & 0xFFU),
				(size_t)seg->file_size);
			/* The image reaches at least to the end of data that was written. */
			if (seg->file_offset + seg->file_size > end) {
				end = seg->file_offset + seg->file_size;
			}
		}
		put32(ph, seg->type);
		put32(ph + 4, seg->flags);
		put64(ph + 8, offset);
		put64(ph + 16, seg->virtual_address);
		put64(ph + 24, seg->physical_address);
		put64(ph + 32, seg->file_size);
		put64(ph + 40, seg->memory_size);
	}
	return (size_t)(cursor > end ? cursor : end);
}
