/*
 * entry_x86_64.S - the Stage 2 entry stub (x86_64).
 *
 * Stage 1 jumps here in long mode with interrupts masked, a valid stack
 * (a zero return address is on it, so RSP is 8 modulo 16, as after a call),
 * and the pointer to the context in RDI.
 *
 *   1. keep the context in RBX (a register that C functions keep);
 *   2. call jp_kernel_init(context); the 32-bit result is in EAX;
 *   3. zero (JP_BOOT_SUCCESS): call jp_kernel_stage2_complete(context);
 *   4. anything else: call jp_kernel_stage2_failure(context, result), the
 *      result in ESI.
 *
 * Neither of the last two calls returns; if one does, the processor stops.
 * The stub enables no interrupt and touches no other state.
 */

	.intel_syntax noprefix

	.text
	.globl just_kernel_entry_x86_64
	.type just_kernel_entry_x86_64, @function
just_kernel_entry_x86_64:
	push rbx                          /* RSP is now 0 modulo 16: the alignment that a call needs */
	mov rbx, rdi
	call jp_kernel_init
	test eax, eax
	jnz 1f
	mov rdi, rbx
	call jp_kernel_stage2_complete
	jmp 2f
1:
	mov esi, eax
	mov rdi, rbx
	call jp_kernel_stage2_failure
2:
	cli
	hlt
	jmp 2b
	.size just_kernel_entry_x86_64, . - just_kernel_entry_x86_64

	.section .note.GNU-stack, "", @progbits
