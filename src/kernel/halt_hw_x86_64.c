/*
 * halt_hw_x86_64.c - stops the processor. Built only for x86_64. No logic.
 */

#include "halt_hw.h"

_Noreturn void jp_kernel_halt(void)
{
	for (;;) {
		__asm__ volatile("cli; hlt");
	}
}
