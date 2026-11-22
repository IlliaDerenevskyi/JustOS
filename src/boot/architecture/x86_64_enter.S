/*
 * x86_64_enter.S - the x86_64 instructions that cannot be written in C.
 *
 * Four functions (declared in x86_64_cpu.h). All take their arguments in the
 * System V registers (RDI, RSI, RDX, RCX, R8): the C code is compiled with
 * the sysv_abi attribute for them, also when it is built for UEFI.
 *
 * The jump follows the steps of "Decided rules (x86_64: paging, descriptor
 * table, and the jump)" in milestone0.md.
 */

	.intel_syntax noprefix
	.text

/* void jp_x86_64_disable_interrupts(void) */
	.globl jp_x86_64_disable_interrupts
jp_x86_64_disable_interrupts:
	cli
	ret

/* void jp_x86_64_read_cpu_state(JPX86CpuState *state): RFLAGS, CR0, CR4, EFER in this order */
	.globl jp_x86_64_read_cpu_state
jp_x86_64_read_cpu_state:
	pushfq
	pop rax
	mov [rdi], rax
	mov rax, cr0
	mov [rdi + 8], rax
	mov rax, cr4
	mov [rdi + 16], rax
	mov ecx, 0xC0000080            /* the EFER register */
	rdmsr
	shl rdx, 32
	or rax, rdx
	mov [rdi + 24], rax
	ret

/* _Noreturn void jp_x86_64_park(void) */
	.globl jp_x86_64_park
jp_x86_64_park:
	cli
1:
	hlt
	jmp 1b

/*
 * _Noreturn void jp_x86_64_jump_to_kernel(root = RDI, gdt = RSI, stack = RDX,
 *                                         entry = RCX, context = R8)
 */
	.globl jp_x86_64_jump_to_kernel
jp_x86_64_jump_to_kernel:
	cli
	mov r10, rcx                    /* entry */
	mov r9, rdx                     /* stack */
	mov r11, rdi                    /* root of the page tables */

	/* EFER.NXE (bit 11): the no-execute bit of the page tables is honoured. */
	mov ecx, 0xC0000080
	rdmsr
	or eax, 0x800
	wrmsr

	/* CR0.WP (bit 16): read-only pages are read-only for the kernel too. */
	mov rax, cr0
	or rax, 0x10000
	mov cr0, rax

	/* Load the GDT: a 10-byte descriptor (limit 23, base) built on the current stack. */
	sub rsp, 16
	mov word ptr [rsp], 23
	mov [rsp + 2], rsi
	lgdt [rsp]
	add rsp, 16

	/* The data segment registers get selector 0x10. */
	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax
	xor eax, eax
	mov fs, ax
	mov gs, ax

	/* CS gets selector 0x08 through a far return. */
	push 0x08
	lea rax, [rip + 2f]
	push rax
	retfq
2:
	mov cr3, r11                    /* the new page tables */
	mov rsp, r9                     /* the kernel stack */
	push 0                          /* the return address of a call that never happened */
	mov rdi, r8                     /* the context is the first argument of the kernel entry */
	jmp r10

#ifdef __ELF__
	.section .note.GNU-stack, "", @progbits
#endif
