/*
 * platform.h - what the test kernel needs from the machine.
 * Implemented once per architecture (serial_<arch>.c). Test code only.
 */

#ifndef JUSTOS_TESTS_QEMU_TEST_KERNEL_PLATFORM_H
#define JUSTOS_TESTS_QEMU_TEST_KERNEL_PLATFORM_H

#include <stdbool.h>

/* Writes one character to the serial port. */
void jp_test_serial_put(char c);

/*
 * Ends the run: x86_64 writes to the isa-debug-exit device (exit code 33 for
 * pass, 35 for fail), AArch64 uses semihosting (exit code 0 or 1).
 * If QEMU does not stop, the function halts the processor forever.
 */
void jp_test_exit(bool pass) __attribute__((noreturn));

#endif /* JUSTOS_TESTS_QEMU_TEST_KERNEL_PLATFORM_H */
