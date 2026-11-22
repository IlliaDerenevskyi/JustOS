/*
 * bios_boot_sector.S - the BIOS boot sector (16-bit real-mode code, x86_64 only).
 *
 * The BIOS reads the first sector of the disk to 0x7C00 and starts it with the
 * drive number in DL. This sector (exactly 512 bytes, ending with 0x55 0xAA):
 *
 *   1. saves the drive number;
 *   2. reads sector 1 (the image header) to 0x7E00 with the extended disk read
 *      (INT 13h, AH = 42h; the "disk address packet" says which sector, how
 *      many, and where to);
 *   3. checks the text "JUSTBIOS" and the version 1, and that the loader is
 *      1 to 112 sectors (at most 56 KiB: it must end before the block at 0x10000);
 *   4. reads the loader to 0x8000 (segment 0x0800, offset 0, so a 16-bit offset
 *      never overflows) and jumps there with the drive number in DL.
 *
 * Any failure prints one character "!" and halts. The image layout is in
 * "Decided rules (the BIOS disk image layout)" of milestone0.md.
 *
 * Every address is a fixed number or is resolved by the linker (the sector is
 * linked at 0x7C00 with bios_boot_sector.ld).
 */

	.intel_syntax noprefix

	.set HEADER_ADDRESS, 0x7E00         /* where sector 1 is read */
	.set LOADER_SEGMENT, 0x0800         /* segment:offset 0800:0000 is the address 0x8000 */
	.set LOADER_ADDRESS, 0x8000
	.set LOADER_MAX_SECTORS, 112        /* 56 KiB */
	.set STACK_TOP, 0x7C00
	.set HEADER_LOADER_LBA, 12          /* offsets in the image header */
	.set HEADER_LOADER_COUNT, 16
	.set HEADER_VERSION, 8

	.section .text.boot_sector, "ax"
	.code16
	.globl jp_bios_boot_sector_start
jp_bios_boot_sector_start:
	cli
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, STACK_TOP
	cld
	sti
	mov byte ptr [drive], dl

	/* Read sector 1 to 0x7E00 (the packet already says: 1 sector, 0000:7E00, LBA 1). */
	mov si, offset packet
	mov ah, 0x42
	mov dl, byte ptr [drive]
	int 0x13
	jc fail

	/* The text "JUSTBIOS" and the version 1. */
	cmp dword ptr [HEADER_ADDRESS], 0x5453554A       /* "JUST" */
	jne fail
	cmp dword ptr [HEADER_ADDRESS + 4], 0x534F4942   /* "BIOS" */
	jne fail
	cmp dword ptr [HEADER_ADDRESS + HEADER_VERSION], 1
	jne fail

	/* The loader: first sector and count (1 to 112 sectors). */
	mov eax, dword ptr [HEADER_ADDRESS + HEADER_LOADER_COUNT]
	test eax, eax
	jz fail
	cmp eax, LOADER_MAX_SECTORS
	ja fail
	mov word ptr [packet + 2], ax                    /* sector count */
	mov word ptr [packet + 4], 0                     /* offset */
	mov word ptr [packet + 6], LOADER_SEGMENT        /* segment */
	mov eax, dword ptr [HEADER_ADDRESS + HEADER_LOADER_LBA]
	mov dword ptr [packet + 8], eax                  /* LBA, low half (the high half is zero already) */
	mov si, offset packet
	mov ah, 0x42
	mov dl, byte ptr [drive]
	int 0x13
	jc fail

	/* Start the loader: far jump to 0000:8000 with the drive in DL. */
	mov dl, byte ptr [drive]
	.byte 0xEA
	.word LOADER_ADDRESS
	.word 0

fail:
	mov ax, 0x0E21                                    /* teletype output of "!" */
	xor bx, bx
	int 0x10
halt:
	cli
	hlt
	jmp halt

drive:
	.byte 0

/* The disk address packet: size 16, reserved 0, sector count, offset, segment, 64-bit LBA. */
	.balign 4
packet:
	.byte 0x10, 0
	.word 1
	.word HEADER_ADDRESS, 0
	.quad 1
