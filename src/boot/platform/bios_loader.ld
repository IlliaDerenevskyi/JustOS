/*
 * Link layout of the BIOS loader: one flat binary that the boot sector reads
 * to 0x8000. The 16-bit part (bios_loader.S) is padded to exactly 4 KiB; the
 * 64-bit part follows at 0x9000 and starts with jp_bios_entry_64. All the 64-bit
 * code runs in place, so it must have no writable static data (the project
 * forbids global mutable state): the asserts below enforce it.
 */
ENTRY(jp_bios_loader_start)

SECTIONS
{
	. = 0x8000;
	.loader16 : {
		*(.text.loader16)
		ASSERT(. <= 0x9000, "the 16-bit loader part is larger than 4 KiB");
		. = 0x9000;
	}
	.code64 : {
		*(.text.entry64)
		*(.text .text.*)
	}
	.rodata : { *(.rodata .rodata.*) }
	.data : { *(.data .data.*) }
	.bss : { *(.bss .bss.*) *(COMMON) }
	ASSERT(SIZEOF(.data) == 0, "the 64-bit loader code has writable static data")
	ASSERT(SIZEOF(.bss) == 0, "the 64-bit loader code has zero-initialized static data")
	ASSERT(. <= 0x10000, "the loader is larger than 56 KiB")
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
