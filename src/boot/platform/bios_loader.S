/*
 * bios_loader.S - the BIOS loader: 16-bit real-mode code that ends in 64-bit mode.
 *
 * The boot sector reads this file to 0x8000 and jumps to its first byte with
 * the drive number in DL. BIOS services work only in real mode, so this code
 * does all the BIOS calls first and leaves the results in the boot data block
 * (bios_block.h) at 0x10000. Then it switches the processor to long mode and
 * starts the 64-bit part (jp_bios_main, bios_entry.c) at 0x9000.
 *
 * The steps (see "The loader" in milestone0.md):
 *   1 A20 line (port 0x92)          5 page tables (identity map of 4 GiB, 2 MiB pages)
 *   2 E820 memory map into the block 6 the descriptor table (GDT)
 *   3 video mode (VBE)               7 protected mode, then long mode
 *   4 kernel.elf from the disk       8 the 64-bit part
 *
 * The 16-bit part is padded to exactly 4 KiB (0x8000 to 0x9000); the 64-bit part
 * follows (bios_loader.ld). Rule of the block: a field is written only after
 * the loader really obtained it; flags tell which groups are valid.
 *
 * Calls and returns in this 16-bit code use 32-bit return addresses (the
 * assembler for the x86_64 target encodes them so, consistently); only this
 * file's own functions are called.
 */

	.intel_syntax noprefix

/* ---- Fixed addresses (the table of the specification) ---- */
	.set BLOCK_SEGMENT, 0x1000          /* the block at 0x10000 */
	.set PAGE_TABLE_SEGMENT, 0x2000     /* page tables at 0x20000 */
	.set PAGE_TABLE_ADDRESS, 0x20000
	.set GDT_SEGMENT, 0x3000            /* the GDT at 0x30000 */
	.set GDT_ADDRESS, 0x30000
	.set STACK_TOP_64, 0x38000          /* the stack of jp_bios_main */
	.set STAGING_ADDRESS, 0x200000      /* the kernel file as read */
	.set STAGING_LIMIT, 0x200000        /* at most 2 MiB */
	.set ENTRY_64, 0x9000
	.set HEADER_ADDRESS, 0x7E00         /* the image header that the boot sector read */
	.set HEADER_KERNEL_LBA, 20
	.set HEADER_KERNEL_SIZE, 28

/* ---- Scratch memory below the loader (free conventional memory) ---- */
	.set VBE_INFO, 0x5000
	.set VBE_MODE_INFO, 0x5200
	.set DISK_PACKET, 0x5400
	.set CHUNK_SEGMENT, 0x4000          /* the disk chunk at 0x40000 */
	.set CHUNK_ADDRESS, 0x40000
	.set CHUNK_SECTORS, 64
	.set STACK_16, 0x7C00

/* ---- Offsets in the block (bios_block.h) ---- */
	.set BLOCK_SIZE_DWORDS, 4096
	.set BLOCK_VERSION, 8
	.set BLOCK_FLAGS, 12
	.set BLOCK_BOOT_DRIVE, 16
	.set BLOCK_E820_COUNT, 20
	.set BLOCK_E820, 24
	.set BLOCK_FRAMEBUFFER_BASE, 3096
	.set BLOCK_WIDTH, 3104
	.set BLOCK_HEIGHT, 3108
	.set BLOCK_PITCH, 3112
	.set BLOCK_VIDEO_FORMAT, 3116
	.set BLOCK_KERNEL_ADDRESS, 3120
	.set BLOCK_KERNEL_SIZE, 3128
	.set FLAG_A20, 1
	.set FLAG_VIDEO, 2
	.set FLAG_KERNEL, 4
	.set E820_ENTRY_SIZE, 24
	.set E820_MAX, 128
	.set E820_SIGNATURE, 0x534D4150     /* "SMAP" */

	.section .text.loader16, "ax"
	.code16
	.globl jp_bios_loader_start
