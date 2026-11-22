/*
 * elf_builder.h - builds small ELF64 images in memory for host tests.
 *
 * Test support only: never linked into production code. The produced bytes
 * follow the ELF64 little-endian layout (64-byte file header, 56-byte program
 * headers). The builder deliberately lets a test write wrong values (bad
 * magic, bad machine, bad offsets) so that rejection paths can be tested.
 */

#ifndef JUSTOS_TESTS_SUPPORT_ELF_BUILDER_H
#define JUSTOS_TESTS_SUPPORT_ELF_BUILDER_H

#include <stddef.h>
#include <stdint.h>

#define JP_TEST_ELF_HEADER_SIZE 64
#define JP_TEST_ELF_PHDR_SIZE 56
#define JP_TEST_ELF_MAX_SEGMENTS 32

typedef struct JPTestElfSegment {
	uint32_t type;           /* program header type, 1 = PT_LOAD */
	uint32_t flags;
	uint64_t virtual_address;
	uint64_t physical_address;
	uint64_t file_size;      /* bytes copied from the file */
	uint64_t memory_size;
	uint64_t file_offset;    /* 0 = let the builder place the data */
} JPTestElfSegment;

typedef struct JPTestElfSpec {
	uint32_t magic;          /* normally 0x464C457F */
	uint8_t elf_class;       /* normally 2 */
	uint16_t type;           /* 2 EXEC, 3 DYN */
	uint16_t machine;        /* 62 or 183 */
	uint64_t entry_point;
	uint16_t phdr_entry_size;  /* normally JP_TEST_ELF_PHDR_SIZE */
	size_t segment_count;
	JPTestElfSegment segments[JP_TEST_ELF_MAX_SEGMENTS];
} JPTestElfSpec;

/* Fills spec with a valid one-segment x86_64 executable. */
void jp_test_elf_default_spec(JPTestElfSpec *spec);

/*
 * Writes the image into buffer. Segment data bytes are filled with the low
 * byte of the segment index plus one. Returns the image size (the end of the
 * last byte written: headers and data), or 0 if the buffer is too small. A
 * segment whose explicit file_offset does not fit into the buffer is described
 * in its program header, but its data is not written and does not count for
 * the size.
 */
size_t jp_test_elf_build(
	const JPTestElfSpec *spec,
	uint8_t *buffer,
	size_t buffer_size
);

#endif /* JUSTOS_TESTS_SUPPORT_ELF_BUILDER_H */
