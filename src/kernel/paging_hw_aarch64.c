/*
 * paging_hw_aarch64.c - the AArch64 machine access of paging_hw.h.
 *
 * Purpose:
 *   Reading and writing TTBR0_EL1 with the synchronization that a change of
 *   the translation tables needs, setting SCTLR_EL1.WXN, and turning a table
 *   address into a pointer. Built only for AArch64.
 *
 * Limitations:
 *   No logic. The identity map makes the physical address of a table its
 *   virtual address, so the pointer is the same number.
 */

#include "paging_hw.h"

#define JP_AARCH64_TTBR_ROOT_MASK 0x0000FFFFFFFFF000ULL
#define JP_AARCH64_SCTLR_WXN (1ULL << 19)

uint64_t *jp_kernel_paging_table_at(uint64_t physical)
{
	return (uint64_t *)(uintptr_t)physical;
}

uint64_t jp_kernel_paging_read_root(void)
{
	uint64_t ttbr0;

	__asm__ volatile("mrs %0, TTBR0_EL1" : "=r"(ttbr0));
	return ttbr0 & JP_AARCH64_TTBR_ROOT_MASK;
}

void jp_kernel_paging_write_root(uint64_t root)
{
	/*
	 * Finish the writes to the new tables (dsb ishst), write the root,
	 * drop every cached translation (tlbi vmalle1), wait for it (dsb ish),
	 * and refetch the instructions after it (isb).
	 */
	__asm__ volatile(
		"dsb ishst\n\t"
		"msr TTBR0_EL1, %0\n\t"
		"tlbi vmalle1\n\t"
		"dsb ish\n\t"
		"isb"
		: : "r"(root) : "memory");
}

void jp_kernel_aarch64_set_wxn(void)
{
	uint64_t sctlr;

	__asm__ volatile("mrs %0, SCTLR_EL1" : "=r"(sctlr));
	sctlr |= JP_AARCH64_SCTLR_WXN;
	__asm__ volatile("msr SCTLR_EL1, %0\n\tisb" : : "r"(sctlr) : "memory");
}