jp_bios_loader_start:
	cli
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, STACK_16
	cld
	mov byte ptr [boot_drive], dl
	call serial_init
	mov al, 'L'                             /* the sign of life of the loader, on a line of its own */
	call serial_putc
	mov al, 0x0D
	call serial_putc
	mov al, 0x0A
	call serial_putc
	sti

	/* ---- 1. the A20 line ---- */
	call a20_is_on
	jnz a20_ready
	in al, 0x92                             /* the "fast A20" port */
	or al, 2
	and al, 0xFE                            /* never touch bit 0: it resets the processor */
	out 0x92, al
	mov cx, 0x4000
a20_wait:
	call a20_is_on
	jnz a20_ready
	loop a20_wait
	mov si, offset msg_a20
	jmp fail
a20_ready:

	/* ---- long mode must exist ---- */
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb no_long_mode
	mov eax, 0x80000001
	cpuid
	test edx, 0x20000000                    /* bit 29: long mode */
	jz no_long_mode

	/* ---- the block: zero it, then the header fields ---- */
	mov ax, BLOCK_SEGMENT
	mov es, ax
	xor di, di
	xor eax, eax
	mov cx, BLOCK_SIZE_DWORDS
	rep stosd
	mov dword ptr es:[0], 0x5453554A        /* "JUST" */
	mov dword ptr es:[4], 0x314B4C42        /* "BLK1" */
	mov dword ptr es:[BLOCK_VERSION], 1
	mov dword ptr es:[BLOCK_FLAGS], FLAG_A20
	movzx eax, byte ptr [boot_drive]
	mov dword ptr es:[BLOCK_BOOT_DRIVE], eax

	/* ---- 2. the E820 memory map ---- */
	xor ebx, ebx                            /* the continuation value, 0 for the first call */
	mov word ptr [e820_count], 0
	mov di, BLOCK_E820
e820_next:
	mov dword ptr es:[di + 20], 1           /* the attributes, in case the BIOS returns only 20 bytes */
	mov eax, 0xE820
	mov edx, E820_SIGNATURE
	mov ecx, E820_ENTRY_SIZE
	int 0x15
	jc e820_done                            /* carry: the list ended (or the call is not supported) */
	cmp eax, E820_SIGNATURE
	jne e820_done
	cmp ecx, 20
	jb e820_skip
	mov eax, dword ptr es:[di + 8]
	or eax, dword ptr es:[di + 12]
	jz e820_skip                            /* a zero-length entry */
	cmp ecx, 24
	jb e820_keep
	test dword ptr es:[di + 20], 1          /* ACPI 3.0: bit 0 clear means "ignore this entry" */
	jz e820_skip
e820_keep:
	inc word ptr [e820_count]
	add di, E820_ENTRY_SIZE
	cmp word ptr [e820_count], E820_MAX
	jae e820_done
e820_skip:
	test ebx, ebx
	jnz e820_next
e820_done:
	movzx eax, word ptr [e820_count]
	mov dword ptr es:[BLOCK_E820_COUNT], eax

	/* The boot environment owns the first 8 MiB, so a usable region must reach
	   from 8 MiB at the latest to 16 MiB at the least. */
	test eax, eax
	jz not_enough_memory
	mov cx, ax
	mov di, BLOCK_E820
memory_check:
	cmp dword ptr es:[di + 16], 1           /* type 1: usable */
	jne memory_next
	cmp dword ptr es:[di + 4], 0            /* the base is below 4 GiB */
	jne memory_next
	mov eax, dword ptr es:[di]
	cmp eax, 0x800000
	ja memory_next
	cmp dword ptr es:[di + 12], 0           /* a length of 4 GiB or more is enough */
	jne memory_ok
	add eax, dword ptr es:[di + 8]
	jc memory_ok
	cmp eax, 0x1000000
	jae memory_ok
memory_next:
	add di, E820_ENTRY_SIZE
	loop memory_check
	jmp not_enough_memory
