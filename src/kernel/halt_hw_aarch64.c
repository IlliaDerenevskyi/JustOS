/*
 * halt_hw_aarch64.c - stops the processor. Built only for AArch64. No logic.
 */

#include "halt_hw.h"

_Noreturn void jp_kernel_halt(void)
{
	for (;;) {
		__asm__ volatile("msr daifset, #0xf\n\twfi");
	}
}
