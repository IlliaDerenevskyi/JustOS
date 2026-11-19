/*
 * kernel_main.c - the Milestone 0 test kernel.
 *
 * Writes the data list of everything it received to the serial port (the
 * serial protocol of milestone0.md) and ends the run with PASS or FAIL.
 * Test code only; it is the kernel that the boot hands control to in the
 * QEMU tests.
 *
 * Steps (see "Test Kernel" in milestone0.md): write the list to the serial
 * port, draw it on the screen, wait 40 seconds or until ESC (unless built
 * with JP_TEST_KERNEL_NO_WAIT), end the run.
 */

#include <stdint.h>

#include "justos/boot/boot_types.h"
#include "platform.h"
#include "report.h"
#include "screen.h"
#include "wait.h"

#define JP_TEST_WAIT_SECONDS 40

static void serial_sink_put(void *user, char c)
{
	(void)user;
	jp_test_serial_put(c);
}

void kernel_main(JPBootContext *context) __attribute__((noreturn));

void kernel_main(JPBootContext *context)
{
	JPTestSink sink = { serial_sink_put, NULL };
	bool pass = false;

	if (context != NULL) {
		pass = jp_test_report_emit(context, &sink);
		/* In Milestone 0 the framebuffer is reached at its physical address. */
		jp_test_screen_show(context, (void *)(uintptr_t)context->framebuffer.base_address);
#ifndef JP_TEST_KERNEL_NO_WAIT
		jp_test_wait_for_escape(JP_TEST_WAIT_SECONDS);
#endif
	}
	jp_test_exit(pass);
}