memory_ok:

	/* ---- 3. the video mode ---- */
	call video_select

	/* ---- 4. the kernel ---- */
	call kernel_read

	/* ---- 5. the page tables: PML4 at 0x20000, PDPT at 0x21000, four page directories ---- */
	mov ax, PAGE_TABLE_SEGMENT
	mov es, ax
	xor di, di
	xor eax, eax
	mov cx, 6144                            /* 24 KiB */
	rep stosd
	mov dword ptr es:[0x0000], 0x21003      /* present + writable */
	mov dword ptr es:[0x1000], 0x22003
	mov dword ptr es:[0x1008], 0x23003
	mov dword ptr es:[0x1010], 0x24003
	mov dword ptr es:[0x1018], 0x25003
	mov di, 0x2000                          /* 2048 entries of 2 MiB: present + writable + large page */
	mov eax, 0x83
	mov cx, 2048
page_directory_fill:
	mov dword ptr es:[di], eax
	add di, 8
	add eax, 0x200000
	loop page_directory_fill

	/* ---- 6. the descriptor table: null, 32-bit code, data, 64-bit code ---- */
	mov ax, GDT_SEGMENT
	mov es, ax
	xor di, di
	mov dword ptr es:[0x00], 0
	mov dword ptr es:[0x04], 0
	mov dword ptr es:[0x08], 0x0000FFFF     /* 0x08: 32-bit code, base 0, limit 4 GiB */
	mov dword ptr es:[0x0C], 0x00CF9A00
	mov dword ptr es:[0x10], 0x0000FFFF     /* 0x10: data */
	mov dword ptr es:[0x14], 0x00CF9200
	mov dword ptr es:[0x18], 0x0000FFFF     /* 0x18: 64-bit code */
	mov dword ptr es:[0x1C], 0x00AF9A00

	/* ---- 7. protected mode ---- */
	cli
	lgdt [gdt_pointer]
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	.byte 0xEA                              /* far jump to 0x08:protected_mode_entry */
	.word protected_mode_entry
	.word 0x08

	.code32
protected_mode_entry:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, STACK_TOP_64
	mov eax, cr4
	or eax, 0x20                            /* CR4.PAE */
	mov cr4, eax
	mov eax, PAGE_TABLE_ADDRESS
	mov cr3, eax
	mov ecx, 0xC0000080                     /* the EFER register */
	rdmsr
	or eax, 0x100                           /* EFER.LME */
	wrmsr
	mov eax, cr0
	or eax, 0x80000000                      /* CR0.PG: long mode becomes active */
	mov cr0, eax
	.byte 0xEA                              /* far jump to 0x18:ENTRY_64 (the 64-bit code segment) */
	.long ENTRY_64
	.word 0x18

	.code16
/* ======== subroutines (16-bit) ======== */

/* ZF is clear when the A20 line is on: a write above 1 MiB must not show up at the same address below 1 MiB. */
a20_is_on:
	push ds
	push es
	xor ax, ax
	mov ds, ax
	mov ax, 0xFFFF
	mov es, ax
	mov al, byte ptr ds:[0x0500]
	mov ah, byte ptr es:[0x0510]
	push ax                                 /* keep the old bytes */
	mov byte ptr ds:[0x0500], 0x00
	mov byte ptr es:[0x0510], 0xFF
	cmp byte ptr ds:[0x0500], 0xFF          /* equal: the two addresses are the same one, A20 is off */
	pop ax
	mov byte ptr ds:[0x0500], al
	mov byte ptr es:[0x0510], ah
	pop es
	pop ds
	ret

/* The serial port COM1, 115200 baud, 8N1 (the output goes to the serial log of the tests). */
serial_init:
	mov dx, 0x3F9
	xor al, al
	out dx, al                              /* no interrupts */
	mov dx, 0x3FB
	mov al, 0x80
	out dx, al                              /* divisor latch on */
	mov dx, 0x3F8
	mov al, 1
	out dx, al                              /* divisor 1: 115200 baud */
	mov dx, 0x3F9
	xor al, al
	out dx, al
	mov dx, 0x3FB
	mov al, 0x03
	out dx, al                              /* 8 bits, no parity, one stop bit */
	mov dx, 0x3FA
	mov al, 0xC7
	out dx, al                              /* enable and clear the FIFO */
	ret

