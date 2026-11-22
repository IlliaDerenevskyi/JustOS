/*
 * cpu.c - Stage 2 CPU initialization.
 *
 * Purpose:
 *   Checks the processor state that Stage 1 handed over, sets the bits that
 *   Stage 2 needs, and reads everything back. The exact steps are in
 *   "Decided rules (mandatory CPU state)" of milestone0.md.
 *
 * Responsibilities:
 *   - x86_64: long mode, privilege level 0, IF = 0, "no execute" support;
 *     sets EFER.NXE and CR0.WP;
 *   - AArch64: EL1, DAIF = 1111, MMU on, TCR_EL1 and MAIR_EL1 as Stage 1
 *     wrote them; writes nothing.
 *
 * Limitations:
 *   Never changes the interrupt state. Does not turn on SSE or the
 *   floating-point unit. Does not touch page tables, the stack, or memory.
 *
 * Depends on:
 *   justos/kernel/cpu.h, cpu_regs.h, arch_dispatch.h.
 *
 * Must not depend on:
 *   firmware, JPBootInfo, Stage 3, Stage 4.
 */

#include "justos/kernel/cpu.h"

#include "arch_dispatch.h"
#include "cpu_regs.h"

/* ---- Private helpers: x86_64 ---- */

#if !defined(JP_KERNEL_ONLY_AARCH64)

static bool x86_64_state_is_valid(const JPKernelX86Registers *registers)
{
	return (registers->cr0 & JP_X86_64_CR0_PE) != 0 &&
	       (registers->cr0 & JP_X86_64_CR0_PG) != 0 &&
	       (registers->cr4 & JP_X86_64_CR4_PAE) != 0 &&
	       (registers->efer & JP_X86_64_EFER_LME) != 0 &&
	       (registers->efer & JP_X86_64_EFER_LMA) != 0 &&
	       (registers->cs & JP_X86_64_CS_PRIVILEGE_MASK) == 0 &&
	       (registers->rflags & JP_X86_64_RFLAGS_IF) == 0;
}

static JPBootResult x86_64_initialize(void)
{
	JPKernelX86Registers registers;

	jp_kernel_x86_64_read_registers(&registers);
	if (!x86_64_state_is_valid(&registers) || !jp_kernel_x86_64_nx_supported()) {
		return JP_BOOT_FAILED;
	}
	if ((registers.efer & JP_X86_64_EFER_NXE) == 0) {
		jp_kernel_x86_64_write_efer(registers.efer | JP_X86_64_EFER_NXE);
	}
	if ((registers.cr0 & JP_X86_64_CR0_WP) == 0) {
		jp_kernel_x86_64_write_cr0(registers.cr0 | JP_X86_64_CR0_WP);
	}

	/* Writing is not proof: read the registers again. */
	jp_kernel_x86_64_read_registers(&registers);
	if (!x86_64_state_is_valid(&registers) ||
	    (registers.efer & JP_X86_64_EFER_NXE) == 0 || (registers.cr0 & JP_X86_64_CR0_WP) == 0) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}

#endif /* !JP_KERNEL_ONLY_AARCH64 */

/* ---- Private helpers: AArch64 ---- */

#if !defined(JP_KERNEL_ONLY_X86_64)

static uint64_t field(uint64_t value, unsigned int shift, uint64_t mask)
{
	return (value >> shift) & mask;
}

static bool aarch64_tcr_is_valid(uint64_t tcr)
{
	return field(tcr, 0, JP_AARCH64_TCR_T0SZ_MASK) == JP_AARCH64_TCR_SIZE_48_BITS &&
	       field(tcr, JP_AARCH64_TCR_T1SZ_SHIFT, JP_AARCH64_TCR_T1SZ_MASK) == JP_AARCH64_TCR_SIZE_48_BITS &&
	       field(tcr, JP_AARCH64_TCR_TG0_SHIFT, JP_AARCH64_TCR_TG0_MASK) == JP_AARCH64_TCR_TG0_4K &&
	       field(tcr, JP_AARCH64_TCR_TG1_SHIFT, JP_AARCH64_TCR_TG1_MASK) == JP_AARCH64_TCR_TG1_4K &&
	       (tcr & JP_AARCH64_TCR_EPD0) == 0 &&
	       (tcr & JP_AARCH64_TCR_EPD1) == 0;
}

static JPBootResult aarch64_initialize(void)
{
	JPKernelAArch64Registers registers;

	jp_kernel_aarch64_read_registers(&registers);
	if (field(registers.current_el, JP_AARCH64_CURRENT_EL_SHIFT, JP_AARCH64_CURRENT_EL_MASK) != JP_AARCH64_EL1 ||
	    field(registers.daif, JP_AARCH64_DAIF_SHIFT, JP_AARCH64_DAIF_MASK) != JP_AARCH64_DAIF_MASK ||
	    (registers.sctlr_el1 & JP_AARCH64_SCTLR_M) == 0 ||
	    !aarch64_tcr_is_valid(registers.tcr_el1) ||
	    registers.mair_el1 != JP_AARCH64_MAIR_STAGE1) {
		return JP_BOOT_FAILED;
	}
	return JP_BOOT_SUCCESS;
}

#endif /* !JP_KERNEL_ONLY_X86_64 */

/* ---- Public API ---- */

JPBootResult jp_kernel_cpu_initialize(JPBootContext *context)
{
	JPArchitectureType architecture;

	if (context == NULL) {
		return JP_BOOT_ERROR_INVALID_PARAM;
	}
	architecture = context->handoff.architecture;
	if (!JP_KERNEL_ARCH_IS_SUPPORTED(architecture)) {
		return JP_BOOT_FAILED;
	}
	return JP_KERNEL_ARCH_DISPATCH(architecture, x86_64_initialize(), aarch64_initialize());
}
