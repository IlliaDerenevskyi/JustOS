/*
 * stack_hw_x86_64.c - reads RSP. Built only for x86_64. No logic.
 */

#include "stack_hw.h"

uint64_t jp_kernel_stack_current_pointer(void)
{
	uint64_t pointer;

	__asm__ volatile("movq %%rsp, %0" : "=r"(pointer));
	return pointer;
}
