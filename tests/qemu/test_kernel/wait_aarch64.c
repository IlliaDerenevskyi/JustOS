#include <stddef.h>
#include <stdint.h>

#include "wait.h"

/* PL011 UART of the QEMU "virt" machine. */
#define JP_PL011_BASE 0x09000000UL
#define JP_PL011_FLAGS_OFFSET 0x18UL
#define JP_PL011_FLAG_RX_EMPTY 0x10U
#define JP_ASCII_ESCAPE 0x1BU

static uint64_t generic_timer_ticks(void *user)
{
	uint64_t count;

	(void)user;
	__asm__ volatile("isb\n\tmrs %0, cntpct_el0" : "=r"(count) : : "memory");
	return count;
}

static uint64_t generic_timer_frequency(void)
{
	uint64_t frequency;

	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
	return frequency;
}

/* No PS/2 keyboard exists on AArch64, and a USB keyboard would need a driver:
 * ESC arrives as the byte 0x1B on the serial port. */
static bool escape_pressed(void *user)
{
	volatile uint32_t *data = (volatile uint32_t *)JP_PL011_BASE;
	volatile uint32_t *flags = (volatile uint32_t *)(JP_PL011_BASE + JP_PL011_FLAGS_OFFSET);

	(void)user;
	while ((*flags & JP_PL011_FLAG_RX_EMPTY) == 0) {
		if ((*data & 0xFFU) == JP_ASCII_ESCAPE) {
			return true;
		}
	}
	return false;
}

JPTestWaitResult jp_test_wait_for_escape(uint32_t seconds)
{
	JPTestWaitSource source;

	source.ticks = generic_timer_ticks;
	source.ticks_per_second = generic_timer_frequency();
	source.escape_pressed = escape_pressed;
	source.user = NULL;
	return jp_test_wait_poll(&source, seconds);
}
