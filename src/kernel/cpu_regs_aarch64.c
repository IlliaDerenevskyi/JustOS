/*
 * cpu_regs_aarch64.c - the AArch64 register access of cpu_regs.h.
 *
 * Purpose:
 *   The instructions that C cannot write: reading CurrentEL, DAIF, SCTLR_EL1,
 *   TCR_EL1, MAIR_EL1 and ID_AA64MMFR0_EL1. Built only for AArch64.
 *
 * Limitations:
 *   No logic and no writes: the decoding of PARange is in cpu_regs.h.
 */

#include "cpu_regs.h"

void jp_kernel_aarch64_read_registers(JPKernelAArch64Registers *registers)
{
	__asm__ volatile("mrs %0, CurrentEL" : "=r"(registers->current_el));
	__asm__ volatile("mrs %0, DAIF" : "=r"(registers->daif));
	__asm__ volatile("mrs %0, SCTLR_EL1" : "=r"(registers->sctlr_el1));
	__asm__ volatile("mrs %0, TCR_EL1" : "=r"(registers->tcr_el1));
	__asm__ volatile("mrs %0, MAIR_EL1" : "=r"(registers->mair_el1));
}

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	uint64_t id_aa64mmfr0;

	__asm__ volatile("mrs %0, ID_AA64MMFR0_EL1" : "=r"(id_aa64mmfr0));
	return jp_kernel_aarch64_parange_to_bits(id_aa64mmfr0);
}
