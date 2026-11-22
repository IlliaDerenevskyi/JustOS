#include <stdint.h>

#include "platform.h"

enum {
	JP_SEMIHOSTING_SYS_EXIT_EXTENDED = 0x20,
	JP_SEMIHOSTING_APPLICATION_EXIT = 0x20026,
	JP_EXIT_CODE_PASS = 0,
	JP_EXIT_CODE_FAIL = 1
};

/* PL011 UART of the QEMU "virt" machine. */
#define JP_PL011_BASE 0x09000000UL
#define JP_PL011_FLAGS_OFFSET 0x18UL
#define JP_PL011_FLAG_TX_FULL 0x20U

void jp_test_serial_put(char c)
{
	volatile uint32_t *data = (volatile uint32_t *)JP_PL011_BASE;
	volatile uint32_t *flags = (volatile uint32_t *)(JP_PL011_BASE + JP_PL011_FLAGS_OFFSET);

	while ((*flags & JP_PL011_FLAG_TX_FULL) != 0) {
	}
	*data = (uint32_t)(unsigned char)c;
}

void jp_test_exit(bool pass)
{
	uint64_t block[2];
	register uint64_t operation __asm__("x0") = JP_SEMIHOSTING_SYS_EXIT_EXTENDED;
	register uint64_t *parameter __asm__("x1") = block;

	block[0] = JP_SEMIHOSTING_APPLICATION_EXIT;
	block[1] = pass ? JP_EXIT_CODE_PASS : JP_EXIT_CODE_FAIL;
	__asm__ volatile("hlt #0xf000" : "+r"(operation) : "r"(parameter) : "memory");
	for (;;) {
		__asm__ volatile("msr daifset, #0xf\n\twfi");
	}
}
