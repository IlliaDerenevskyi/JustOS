#include <stdint.h>

#include "wait.h"

enum {
	JP_PIT_CHANNEL0_PORT = 0x40,
	JP_PIT_COMMAND_PORT = 0x43,
	JP_PIT_COMMAND_CHANNEL0_LATCH = 0x00,
	JP_PIT_COMMAND_CHANNEL0_RATE = 0x34,   /* channel 0, low+high byte, mode 2, binary */
	JP_PIT_COUNTER_MASK = 0xFFFF,
	JP_PIT_COUNTER_SPAN = 0x10000,
	JP_BYTE_BITS = 8,
	JP_PS2_DATA_PORT = 0x60,
	JP_PS2_STATUS_PORT = 0x64,
	JP_PS2_STATUS_OUTPUT_FULL = 0x01,
	JP_PS2_STATUS_FROM_MOUSE = 0x20,
	JP_PS2_SCANCODE_ESCAPE_MAKE = 0x01,
	JP_COM1_DATA_PORT = 0x3F8,
	JP_COM1_LINE_STATUS_PORT = 0x3FD,
	JP_COM1_DATA_READY = 0x01,
	JP_ASCII_ESCAPE = 0x1B
};

#define JP_PIT_FREQUENCY 1193182ULL

typedef struct PitClock {
	uint16_t last;
	uint64_t total;
} PitClock;

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

/* The PIT counter counts down and wraps; adding the distance since the last
 * read gives a 64-bit tick count (the loop reads far more often than the
 * 55 ms in which the 16-bit counter wraps once). */
static uint16_t read_pit_counter(void)
{
	uint16_t low;
	uint16_t high;

	outb(JP_PIT_COMMAND_PORT, JP_PIT_COMMAND_CHANNEL0_LATCH);
	low = inb(JP_PIT_CHANNEL0_PORT);
	high = inb(JP_PIT_CHANNEL0_PORT);
	return (uint16_t)((high << JP_BYTE_BITS) | low);
}

static uint64_t pit_ticks(void *user)
{
	PitClock *clock = (PitClock *)user;
	uint16_t now = read_pit_counter();
	uint32_t distance = ((uint32_t)clock->last - now) & JP_PIT_COUNTER_MASK;

	clock->last = now;
	clock->total += distance;
	return clock->total;
}

static bool escape_pressed(void *user)
{
	(void)user;
	/* The PS/2 controller: a byte waits when bit 0 of the status is set; bit 5
	 * says the byte comes from a mouse (such bytes are discarded). */
	while ((inb(JP_PS2_STATUS_PORT) & JP_PS2_STATUS_OUTPUT_FULL) != 0) {
		uint8_t status = inb(JP_PS2_STATUS_PORT);
		uint8_t code = inb(JP_PS2_DATA_PORT);

		if ((status & JP_PS2_STATUS_FROM_MOUSE) == 0 &&
		    code == JP_PS2_SCANCODE_ESCAPE_MAKE) {
			return true;
		}
	}
	while ((inb(JP_COM1_LINE_STATUS_PORT) & JP_COM1_DATA_READY) != 0) {
		if (inb(JP_COM1_DATA_PORT) == JP_ASCII_ESCAPE) {
			return true;
		}
	}
	return false;
}

JPTestWaitResult jp_test_wait_for_escape(uint32_t seconds)
{
	PitClock clock;
	JPTestWaitSource source;

	/* Make the PIT count the full 16-bit range, then start from its count. */
	outb(JP_PIT_COMMAND_PORT, JP_PIT_COMMAND_CHANNEL0_RATE);
	outb(JP_PIT_CHANNEL0_PORT, 0);
	outb(JP_PIT_CHANNEL0_PORT, 0);
	clock.last = read_pit_counter();
	clock.total = 0;

	source.ticks = pit_ticks;
	source.ticks_per_second = JP_PIT_FREQUENCY;
	source.escape_pressed = escape_pressed;
	source.user = &clock;
	return jp_test_wait_poll(&source, seconds);
}
