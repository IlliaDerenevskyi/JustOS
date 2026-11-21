/*
 * diagnostic_hw_aarch64.c - the PL011 serial port and halt for AArch64. No logic.
 */

#include "diagnostic_hw.h"

#define JP_DIAGNOSTIC_PL011_BASE 0x09000000UL

uint32_t jp_diagnostic_serial_read(uint32_t offset)
{
	return *(volatile uint32_t *)(JP_DIAGNOSTIC_PL011_BASE + offset);
}

void jp_diagnostic_serial_write(uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *)(JP_DIAGNOSTIC_PL011_BASE + offset) = value;
}

_Noreturn void jp_diagnostic_halt(void)
{
	for (;;) {
		__asm__ volatile("msr daifset, #0xf\n\twfi");
	}
}
