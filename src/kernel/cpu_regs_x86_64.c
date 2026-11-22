/*
 * cpu_regs_x86_64.c - the x86_64 register access of cpu_regs.h.
 *
 * Purpose:
 *   The instructions that C cannot write: reading RFLAGS, CR0, CR4, EFER and
 *   CS, CPUID, and writing CR0 and EFER. Built only for x86_64.
 *
 * Limitations:
 *   No logic: every function is one or two instructions. The kernel is built
 *   without SSE (-mgeneral-regs-only), so nothing here uses it.
 */

#include "cpu_regs.h"

#define JP_X86_64_MSR_EFER 0xC0000080U
#define JP_X86_64_CPUID_EXTENDED_MAX 0x80000000U
#define JP_X86_64_CPUID_EXTENDED_FEATURES 0x80000001U
#define JP_X86_64_CPUID_ADDRESS_SIZES 0x80000008U
#define JP_X86_64_CPUID_NX_EDX_BIT (1U << 20)
#define JP_X86_64_CPUID_PHYSICAL_BITS_MASK 0xFFU

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf), "c"(0U));
}

static uint32_t highest_extended_leaf(void)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	cpuid(JP_X86_64_CPUID_EXTENDED_MAX, &eax, &ebx, &ecx, &edx);
	return eax;
}

void jp_kernel_x86_64_read_registers(JPKernelX86Registers *registers)
{
	uint64_t rflags;
	uint64_t cs;
	uint32_t efer_low;
	uint32_t efer_high;

	__asm__ volatile("pushfq; popq %0" : "=r"(rflags) : : "memory");
	__asm__ volatile("movq %%cr0, %0" : "=r"(registers->cr0));
	__asm__ volatile("movq %%cr4, %0" : "=r"(registers->cr4));
	__asm__ volatile("rdmsr" : "=a"(efer_low), "=d"(efer_high) : "c"(JP_X86_64_MSR_EFER));
	__asm__ volatile("movq %%cs, %0" : "=r"(cs));
	registers->rflags = rflags;
	registers->efer = ((uint64_t)efer_high << 32) | efer_low;
	registers->cs = cs;
}

bool jp_kernel_x86_64_nx_supported(void)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	if (highest_extended_leaf() < JP_X86_64_CPUID_EXTENDED_FEATURES) {
		return false;
	}
	cpuid(JP_X86_64_CPUID_EXTENDED_FEATURES, &eax, &ebx, &ecx, &edx);
	return (edx & JP_X86_64_CPUID_NX_EDX_BIT) != 0;
}

void jp_kernel_x86_64_write_cr0(uint64_t value)
{
	__asm__ volatile("movq %0, %%cr0" : : "r"(value) : "memory");
}

void jp_kernel_x86_64_write_efer(uint64_t value)
{
	__asm__ volatile("wrmsr" : : "a"((uint32_t)value), "d"((uint32_t)(value >> 32)),
		"c"(JP_X86_64_MSR_EFER) : "memory");
}

uint32_t jp_kernel_cpu_physical_address_bits(void)
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	if (highest_extended_leaf() < JP_X86_64_CPUID_ADDRESS_SIZES) {
		return 0;
	}
	cpuid(JP_X86_64_CPUID_ADDRESS_SIZES, &eax, &ebx, &ecx, &edx);
	return eax & JP_X86_64_CPUID_PHYSICAL_BITS_MASK;
}
