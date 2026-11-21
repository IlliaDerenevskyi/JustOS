/*
 * diagnostic_hw_x86_64.c - serial port COM1 and halt for x86_64. No logic.
 */

#include "diagnostic_hw.h"

#define JP_DIAGNOSTIC_COM1_BASE 0x3F8U

uint32_t jp_diagnostic_serial_read(uint32_t offset)
{
	uint8_t value;
	uint16_t port = (uint16_t)(JP_DIAGNOSTIC_COM1_BASE + offset);

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

void jp_diagnostic_serial_write(uint32_t offset, uint32_t value)
{
	uint16_t port = (uint16_t)(JP_DIAGNOSTIC_COM1_BASE + offset);
	uint8_t byte = (uint8_t)value;

	__asm__ volatile("outb %0, %1" : : "a"(byte), "Nd"(port));
}

_Noreturn void jp_diagnostic_halt(void)
{
	for (;;) {
		__asm__ volatile("cli; hlt");
	}
}
