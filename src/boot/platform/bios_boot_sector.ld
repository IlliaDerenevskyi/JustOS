/*
 * Link layout of the BIOS boot sector: one flat binary of exactly 512 bytes
 * that the BIOS places at 0x7C00; the last two bytes are the boot signature.
 */
ENTRY(jp_bios_boot_sector_start)

SECTIONS
{
	. = 0x7C00;
	.boot : {
		*(.text.boot_sector)
		ASSERT(. <= 0x7DFE, "the boot sector code is too big");
		. = 0x7DFE;
		SHORT(0xAA55)
	}
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) *(.data*) *(.bss*) }
}
