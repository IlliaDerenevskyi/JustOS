/*
 * entry_aarch64.S - the Stage 2 entry stub (AArch64).
 *
 * Stage 1 jumps here at EL1 with interrupts masked, a valid 16-byte aligned
 * stack, and the pointer to the context in X0.
 *
 *   1. keep the context in X19 (a register that C functions keep);
 *   2. call jp_kernel_init(context); the 32-bit result is in W0;
 *   3. zero (JP_BOOT_SUCCESS): call jp_kernel_stage2_complete(context);
 *   4. anything else: call jp_kernel_stage2_failure(context, result), the
 *      result in W1.
 *
 * Neither of the last two calls returns; if one does, the processor stops.
 * The stub enables no interrupt and touches no other state.
 */

	.text
	.globl just_kernel_entry_aarch64
	.type just_kernel_entry_aarch64, %function
just_kernel_entry_aarch64:
	stp x19, x30, [sp, #-16]!         /* the stack stays 16-byte aligned */
	mov x19, x0
	bl jp_kernel_init
	cbnz w0, 1f
	mov x0, x19
	bl jp_kernel_stage2_complete
	b 2f
1:
	mov w1, w0
	mov x0, x19
	bl jp_kernel_stage2_failure
2:
	wfi
	b 2b
	.size just_kernel_entry_aarch64, . - just_kernel_entry_aarch64
