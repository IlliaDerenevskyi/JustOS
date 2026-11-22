/*
 * paging_hw_x86_64.c - the x86_64 machine access of paging_hw.h.
 *
 * Purpose:
 *   Reading and writing CR3, and turning a table address into a pointer.
 *   Built only for x86_64.
 *
 * Limitations:
 *   No logic. The identity map makes the physical address of a table its
 *   virtual address, so the pointer is the same number.
 */

#include "paging_hw.h"

#define JP_X86_64_CR3_ROOT_MASK 0x000FFFFFFFFFF000ULL

uint64_t *jp_kernel_paging_table_at(uint64_t physical)
{
	return (uint64_t *)(uintptr_t)physical;
}

uint64_t jp_kernel_paging_read_root(void)
{
	uint64_t cr3;

	__asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
	return cr3 & JP_X86_64_CR3_ROOT_MASK;
}

void jp_kernel_paging_write_root(uint64_t root)
{
	/* Writing CR3 flushes the translations that are not global. */
	__asm__ volatile("movq %0, %%cr3" : : "r"(root) : "memory");
}
