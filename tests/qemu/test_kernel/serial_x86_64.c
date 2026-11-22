#include <stdint.h>

#include "platform.h"

enum {
	JP_COM1_DATA_PORT = 0x3F8,
	JP_COM1_LINE_STATUS_PORT = 0x3FD,
	JP_LINE_STATUS_TX_EMPTY = 0x20,
	JP_DEBUG_EXIT_PORT = 0xF4,
	JP_DEBUG_EXIT_PASS_VALUE = 0x10,
	JP_DEBUG_EXIT_FAIL_VALUE = 0x11
};

static void outb(uint16_t port, uint8_t value)
{
	__asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port)
{
	uint8_t value;

	__asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

void jp_test_serial_put(char c)
{
	while ((inb(JP_COM1_LINE_STATUS_PORT) & JP_LINE_STATUS_TX_EMPTY) == 0) {
	}
	outb(JP_COM1_DATA_PORT, (uint8_t)c);
}

void jp_test_exit(bool pass)
{
	outb(JP_DEBUG_EXIT_PORT, pass ? JP_DEBUG_EXIT_PASS_VALUE : JP_DEBUG_EXIT_FAIL_VALUE);
	for (;;) {
		__asm__ volatile("cli; hlt");
	}
}