/* Writes the character in AL to COM1 (waits until the transmitter is free). */
serial_putc:
	push ax
	push dx
	push cx
	mov ah, al
	mov cx, 0xFFFF
serial_wait:
	mov dx, 0x3FD
	in al, dx
	test al, 0x20
	jnz serial_send
	loop serial_wait
serial_send:
	mov dx, 0x3F8
	mov al, ah
	out dx, al
	pop cx
	pop dx
	pop ax
	ret

/* Prints the zero-ended text at DS:SI to the serial port and to the screen. */
print_text:
	lodsb
	test al, al
	jz print_done
	call serial_putc
	mov ah, 0x0E
	xor bx, bx
	int 0x10
	jmp print_text
print_done:
	ret

/* Prints "JustOS loader: " and the text at SI, then halts. */
fail:
	xor ax, ax
	mov ds, ax
	push si
	mov si, offset msg_prefix
	call print_text
	pop si
	call print_text
	mov si, offset msg_newline
	call print_text
halt:
	cli
	hlt
	jmp halt

no_long_mode:
	mov si, offset msg_long_mode
	jmp fail
not_enough_memory:
	mov si, offset msg_memory
	jmp fail

/*
 * Chooses a video mode with the VBE functions and sets it. A mode is accepted
 * when it is a graphics mode with a linear framebuffer, 32 bits per pixel,
 * direct color, with 8 bits for each of red, green, blue in the order B,G,R or
 * R,G,B. 1024 x 768 is taken at once; otherwise the largest mode up to
 * 1920 x 1080. No mode is not an error: the block then has no VIDEO flag.
 */
video_select:
	xor ax, ax
	mov es, ax
	mov dword ptr es:[VBE_INFO], 0x32454256 /* "VBE2": ask for the VBE 2 information */
	mov di, VBE_INFO
	mov ax, 0x4F00
	int 0x10
	cmp ax, 0x004F
	jne video_none
	cmp dword ptr es:[VBE_INFO], 0x41534556 /* "VESA" */
	jne video_none
	mov si, word ptr es:[VBE_INFO + 14]     /* the far pointer to the list of modes */
	mov ax, word ptr es:[VBE_INFO + 16]
	mov fs, ax
	mov word ptr [best_mode], 0
	mov dword ptr [best_area], 0
mode_loop:
	mov cx, word ptr fs:[si]
	add si, 2
	cmp cx, 0xFFFF
	je modes_done
	mov di, VBE_MODE_INFO
	mov ax, 0x4F01
	int 0x10
	cmp ax, 0x004F
	jne mode_loop
	mov ax, word ptr es:[VBE_MODE_INFO]     /* attributes: supported (0), graphics (4), linear framebuffer (7) */
	and ax, 0x0091
	cmp ax, 0x0091
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 25], 32      /* bits per pixel */
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 27], 6       /* memory model: direct color */
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 31], 8       /* red, green, blue: 8 bits each */
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 33], 8
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 35], 8
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 34], 8       /* green at bit 8 */
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 32], 16      /* red at bit 16 and blue at bit 0 */
	jne format_rgb
	cmp byte ptr es:[VBE_MODE_INFO + 36], 0
	jne mode_loop
	jmp format_known
format_rgb:
	cmp byte ptr es:[VBE_MODE_INFO + 32], 0
	jne mode_loop
	cmp byte ptr es:[VBE_MODE_INFO + 36], 16
	jne mode_loop
format_known:
	cmp dword ptr es:[VBE_MODE_INFO + 40], 0      /* the framebuffer address must exist */
	je mode_loop
	movzx eax, word ptr es:[VBE_MODE_INFO + 18]   /* width */
	movzx edx, word ptr es:[VBE_MODE_INFO + 20]   /* height */
	mov byte ptr [candidate_format], 1            /* the format of the block: 1 = R,G,B in memory */
	cmp byte ptr es:[VBE_MODE_INFO + 32], 16
	jne format_saved
	mov byte ptr [candidate_format], 2            /* 2 = B,G,R in memory (red at bit 16) */
