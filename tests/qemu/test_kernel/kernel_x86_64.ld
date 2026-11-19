/* Link layout of the test kernel for x86_64: higher half, 4 KiB aligned segments. */
ENTRY(kernel_main)

PHDRS
{
	text PT_LOAD FLAGS(5);   /* read + execute */
	data PT_LOAD FLAGS(6);   /* read + write */
}

SECTIONS
{
	. = 0xFFFFFFFF80000000;
	.text : ALIGN(4096) { *(.text .text.*) } :text
	.rodata : ALIGN(4096) { *(.rodata .rodata.*) } :text
	.data : ALIGN(4096) { *(.data .data.*) } :data
	.bss : ALIGN(4096) { *(.bss .bss.*) *(COMMON) } :data
	/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
