/*
 * stack_hw_aarch64.c - reads SP. Built only for AArch64. No logic.
 */

#include "stack_hw.h"

uint64_t jp_kernel_stack_current_pointer(void)
{
	uint64_t pointer;

	__asm__ volatile("mov %0, sp" : "=r"(pointer));
	return pointer;
}