format_saved:
	cmp eax, 1024
	jne not_preferred
	cmp edx, 768
	jne not_preferred
	mov ebx, 0xFFFFFFFF                     /* the preferred mode beats every area */
	jmp candidate_score
not_preferred:
	cmp eax, 1920
	ja mode_loop
	cmp edx, 1080
	ja mode_loop
	mov ebx, eax
	imul ebx, edx                           /* the area */
candidate_score:
	cmp ebx, dword ptr [best_area]
	jbe mode_loop
	mov dword ptr [best_area], ebx
	mov word ptr [best_mode], cx
	mov word ptr [best_width], ax
	mov word ptr [best_height], dx
	mov ax, word ptr es:[VBE_MODE_INFO + 16]
	mov word ptr [best_pitch], ax
	mov eax, dword ptr es:[VBE_MODE_INFO + 40]
	mov dword ptr [best_base], eax
	mov al, byte ptr [candidate_format]
	mov byte ptr [best_format], al
	cmp ebx, 0xFFFFFFFF
	jne mode_loop
modes_done:
	cmp word ptr [best_mode], 0
	je video_none
	mov ax, 0x4F02                          /* set the mode, with the linear framebuffer */
	mov bx, word ptr [best_mode]
	or bx, 0x4000
	int 0x10
	cmp ax, 0x004F
	jne video_none
	mov ax, BLOCK_SEGMENT
	mov es, ax
	mov eax, dword ptr [best_base]
	mov dword ptr es:[BLOCK_FRAMEBUFFER_BASE], eax
	movzx eax, word ptr [best_width]
	mov dword ptr es:[BLOCK_WIDTH], eax
	movzx eax, word ptr [best_height]
	mov dword ptr es:[BLOCK_HEIGHT], eax
	movzx eax, word ptr [best_pitch]
	mov dword ptr es:[BLOCK_PITCH], eax
	movzx eax, byte ptr [best_format]
	mov dword ptr es:[BLOCK_VIDEO_FORMAT], eax
	or dword ptr es:[BLOCK_FLAGS], FLAG_VIDEO
video_none:
	xor ax, ax
	mov es, ax
	ret

/*
 * Reads kernel.elf (sector and size from the image header) in chunks of 64
 * sectors into the low buffer and copies every chunk to 2 MiB with the BIOS
 * copy function (INT 15h, AH = 87h), which can reach memory above 1 MiB.
 */
kernel_read:
	xor ax, ax
	mov ds, ax
	mov es, ax
	cmp dword ptr [HEADER_ADDRESS + HEADER_KERNEL_SIZE + 4], 0
	jne kernel_too_big
	mov eax, dword ptr [HEADER_ADDRESS + HEADER_KERNEL_SIZE]
	test eax, eax
	jz kernel_bad
	cmp eax, STAGING_LIMIT
	ja kernel_too_big
	mov dword ptr [kernel_size], eax
	add eax, 511
	shr eax, 9
	mov dword ptr [sectors_left], eax       /* the number of sectors */
	cmp dword ptr [HEADER_ADDRESS + HEADER_KERNEL_LBA + 4], 0
	jne kernel_bad
	mov eax, dword ptr [HEADER_ADDRESS + HEADER_KERNEL_LBA]
	mov dword ptr [next_lba], eax
	mov dword ptr [copy_to], STAGING_ADDRESS
kernel_chunk:
	mov eax, dword ptr [sectors_left]
	test eax, eax
	jz kernel_done
	cmp eax, CHUNK_SECTORS
	jbe chunk_size_known
	mov eax, CHUNK_SECTORS
