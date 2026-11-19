/*
 * aarch64_enter.S - the AArch64 instructions that cannot be written in C.
 *
 * Four functions (declared in aarch64_cpu.h), called with the standard AArch64
 * argument registers (X0 to X4). The jump follows the steps of "Decided rules
 * (AArch64: exception level, translation, and the jump)" in milestone0.md.
 */

	.text
	.p2align 2

/* void jp_aarch64_disable_interrupts(void) */
	.globl jp_aarch64_disable_interrupts
jp_aarch64_disable_interrupts:
	msr daifset, #0xf
	ret

/* void jp_aarch64_read_cpu_state(JPA64CpuState *state): current EL, DAIF, SCTLR, MIDR_EL1 */
	.globl jp_aarch64_read_cpu_state
jp_aarch64_read_cpu_state:
	mrs x1, CurrentEL
	lsr x1, x1, #2
	str x1, [x0]
	mrs x2, daif
	lsr x2, x2, #6
	and x2, x2, #0xf
	str x2, [x0, #8]
	cmp x1, #2
	b.eq 1f
	mrs x3, sctlr_el1
	b 2f
1:
	mrs x3, sctlr_el2
2:
	str x3, [x0, #16]
	mrs x4, midr_el1
	str x4, [x0, #24]
	ret

/* _Noreturn void jp_aarch64_park(void) */
	.globl jp_aarch64_park
jp_aarch64_park:
	msr daifset, #0xf
1:
	wfe
	b 1b

/*
 * _Noreturn void jp_aarch64_jump_to_kernel(ttbr1 = X0, ttbr0 = X1, stack = X2,
 *                                          entry = X3, context = X4)
 */
	.globl jp_aarch64_jump_to_kernel
jp_aarch64_jump_to_kernel:
	msr daifset, #0xf
	mov x9, x0                       /* TTBR1 root */
	mov x10, x1                      /* TTBR0 root */
	mov x11, x2                      /* stack */
	mov x12, x3                      /* entry */
	mov x13, x4                      /* context */

	/* MAIR_EL1: attribute 0 is device memory, attribute 1 is normal write-back memory. */
	mov x6, #0xff00
	msr mair_el1, x6

	/* TCR_EL1: fixed part 0xB5103510, IPS copied from the processor (at most 5). */
	mrs x6, id_aa64mmfr0_el1
	and x6, x6, #0xf
	mov x7, #5
	cmp x6, x7
	csel x6, x6, x7, lo
	movz x7, #0x3510
	movk x7, #0xb510, lsl #16
	orr x7, x7, x6, lsl #32
	msr tcr_el1, x7

	msr ttbr0_el1, x10
	msr ttbr1_el1, x9
	isb

	mrs x5, CurrentEL
	lsr x5, x5, #2
	cmp x5, #2
	b.eq 3f

	/* EL1: synchronize, then turn the MMU and the caches on. WXN stays clear. */
	dsb ish
	tlbi vmalle1
	dsb ish
	isb
	mrs x6, sctlr_el1
	mov x7, #0x1005                  /* M (bit 0), C (bit 2), I (bit 12) */
	orr x6, x6, x7
	msr sctlr_el1, x6
	isb
	mov sp, x11
	mov x0, x13
	br x12

3:
	/* EL2: prepare EL1 and drop to it. HCR_EL2.RW = 1 and no other bit. */
	mov x6, #0x80000000
	msr hcr_el2, x6
	movz x6, #0x0800                 /* SCTLR_EL1: the reserved-to-one bits, with M, C, I */
	movk x6, #0x30d0, lsl #16
	mov x7, #0x1005
	orr x6, x6, x7
	msr sctlr_el1, x6
	dsb sy
	tlbi vmalle1
	dsb sy
	isb
	msr sp_el1, x11
	mov x6, #0x3c5                   /* EL1 with its own stack, all interrupts masked */
	msr spsr_el2, x6
	adr x6, 4f
	msr elr_el2, x6
	eret
4:
	mov x0, x13
	br x12
