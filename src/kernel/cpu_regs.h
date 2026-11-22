/*
 * cpu_regs.h - private access to processor registers for the Stage 2 modules.
 *
 * Purpose:
 *   The one place that names the instructions that read or write registers,
 *   so that the logic of the modules can be tested on the host with a
 *   scripted replacement. The real functions are in cpu_regs_x86_64.c and
 *   cpu_regs_aarch64.c (inline assembly); the QEMU tests run them.
 *
 * Responsibilities:
 *   - read the registers that cpu.c checks, write the two bits it sets;
 *   - the width of a physical address (used by memory.c).
 *
 * Limitations:
 *   Private to src/kernel. Not a public API; the declarations grow with the
 *   modules that need them.
 */

#ifndef JUSTOS_SRC_KERNEL_CPU_REGS_H
#define JUSTOS_SRC_KERNEL_CPU_REGS_H

#include <stdbool.h>
#include <stdint.h>

/* ---- x86_64 ---- */

#define JP_X86_64_CR0_PE (1ULL << 0)
#define JP_X86_64_CR0_WP (1ULL << 16)
#define JP_X86_64_CR0_PG (1ULL << 31)
#define JP_X86_64_CR4_PAE (1ULL << 5)
#define JP_X86_64_EFER_LME (1ULL << 8)
#define JP_X86_64_EFER_LMA (1ULL << 10)
#define JP_X86_64_EFER_NXE (1ULL << 11)
#define JP_X86_64_RFLAGS_IF (1ULL << 9)
#define JP_X86_64_CS_PRIVILEGE_MASK 3ULL

typedef struct JPKernelX86Registers {
	uint64_t rflags;
	uint64_t cr0;
	uint64_t cr4;
	uint64_t efer;
	uint64_t cs;
} JPKernelX86Registers;

void jp_kernel_x86_64_read_registers(JPKernelX86Registers *registers);
/* CPUID leaf 0x80000001, EDX bit 20. */
bool jp_kernel_x86_64_nx_supported(void);
void jp_kernel_x86_64_write_cr0(uint64_t value);
void jp_kernel_x86_64_write_efer(uint64_t value);

/* ---- AArch64 ---- */

#define JP_AARCH64_CURRENT_EL_SHIFT 2
#define JP_AARCH64_CURRENT_EL_MASK 3ULL
#define JP_AARCH64_EL1 1ULL
#define JP_AARCH64_DAIF_SHIFT 6
#define JP_AARCH64_DAIF_MASK 0xFULL
#define JP_AARCH64_SCTLR_M (1ULL << 0)
#define JP_AARCH64_TCR_T0SZ_MASK 0x3FULL
#define JP_AARCH64_TCR_EPD0 (1ULL << 7)
#define JP_AARCH64_TCR_TG0_SHIFT 14
#define JP_AARCH64_TCR_TG0_MASK 3ULL
#define JP_AARCH64_TCR_T1SZ_SHIFT 16
#define JP_AARCH64_TCR_T1SZ_MASK 0x3FULL
#define JP_AARCH64_TCR_EPD1 (1ULL << 23)
#define JP_AARCH64_TCR_TG1_SHIFT 30
#define JP_AARCH64_TCR_TG1_MASK 3ULL
#define JP_AARCH64_TCR_SIZE_48_BITS 16ULL    /* T0SZ and T1SZ for 48-bit addresses */
#define JP_AARCH64_TCR_TG0_4K 0ULL
#define JP_AARCH64_TCR_TG1_4K 2ULL
#define JP_AARCH64_MAIR_STAGE1 0xFF00ULL     /* attribute 0 device, attribute 1 normal write-back */

typedef struct JPKernelAArch64Registers {
	uint64_t current_el;
	uint64_t daif;
	uint64_t sctlr_el1;
	uint64_t tcr_el1;
	uint64_t mair_el1;
} JPKernelAArch64Registers;

void jp_kernel_aarch64_read_registers(JPKernelAArch64Registers *registers);

/* ---- Both ---- */

/*
 * The width of a physical address in bits; 0 if the processor does not say.
 * x86_64: CPUID leaf 0x80000008, EAX bits 7:0. AArch64: decoded PARange.
 */
uint32_t jp_kernel_cpu_physical_address_bits(void);

/* Decodes ID_AA64MMFR0_EL1.PARange (bits 3:0). Pure; defined here so that the host tests can run it. */
static inline uint32_t jp_kernel_aarch64_parange_to_bits(uint64_t id_aa64mmfr0)
{
	static const uint32_t bits[] = { 32, 36, 40, 42, 44, 48, 52 };
	uint64_t parange = id_aa64mmfr0 & 0xFULL;

	return parange < sizeof bits / sizeof bits[0] ? bits[parange] : 0;
}

#endif /* JUSTOS_SRC_KERNEL_CPU_REGS_H */