chunk_size_known:
	mov dword ptr [chunk_sectors], eax
	/* the disk address packet: size, sector count, offset, segment, LBA */
	mov word ptr [DISK_PACKET], 0x0010
	mov word ptr [DISK_PACKET + 2], ax
	mov word ptr [DISK_PACKET + 4], 0
	mov word ptr [DISK_PACKET + 6], CHUNK_SEGMENT
	mov eax, dword ptr [next_lba]
	mov dword ptr [DISK_PACKET + 8], eax
	mov dword ptr [DISK_PACKET + 12], 0
	mov si, DISK_PACKET
	mov dl, byte ptr [boot_drive]
	mov ah, 0x42
	int 0x13
	jc kernel_bad
	/* the copy: patch the destination base into the descriptor table of the BIOS function */
	mov eax, dword ptr [copy_to]
	mov word ptr [copy_table + 0x18 + 2], ax
	shr eax, 16
	mov byte ptr [copy_table + 0x18 + 4], al
	mov byte ptr [copy_table + 0x18 + 7], ah
	mov eax, dword ptr [chunk_sectors]
	shl eax, 8                              /* the number of 16-bit words: sectors * 256 */
	mov cx, ax
	mov si, offset copy_table
	mov ah, 0x87
	int 0x15
	jc kernel_bad
	test ah, ah
	jnz kernel_bad
	mov eax, dword ptr [chunk_sectors]
	sub dword ptr [sectors_left], eax
	add dword ptr [next_lba], eax
	shl eax, 9
	add dword ptr [copy_to], eax
	jmp kernel_chunk
kernel_done:
	mov ax, BLOCK_SEGMENT
	mov es, ax
	mov dword ptr es:[BLOCK_KERNEL_ADDRESS], STAGING_ADDRESS
	mov eax, dword ptr [kernel_size]
	mov dword ptr es:[BLOCK_KERNEL_SIZE], eax
	or dword ptr es:[BLOCK_FLAGS], FLAG_KERNEL
	xor ax, ax
	mov es, ax
	ret
kernel_bad:
	mov si, offset msg_kernel
	jmp fail
kernel_too_big:
	mov si, offset msg_kernel_big
	jmp fail

/* ======== data ======== */
	.balign 4
gdt_pointer:
	.word 31                                /* the limit: 4 entries of 8 bytes, minus 1 */
	.long GDT_ADDRESS

/*
 * The table of INT 15h AH = 87h: 6 descriptors of 8 bytes. 0x00 is unused,
 * 0x08 is filled by the BIOS (the table itself), 0x10 is the source, 0x18 is
 * the destination, 0x20 and 0x28 are filled by the BIOS (its own code and stack).
 */
	.balign 4
copy_table:
	.fill 0x10, 1, 0
	.word 0xFFFF, 0x0000                    /* 0x10: the source, limit 64 KiB, base 0x40000 */
	.byte 0x04, 0x93, 0x00, 0x00
	.word 0xFFFF, 0x0000                    /* 0x18: the destination, the base is patched for every chunk */
	.byte 0x00, 0x93, 0x00, 0x00
	.fill 0x10, 1, 0

boot_drive:      .byte 0
best_format:     .byte 0
candidate_format: .byte 0
	.balign 4
e820_count:      .word 0
best_mode:       .word 0
best_width:      .word 0
best_height:     .word 0
best_pitch:      .word 0
	.balign 4
best_area:       .long 0
best_base:       .long 0
kernel_size:     .long 0
sectors_left:    .long 0
next_lba:        .long 0
copy_to:         .long 0
chunk_sectors:   .long 0

msg_prefix:      .asciz "JustOS loader: "
msg_newline:     .asciz "\r\n"
msg_a20:         .asciz "the A20 line cannot be enabled"
msg_long_mode:   .asciz "the processor has no long mode"
msg_memory:      .asciz "at least 16 MiB of memory are needed"
msg_kernel:      .asciz "the kernel cannot be read"
msg_kernel_big:  .asciz "the kernel is larger than 2 MiB"

/*
 * The 64-bit start (at 0x9000, the first byte of the 64-bit part). The
 * protected-mode code jumped here with long mode active: load the data
 * segment registers with the null selector, put the stack at its top, give
 * jp_bios_main the address of the block in RDI (a 32-bit move clears the upper
 * half), and call it. It does not return; if it does, the processor halts.
 */
	.section .text.entry64, "ax"
	.code64
	.globl jp_bios_entry_64
jp_bios_entry_64:
	xor eax, eax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp, STACK_TOP_64
	mov edi, 0x10000
	call jp_bios_main
entry_halt:
	cli
	hlt
	jmp entry_halt
