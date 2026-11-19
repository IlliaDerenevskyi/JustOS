/*
 * wait.h - "40 seconds or ESC" for the test kernel.
 *
 * jp_test_wait_for_escape() waits until the given number of seconds has
 * passed or ESC is pressed. It polls (checks again and again) and never
 * sleeps. The loop is shared (wait_common.c); each architecture supplies a
 * source: a clock and an ESC check. Test code only.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_WAIT_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_WAIT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum JPTestWaitResult {
	JP_TEST_WAIT_TIMEOUT = 0,
	JP_TEST_WAIT_ESCAPE
} JPTestWaitResult;

typedef struct JPTestWaitSource {
	/* Monotonic count of clock ticks; wraps never within a run. */
	uint64_t (*ticks)(void *user);
	/* Clock ticks per second; 0 means "the clock is broken". */
	uint64_t ticks_per_second;
	/* True when ESC has been pressed since the last call. */
	bool (*escape_pressed)(void *user);
	void *user;
} JPTestWaitSource;

/* Shared loop; testable with a fake source. */
JPTestWaitResult jp_test_wait_poll(const JPTestWaitSource *source, uint32_t seconds);

/* Architecture entry: builds the real source and calls jp_test_wait_poll(). */
JPTestWaitResult jp_test_wait_for_escape(uint32_t seconds);

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_WAIT_H */
